#!/usr/bin/env python3
"""Independently validate a resumable evaluation run directory."""

from __future__ import annotations

import argparse
import hashlib
import json
import tempfile
from pathlib import Path
from typing import Any


IDENTITY_FIELDS = (
    "schema_version",
    "run_id",
    "dataset_fingerprint",
    "config_fingerprint",
    "code_revision",
    "total_work_units",
)
VALID_STATUSES = {"pending", "running", "interrupted", "failed", "complete"}


class ValidationError(RuntimeError):
    pass


def read_object(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise ValidationError(f"missing required file: {path.name}")
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValidationError(f"invalid JSON in {path.name}: {error}") from error
    if not isinstance(value, dict):
        raise ValidationError(f"{path.name} must contain a JSON object")
    return value


def read_journal(path: Path) -> dict[str, dict[str, Any]]:
    if not path.exists():
        return {}
    records: dict[str, dict[str, Any]] = {}
    with path.open("rb") as handle:
        for line_number, line in enumerate(handle, 1):
            if not line.endswith(b"\n"):
                raise ValidationError(f"{path.name}:{line_number} has a torn final record")
            try:
                record = json.loads(line)
            except (UnicodeDecodeError, json.JSONDecodeError) as error:
                raise ValidationError(f"invalid JSON at {path.name}:{line_number}: {error}") from error
            if not isinstance(record, dict):
                raise ValidationError(f"{path.name}:{line_number} is not an object")
            work_unit_id = record.get("work_unit_id")
            if not isinstance(work_unit_id, str) or not work_unit_id:
                raise ValidationError(f"{path.name}:{line_number} has no non-empty work_unit_id")
            if work_unit_id in records:
                raise ValidationError(f"duplicate work_unit_id {work_unit_id!r} in {path.name}")
            records[work_unit_id] = record
    return records


def validate(run_dir: Path, *, require_complete: bool) -> dict[str, Any]:
    run_dir = run_dir.resolve()
    if not run_dir.is_dir():
        raise ValidationError(f"run directory does not exist: {run_dir}")

    manifest = read_object(run_dir / "manifest.json")
    state = read_object(run_dir / "state.json")
    for field in IDENTITY_FIELDS:
        if field not in manifest:
            raise ValidationError(f"manifest.json is missing {field}")
        if state.get(field) != manifest[field]:
            raise ValidationError(f"state/manifest mismatch for {field}")

    if not isinstance(manifest["total_work_units"], int) or manifest["total_work_units"] < 0:
        raise ValidationError("total_work_units must be a non-negative integer")
    status = state.get("status")
    if status not in VALID_STATUSES:
        raise ValidationError(f"invalid state status: {status!r}")

    results = read_journal(run_dir / "results.jsonl")
    failures = read_journal(run_dir / "failures.jsonl")
    overlap = set(results) & set(failures)
    if overlap:
        raise ValidationError(f"IDs occur in result and failure journals: {sorted(overlap)!r}")
    if state.get("completed_work_units") != len(results):
        raise ValidationError("completed_work_units does not match results.jsonl")
    if state.get("failed_work_units") != len(failures):
        raise ValidationError("failed_work_units does not match failures.jsonl")

    accounted = len(results) + len(failures)
    total = manifest["total_work_units"]
    if accounted > total:
        raise ValidationError(f"accounted work units exceed total: {accounted} > {total}")

    complete_marker = run_dir / "COMPLETE"
    if complete_marker.exists() and status != "complete":
        raise ValidationError("COMPLETE exists but state status is not complete")
    if status == "complete" or require_complete:
        if status != "complete":
            raise ValidationError(f"run is not complete; status is {status!r}")
        if accounted != total:
            raise ValidationError(f"complete run accounts for {accounted} of {total} work units")
        marker = read_object(complete_marker)
        if marker.get("run_id") != manifest["run_id"]:
            raise ValidationError("COMPLETE run_id does not match manifest")
        final_path = run_dir / "final.json"
        checksum_path = run_dir / "final.sha256"
        if not final_path.is_file() or not checksum_path.is_file():
            raise ValidationError("complete run is missing final.json or final.sha256")
        expected_checksum = checksum_path.read_text(encoding="ascii").split()[0]
        actual_checksum = hashlib.sha256(final_path.read_bytes()).hexdigest()
        if expected_checksum != actual_checksum:
            raise ValidationError("final.sha256 does not match final.json")
        if marker.get("final_sha256") != actual_checksum:
            raise ValidationError("COMPLETE checksum does not match final.json")

    return {
        "run_dir": str(run_dir),
        "run_id": manifest["run_id"],
        "status": status,
        "completed": len(results),
        "failed": len(failures),
        "total": total,
        "valid": True,
    }


def self_test() -> None:
    with tempfile.TemporaryDirectory() as temporary_dir:
        run_dir = Path(temporary_dir) / "run"
        run_dir.mkdir()
        identity = {
            "schema_version": 1,
            "run_id": "self-test",
            "dataset_fingerprint": "sha256:dataset",
            "config_fingerprint": "sha256:config",
            "code_revision": "git:test",
            "total_work_units": 1,
        }
        manifest = {**identity, "created_at": "2026-01-01T00:00:00Z"}
        state = {
            **identity,
            "completed_work_units": 1,
            "failed_work_units": 0,
            "status": "complete",
            "started_at": "2026-01-01T00:00:00Z",
            "updated_at": "2026-01-01T00:01:00Z",
        }
        final_bytes = b'{"ok":true}\n'
        checksum = hashlib.sha256(final_bytes).hexdigest()
        files: dict[str, bytes] = {
            "manifest.json": (json.dumps(manifest) + "\n").encode(),
            "state.json": (json.dumps(state) + "\n").encode(),
            "results.jsonl": b'{"work_unit_id":"one","status":"complete"}\n',
            "final.json": final_bytes,
            "final.sha256": f"{checksum}  final.json\n".encode(),
            "COMPLETE": (json.dumps({"run_id": "self-test", "final_sha256": checksum}) + "\n").encode(),
        }
        for name, payload in files.items():
            (run_dir / name).write_bytes(payload)
        result = validate(run_dir, require_complete=True)
        if not result["valid"]:
            raise AssertionError("valid fixture did not validate")

        with (run_dir / "results.jsonl").open("ab") as handle:
            handle.write(b'{"work_unit_id":"torn"')
        try:
            validate(run_dir, require_complete=True)
        except ValidationError:
            pass
        else:
            raise AssertionError("torn journal fixture unexpectedly validated")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", nargs="?", type=Path)
    parser.add_argument("--require-complete", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if not args.self_test and args.run_dir is None:
        parser.error("RUN_DIR is required unless --self-test is used")
    return args


def main() -> int:
    args = parse_args()
    try:
        if args.self_test:
            self_test()
            print("self-test: ok")
            return 0
        result = validate(args.run_dir, require_complete=args.require_complete)
    except (OSError, ValidationError) as error:
        print(f"invalid: {error}")
        return 1
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
