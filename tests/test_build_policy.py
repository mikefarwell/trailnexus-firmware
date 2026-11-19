import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('build_policy', ROOT / 'scripts/build_policy.py')
policy = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(policy)


class BuildPolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for name in ['.ci', 'scripts', '.github/workflows', 'targets/core', 'tests']:
            (self.root / name).mkdir(parents=True, exist_ok=True)
        for name in ['.ci/build-profiles.json', '.ci/build-policy.json', '.ci/cache-policy.json',
                     'scripts/build_policy.py', '.github/workflows/build.yml', 'requirements.txt',
                     'targets/core/development.yaml', 'tests/board_registry.cpp']:
            shutil.copyfile(ROOT / name, self.root / name)
        self.git('init', '-q')
        self.git('add', '.')
        self.git('-c', 'user.name=Test', '-c', 'user.email=test@example.invalid', 'commit', '-qm', 'base')

    def git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.root), *args], text=True).strip()

    def test_conservative_routing(self):
        _, rules, _ = policy.load(self.root)
        for paths in [None, [], ['components/new.cpp'], ['unknown.file'], ['README.md', 'requirements.txt']]:
            self.assertTrue(policy.classify(paths, rules)['fullBuild'])
        self.assertFalse(policy.classify(['README.md', 'docs/guide.md', '.change-reviews/example.json'], rules)['fullBuild'])
        for event in ['push', 'workflow_dispatch', 'unknown']:
            self.assertTrue(policy.classify(['README.md'], rules, event)['fullBuild'])
        self.assertTrue(policy.classify(['README.md'], rules, force_full=True)['fullBuild'])

    def test_source_rename_cannot_hide_as_metadata(self):
        base = self.git('rev-parse', 'HEAD')
        (self.root / 'docs').mkdir()
        self.git('mv', 'tests/board_registry.cpp', 'docs/guide.md')
        self.git('-c', 'user.name=Test', '-c', 'user.email=test@example.invalid', 'commit', '-qm', 'rename')
        paths = policy.changed_paths(self.root, base)
        self.assertIn('tests/board_registry.cpp', paths)
        self.assertTrue(policy.classify(paths, policy.load(ROOT)[1])['fullBuild'])
        self.assertIsNone(policy.changed_paths(self.root, 'invalid-commit'))

    def test_policy_rejects_unsafe_shortcuts(self):
        with self.assertRaises(ValueError):
            policy.load(self.root, target='different.yaml')
        path = self.root / '.ci/build-policy.json'
        value = json.loads(path.read_text())
        value['metadataOnlyPatterns'].append('components/*')
        path.write_text(json.dumps(value))
        with self.assertRaises(ValueError):
            policy.load(self.root)

    def test_download_cache_identity(self):
        first = policy.cache_keys(self.root)
        (self.root / 'README.md').write_text('Metadata')
        self.assertEqual(first['key'], policy.cache_keys(self.root)['key'])
        (self.root / 'tests/board_registry.cpp').write_text('// Always compiled, never cached')
        self.assertEqual(first['key'], policy.cache_keys(self.root)['key'])
        (self.root / 'requirements.txt').write_text('esphome==different')
        second = policy.cache_keys(self.root)
        self.assertNotEqual(first['sourceHash'], second['sourceHash'])
        with patch.dict(os.environ, {'ImageVersion': 'different-image'}):
            self.assertNotEqual(second['toolHash'], policy.cache_keys(self.root)['toolHash'])
        (self.root / '.github/workflows/build.yml').write_text('# policy change')
        self.assertNotEqual(second['policyHash'], policy.cache_keys(self.root)['policyHash'])

    def test_cache_cannot_store_build_or_qualification_results(self):
        path = self.root / '.ci/cache-policy.json'
        value = json.loads(path.read_text())
        value['cacheCompiledOutputs'] = True
        path.write_text(json.dumps(value))
        with self.assertRaises(ValueError):
            policy.load(self.root)


if __name__ == '__main__':
    unittest.main()
