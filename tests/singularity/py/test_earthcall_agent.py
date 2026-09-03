import unittest
from unittest.mock import patch, MagicMock
import sys
from pathlib import Path

# Add python path
_SRC = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(_SRC))

# Mock platform-dependent and heavy modules before importing earthcall_agent
sys.modules['pygetwindow'] = MagicMock()
sys.modules['pyautogui'] = MagicMock()
sys.modules['playwright.sync_api'] = MagicMock()

from src.Singularity.Foreign.py.agent import earthcall_agent

class TestEarthcallAgent(unittest.TestCase):

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.gui")
    def test_type_text_standard_string(self, mock_gui):
        result = earthcall_agent.type_text("Hello World")
        self.assertEqual(result, 0)
        mock_gui.typewrite.assert_called_once_with("Hello World", interval=0.02)

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.gui")
    def test_type_text_empty_string(self, mock_gui):
        result = earthcall_agent.type_text("")
        self.assertEqual(result, 0)
        mock_gui.typewrite.assert_called_once_with("", interval=0.02)

if __name__ == "__main__":
    unittest.main()
