#!/usr/bin/env python3
"""App-root selection, native precedence and append-only seed preservation."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts"))
import seed_law_line as seed


class SeedTest(unittest.TestCase):
    def run_seed(self, root, legacy=False, expect=0):
        env = dict(os.environ, EARTHCALL_DATA=str(root))
        env.pop("EARTHCALL_SEED_ROOT", None)
        if legacy:
            env["EARTHCALL_SEED_ROOT"] = str(root.parent)
        result = subprocess.run([sys.executable, str(REPO / "scripts/seed_law_line.py")],
                                env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, expect, result.stderr)
        return result

    def test_app_root_preserves_native_zone_existing_law_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "data-home.json").write_text('{}')
            zone_dir = root / "zones/LawLine"
            zone_dir.mkdir(parents=True)
            old = {"identifier": "LawLine", "name": "Zach's lived line", "world": {"objects": [{"objectID": "keep-me"}]},
                   "lexemes": [{"id": "my-word", "symbol": "Mine"}], "formationRelations": [], "lawRefs": ["my-law"]}
            original = json.dumps(old, indent=2).encode()
            native = zone_dir / "zone.ecsave"
            native.write_bytes(seed.ecform_wrap(original))
            shadow = zone_dir / "zone.json"
            shadow.write_text('{"shadow":"untouched"}')
            hear = root / "laws/law-line-hear/law.json"
            hear.parent.mkdir(parents=True)
            author = "did:earthcall:fixture-person"
            hear.write_text(json.dumps({"authors": [author], "law": {"authors": [author]}}))
            before_hear = hear.read_bytes()
            self.run_seed(root)
            new = json.loads(seed.ecform_text(native.read_bytes()))
            self.assertEqual(new["world"], old["world"])
            self.assertEqual(new["name"], old["name"])
            self.assertEqual(new["lexemes"][:1], old["lexemes"])
            self.assertEqual(new["lawRefs"][:1], old["lawRefs"])
            self.assertEqual(hear.read_bytes(), before_hear)
            self.assertEqual(shadow.read_text(), '{"shadow":"untouched"}')
            body = json.loads((root / "laws/law-line-shortcut-ctrl-t/law.json").read_text())
            self.assertEqual(body["authors"], [author])
            self.assertEqual(body["law"]["provenance"][0]["entityB"], author)
            digests = {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).digest()
                       for p in root.rglob('*') if p.is_file()}
            self.run_seed(root)
            self.assertEqual(digests, {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).digest()
                                      for p in root.rglob('*') if p.is_file()})

    def test_legacy_scratch_override_still_uses_saves(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "saves"
            self.run_seed(root, legacy=True)
            self.assertTrue((root / "zones/LawLine/zone.json").is_file())
            self.assertTrue((root / "laws/law-line-shortcut-cmd-t/law.json").is_file())

    def test_unsupported_native_refuses_before_any_seed_write(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "data-home.json").write_text('{}')
            native = root / "zones/LawLine/zone.ecsave"
            native.parent.mkdir(parents=True)
            native.write_bytes(b'unsupported-native-envelope')
            self.run_seed(root, expect=1)
            self.assertFalse((root / "laws").exists())
            self.assertEqual(native.read_bytes(), b'unsupported-native-envelope')


if __name__ == '__main__':
    unittest.main()
