"""Standard-library reference implementation for resumable evaluation jobs.

Copy this file into the owning repository, adapt its naming and payload types, and
keep the durability and identity invariants covered by project tests.
"""

from __future__ import annotations

import hashlib
import json
import os
import signal
import tempfile
import threading
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable


SCHEMA_VERSION = 1
VALID_STATUSES = {"pending", "running", "interrupted", "failed", "complete"}


class ResumeError(RuntimeError):
    """Raised when persisted state cannot safely resume the requested run."""


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="microseconds").replace("+00:00", "Z")


def canonical_json_bytes(value: Any) -> bytes:
    return (
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n"
    ).encode("utf-8")


def fingerprint_json(value: Any) -> str:
    return "sha256:" + hashlib.sha256(canonical_json_bytes(value)).hexdigest()


def fingerprint_file(path: str | os.PathLike[str]) -> str:
    digest = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return "sha256:" + digest.hexdigest()


def _fsync_directory(path: Path) -> None:
    descriptor = os.open(path, os.O_RDONLY)
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def atomic_write(path: Path, payload: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    temporary_path = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "wb") as handle:
            handle.write(payload)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary_path, path)
        _fsync_directory(path.parent)
    except BaseException:
        temporary_path.unlink(missing_ok=True)
        raise


def atomic_write_json(path: Path, value: Any) -> None:
    atomic_write(path, canonical_json_bytes(value))


def append_jsonl(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = canonical_json_bytes(value)
    descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o644)
    try:
        written = 0
        while written < len(payload):
            chunk_size = os.write(descriptor, payload[written:])
            if chunk_size == 0:
                raise OSError("zero-byte write while appending journal record")
            written += chunk_size
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def load_jsonl(path: Path, *, repair_torn_tail: bool) -> list[dict[str, Any]]:
    if not path.exists():
        return []

    records: list[dict[str, Any]] = []
    last_good_offset = 0
    with path.open("rb") as handle:
        while True:
            record_offset = handle.tell()
            line = handle.readline()
            if not line:
                break
            is_final_line = handle.peek(1) == b""
            if not line.endswith(b"\n"):
                if repair_torn_tail and is_final_line:
                    with path.open("r+b") as repair_handle:
                        repair_handle.truncate(record_offset)
                        repair_handle.flush()
                        os.fsync(repair_handle.fileno())
                    _fsync_directory(path.parent)
                    break
                raise ResumeError(f"invalid journal record at byte {record_offset}: not newline-terminated")
            try:
                value = json.loads(line)
                if not isinstance(value, dict):
                    raise ValueError("record is not an object")
                work_unit_id = value.get("work_unit_id")
                if not isinstance(work_unit_id, str) or not work_unit_id:
                    raise ValueError("record has no non-empty work_unit_id")
            except (UnicodeDecodeError, json.JSONDecodeError, ValueError) as error:
                raise ResumeError(f"invalid journal record at byte {record_offset}: {error}") from error
            records.append(value)
            last_good_offset = handle.tell()

    if path.stat().st_size < last_good_offset:
        raise ResumeError(f"journal unexpectedly shrank while reading: {path}")
    return records


@dataclass(frozen=True)
class RunIdentity:
    run_id: str
    dataset_fingerprint: str
    config_fingerprint: str
    code_revision: str
    total_work_units: int
    schema_version: int = SCHEMA_VERSION

    def validate(self) -> None:
        for field_name in ("run_id", "dataset_fingerprint", "config_fingerprint", "code_revision"):
            if not getattr(self, field_name):
                raise ValueError(f"{field_name} must be non-empty")
        if self.total_work_units < 0:
            raise ValueError("total_work_units must be non-negative")


