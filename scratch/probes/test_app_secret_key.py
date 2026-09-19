#!/usr/bin/env python3
import importlib
import os
import sys
import unittest
from pathlib import Path

# Add app directory to sys.path
_CURRENT = Path(__file__).resolve().parent
_REPO_ROOT = _CURRENT.parents[1]
_PY_DIR = _REPO_ROOT / "src" / "Singularity" / "Foreign" / "py"
sys.path.insert(0, str(_PY_DIR))

class TestAppSecretKey(unittest.TestCase):

    def setUp(self):
        # Remove app module from sys.modules to ensure clean import
        if "app" in sys.modules:
            del sys.modules["app"]

    def tearDown(self):
        if "app" in sys.modules:
            del sys.modules["app"]

    def test_secret_key_from_env(self):
        """Test that SECRET_KEY environment variable is respected when set."""
        test_key = "custom-test-secret-key-12345"
        os.environ["SECRET_KEY"] = test_key
        try:
            import app
            self.assertEqual(app.app.config["SECRET_KEY"], test_key)
        finally:
            os.environ.pop("SECRET_KEY", None)

    def test_secret_key_auto_generated_when_unset(self):
        """Test that SECRET_KEY is dynamically generated and secure when not in env."""
        if "SECRET_KEY" in os.environ:
            del os.environ["SECRET_KEY"]

        import app
        secret1 = app.app.config["SECRET_KEY"]

        # Verify it's not the old hardcoded fallback
        self.assertNotEqual(secret1, "earthcall-secret-key-logos")
        # Check it's a 64-character hex string (32 bytes token_hex)
        self.assertEqual(len(secret1), 64)
        self.assertTrue(all(c in "0123456789abcdefABCDEF" for c in secret1))

        # Test uniqueness on second load
        if "app" in sys.modules:
            del sys.modules["app"]

        import app as app_module2
        importlib.reload(app_module2)
        secret2 = app_module2.app.config["SECRET_KEY"]

        self.assertNotEqual(secret1, secret2)

if __name__ == "__main__":
    unittest.main()
