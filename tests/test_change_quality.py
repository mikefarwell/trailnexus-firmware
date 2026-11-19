import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("quality", Path(__file__).resolve().parents[1] / "scripts/change_quality_gate.py")
quality = importlib.util.module_from_spec(spec); spec.loader.exec_module(quality)


class QualityTests(unittest.TestCase):
    def test_missing_record_and_bad_json_are_not_passes(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); (root / "bad.json").write_text("{bad")
            result = quality.inspect_paths(root, ["bad.json"])
            self.assertEqual(len(result["failures"]), 2)

    def test_links_and_all_impact_dispositions(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); (root / ".change-reviews").mkdir()
            record = {"summary": "Tooling", "verification": ["unit tests"], "impacts": {
                area: {"status": "no-change", "reason": "No product behavior change"} for area in quality.AREAS}}
            (root / ".change-reviews/test.json").write_text(json.dumps(record))
            (root / "README.md").write_text("[bad](missing.md)")
            result = quality.inspect_paths(root, ["README.md", ".change-reviews/test.json"])
            self.assertEqual(len(result["failures"]), 1)
            (root / "missing.md").write_text("exists")
            self.assertFalse(quality.inspect_paths(root, ["README.md", ".change-reviews/test.json"])["failures"])