class CheckpointStore:
    """Single-writer durable journal and checkpoint store."""

    def __init__(self, run_dir: str | os.PathLike[str], identity: RunIdentity):
        identity.validate()
        self.run_dir = Path(run_dir).resolve()
        self.identity = identity
        self.manifest_path = self.run_dir / "manifest.json"
        self.state_path = self.run_dir / "state.json"
        self.results_path = self.run_dir / "results.jsonl"
        self.failures_path = self.run_dir / "failures.jsonl"
        self.final_path = self.run_dir / "final.json"
        self.checksum_path = self.run_dir / "final.sha256"
        self.complete_path = self.run_dir / "COMPLETE"
        self.run_dir.mkdir(parents=True, exist_ok=True)
        self._started_at = utc_now()
        self._result_ids: set[str] = set()
        self._failure_ids: set[str] = set()
        self._last_work_unit_id: str | None = None
        self._status = "pending"
        self._open_or_create()

    @property
    def accounted_ids(self) -> frozenset[str]:
        return frozenset(self._result_ids | self._failure_ids)

    @property
    def completed_count(self) -> int:
        return len(self._result_ids)

    @property
    def failed_count(self) -> int:
        return len(self._failure_ids)

    def is_accounted(self, work_unit_id: str) -> bool:
        return work_unit_id in self._result_ids or work_unit_id in self._failure_ids

    def _manifest(self) -> dict[str, Any]:
        return {**asdict(self.identity), "created_at": self._started_at}

    def _open_or_create(self) -> None:
        if self.manifest_path.exists():
            persisted = json.loads(self.manifest_path.read_text(encoding="utf-8"))
            requested = asdict(self.identity)
            mismatches = {
                key: (persisted.get(key), value)
                for key, value in requested.items()
                if persisted.get(key) != value
            }
            if mismatches:
                raise ResumeError(f"run identity mismatch: {mismatches}")
            if not self.state_path.exists():
                raise ResumeError("manifest exists but state.json is missing")
            state = json.loads(self.state_path.read_text(encoding="utf-8"))
            self._started_at = state.get("started_at", persisted.get("created_at", self._started_at))
        else:
            occupied = [
                path.name
                for path in self.run_dir.iterdir()
                if path.name not in {"lost+found"} and not path.name.startswith(".manifest.json.")
            ]
            if occupied:
                raise ResumeError(f"refusing to initialize non-empty run directory: {occupied}")
            atomic_write_json(self.manifest_path, self._manifest())

        results = load_jsonl(self.results_path, repair_torn_tail=True)
        failures = load_jsonl(self.failures_path, repair_torn_tail=True)
        self._result_ids = self._unique_ids(results, self.results_path)
        self._failure_ids = self._unique_ids(failures, self.failures_path)
        overlap = self._result_ids & self._failure_ids
        if overlap:
            raise ResumeError(f"work units occur in both result and failure journals: {sorted(overlap)!r}")
        if len(self.accounted_ids) > self.identity.total_work_units:
            raise ResumeError("journal contains more work units than the manifest total")

        previous_status = None
        if self.state_path.exists():
            previous_state = json.loads(self.state_path.read_text(encoding="utf-8"))
            previous_status = previous_state.get("status")
            self._last_work_unit_id = previous_state.get("last_work_unit_id")
            for key, expected in asdict(self.identity).items():
                if previous_state.get(key) != expected:
                    raise ResumeError(f"state identity mismatch for {key}")
            if previous_status not in VALID_STATUSES:
                raise ResumeError(f"invalid persisted status: {previous_status!r}")
        if previous_status == "complete":
            if len(self.accounted_ids) != self.identity.total_work_units:
                raise ResumeError("complete state does not account for all work units")
            self._status = "complete"
            self._verify_completion(repair_missing_marker=True)
            return
        if self.complete_path.exists():
            raise ResumeError("COMPLETE marker exists but state is not complete")
        self._write_state("running")

    @staticmethod
    def _unique_ids(records: Iterable[dict[str, Any]], path: Path) -> set[str]:
        ids: set[str] = set()
        for record in records:
            work_unit_id = record["work_unit_id"]
            if work_unit_id in ids:
                raise ResumeError(f"duplicate work_unit_id {work_unit_id!r} in {path}")
            ids.add(work_unit_id)
        return ids

    def _write_state(self, status: str) -> None:
        if status not in VALID_STATUSES:
            raise ValueError(f"invalid status: {status}")
        state = {
            **asdict(self.identity),
            "completed_work_units": self.completed_count,
            "failed_work_units": self.failed_count,
            "status": status,
            "started_at": self._started_at,
            "updated_at": utc_now(),
            "last_work_unit_id": self._last_work_unit_id,
        }
        atomic_write_json(self.state_path, state)
        self._status = status

    def _verify_completion(self, *, repair_missing_marker: bool) -> str:
        if not self.final_path.is_file() or not self.checksum_path.is_file():
            raise ResumeError("complete state is missing final.json or final.sha256")
        final_bytes = self.final_path.read_bytes()
        actual_checksum = hashlib.sha256(final_bytes).hexdigest()
        try:
            expected_checksum = self.checksum_path.read_text(encoding="ascii").split()[0]
        except (IndexError, UnicodeDecodeError) as error:
            raise ResumeError("final.sha256 is malformed") from error
        if actual_checksum != expected_checksum:
            raise ResumeError("final.sha256 does not match final.json")
        if self.complete_path.exists():
            marker = json.loads(self.complete_path.read_text(encoding="utf-8"))
            if marker.get("run_id") != self.identity.run_id:
                raise ResumeError("COMPLETE run_id does not match the run identity")
            if marker.get("final_sha256") != actual_checksum:
                raise ResumeError("COMPLETE checksum does not match final.json")
        elif repair_missing_marker:
            atomic_write_json(
                self.complete_path,
                {
                    "run_id": self.identity.run_id,
                    "completed_at": utc_now(),
                    "final_sha256": actual_checksum,
                },
            )
        else:
            raise ResumeError("COMPLETE marker is missing")
        return actual_checksum

    def _ensure_writable(self) -> None:
        if self._status == "complete":
            raise ResumeError("completed run is immutable")
        if len(self.accounted_ids) >= self.identity.total_work_units:
            raise ResumeError("all declared work units are already accounted for")

    @staticmethod
    def _validate_work_unit_id(work_unit_id: str) -> None:
        if not isinstance(work_unit_id, str) or not work_unit_id:
            raise ValueError("work_unit_id must be a non-empty string")

    def record_result(self, work_unit_id: str, payload: Any) -> bool:
        self._validate_work_unit_id(work_unit_id)
        if self.is_accounted(work_unit_id):
            return False
        self._ensure_writable()
        append_jsonl(
            self.results_path,
            {
                "work_unit_id": work_unit_id,
                "status": "complete",
                "recorded_at": utc_now(),
                "payload": payload,
            },
        )
        self._result_ids.add(work_unit_id)
        self._last_work_unit_id = work_unit_id
        self._write_state("running")
        return True

    def record_failure(self, work_unit_id: str, error: Any) -> bool:
        self._validate_work_unit_id(work_unit_id)
        if self.is_accounted(work_unit_id):
            return False
        self._ensure_writable()
        append_jsonl(
            self.failures_path,
            {
                "work_unit_id": work_unit_id,
                "status": "failed",
                "recorded_at": utc_now(),
                "error": error,
            },
        )
        self._failure_ids.add(work_unit_id)
        self._last_work_unit_id = work_unit_id
        self._write_state("running")
        return True

    def mark_interrupted(self, reason: str) -> None:
        if self._status == "complete":
            raise ResumeError("completed run is immutable")
        self._write_state("interrupted")
        atomic_write_json(self.run_dir / "interruption.json", {"reason": reason, "recorded_at": utc_now()})

    def finish(self, final_payload: Any, *, allow_failures: bool = False) -> None:
        final_bytes = canonical_json_bytes(final_payload)
        if self._status == "complete":
            self._verify_completion(repair_missing_marker=True)
            if self.final_path.read_bytes() != final_bytes:
                raise ResumeError("completed run has a different final payload")
            return
        if len(self.accounted_ids) != self.identity.total_work_units:
            raise ResumeError(
                f"cannot finish: accounted {len(self.accounted_ids)} of {self.identity.total_work_units}"
            )
        if self.failed_count and not allow_failures:
            raise ResumeError(f"cannot finish with {self.failed_count} failed work units")
        atomic_write(self.final_path, final_bytes)
        checksum = hashlib.sha256(final_bytes).hexdigest()
        atomic_write(self.checksum_path, f"{checksum}  final.json\n".encode("ascii"))
        self._write_state("complete")
        atomic_write_json(
            self.complete_path,
            {"run_id": self.identity.run_id, "completed_at": utc_now(), "final_sha256": checksum},
        )


class StopController:
    """Cooperative SIGINT/SIGTERM flag; checkpoint from normal control flow."""

    def __init__(self) -> None:
        self._event = threading.Event()
        self.signal_name: str | None = None

    @property
    def requested(self) -> bool:
        return self._event.is_set()

    def install(self) -> None:
        def request_stop(signum: int, _frame: Any) -> None:
            self.signal_name = signal.Signals(signum).name
            self._event.set()

        signal.signal(signal.SIGINT, request_stop)
        signal.signal(signal.SIGTERM, request_stop)
