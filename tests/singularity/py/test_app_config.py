import unittest
from unittest.mock import patch
import sys
import os
from pathlib import Path

# Add python path
_SRC = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(_SRC))
sys.path.insert(0, str(_SRC / "Singularity" / "Foreign" / "py"))

class TestAppConfig(unittest.TestCase):

    @patch("flask_socketio.SocketIO.run")
    def test_werkzeug_unsafe_flag_reflects_debug_mode(self, mock_socketio_run):
        # Test when DEBUG is False
        with patch.dict(os.environ, {"DEBUG": "False"}):
            import importlib
            import src.Singularity.Foreign.py.app as app_mod

            # Simulate __main__ execution block logic
            debug = os.environ.get('DEBUG', 'False').lower() in ('true', '1', 't')
            app_mod.socketio.run(app_mod.app, host="127.0.0.1", port=5005, debug=debug, allow_unsafe_werkzeug=debug, use_reloader=False)

            mock_socketio_run.assert_called_with(
                app_mod.app, host="127.0.0.1", port=5005, debug=False, allow_unsafe_werkzeug=False, use_reloader=False
            )

        # Test when DEBUG is True
        with patch.dict(os.environ, {"DEBUG": "True"}):
            debug = os.environ.get('DEBUG', 'False').lower() in ('true', '1', 't')
            app_mod.socketio.run(app_mod.app, host="127.0.0.1", port=5005, debug=debug, allow_unsafe_werkzeug=debug, use_reloader=False)

            mock_socketio_run.assert_called_with(
                app_mod.app, host="127.0.0.1", port=5005, debug=True, allow_unsafe_werkzeug=True, use_reloader=False
            )

if __name__ == "__main__":
    unittest.main()
