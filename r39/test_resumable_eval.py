"""Tests for the vendorable resumable_eval.py reference implementation."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from resumable_eval import CheckpointStore, ResumeError, RunIdentity, fingerprint_json


def identity(**overrides: object) -> RunIdentity:
    values: dict[str, object] = {
        "run_id": "fixture-run",
        "dataset_fingerprint": fingerprint_json({"dataset": "fixture-v1"}),
        "config_fingerprint": fingerprint_json({"k": 4}),
        "code_revision": "git:test",
        "total_work_units": 3,
    }
    values.update(overrides)
    return RunIdentity(**values)  # type: ignore[arg-type]


class CheckpointStoreTests(unittest.TestCase):
    def test_resume_is_idempotent_and_repairs_torn_tail(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            run_dir = Path(temporary_dir) / "run"
            first = CheckpointStore(run_dir, identity())
            self.assertTrue(first.record_result("a", {"value": 1}))
            self.assertTrue(first.record_result("b", {"value": 2}))
            first.mark_interrupted("fixture interruption")

            with (run_dir / "results.jsonl").open("ab") as handle:
                handle.write(b'{"work_unit_id":"torn"')
                handle.flush()

            resumed = CheckpointStore(run_dir, identity())
            self.assertEqual(resumed.accounted_ids, frozenset({"a", "b"}))
            self.assertFalse(resumed.record_result("a", {"value": "duplicate"}))
            self.assertTrue(resumed.record_result("c", {"value": 3}))
            resumed.finish({"ids": sorted(resumed.accounted_ids)})

            state = json.loads((run_dir / "state.json").read_text(encoding="utf-8"))
            self.assertEqual(state["status"], "complete")
            self.assertEqual(state["completed_work_units"], 3)
            self.assertTrue((run_dir / "COMPLETE").exists())

            (run_dir / "COMPLETE").unlink()
            completed = CheckpointStore(run_dir, identity())
            self.assertTrue((run_dir / "COMPLETE").exists())
            completed.finish({"ids": ["a", "b", "c"]})
            with self.assertRaises(ResumeError):
                completed.record_result("d", {"value": 4})

    def test_resume_refuses_identity_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            run_dir = Path(temporary_dir) / "run"
            CheckpointStore(run_dir, identity()).record_result("a", {"value": 1})
            with self.assertRaises(ResumeError):
                CheckpointStore(run_dir, identity(config_fingerprint=fingerprint_json({"k": 8})))

    def test_duplicate_journal_id_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_dir:
            run_dir = Path(temporary_dir) / "run"
            CheckpointStore(run_dir, identity()).record_result("a", {"value": 1})
            duplicate = {"work_unit_id": "a", "status": "complete", "payload": {"value": 2}}
            with (run_dir / "results.jsonl").open("a", encoding="utf-8") as handle:
                handle.write(json.dumps(duplicate) + "\n")
            with self.assertRaises(ResumeError):
                CheckpointStore(run_dir, identity())


if __name__ == "__main__":
    unittest.main()
