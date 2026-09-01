import sys
import unittest
from unittest.mock import patch, MagicMock

# pygetwindow raises NotImplementedError on Linux upon import.
# Pre-emptively mock external platform-dependent modules before importing earthcall_agent.
mock_gw = MagicMock()
mock_gui = MagicMock()
mock_pw = MagicMock()

sys.modules["pygetwindow"] = mock_gw
sys.modules["pyautogui"] = mock_gui
sys.modules["playwright"] = mock_pw
sys.modules["playwright.sync_api"] = mock_pw

from src.Singularity.Foreign.py.agent.earthcall_agent import focus_window


class TestFocusWindow(unittest.TestCase):

    def setUp(self):
        mock_gw.reset_mock()

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.time.sleep")
    def test_focus_window_found(self, mock_sleep):
        mock_gw.getAllTitles.return_value = ["Earthcall Engine v1.0", "Calculator"]
        mock_win = MagicMock()
        mock_gw.getWindowsWithTitle.return_value = [mock_win]

        res = focus_window("Earthcall")

        self.assertEqual(res, 0)
        mock_gw.getWindowsWithTitle.assert_called_once_with("Earthcall Engine v1.0")
        mock_win.activate.assert_called_once()
        mock_sleep.assert_called_once_with(0.2)

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.time.sleep")
    def test_focus_window_not_found(self, mock_sleep):
        mock_gw.getAllTitles.return_value = ["Calculator", "Terminal"]

        res = focus_window("Earthcall")

        self.assertEqual(res, 1)
        mock_gw.getWindowsWithTitle.assert_not_called()
        mock_sleep.assert_not_called()

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.time.sleep")
    def test_focus_window_case_insensitive(self, mock_sleep):
        mock_gw.getAllTitles.return_value = ["EARTHCALL ENGINE"]
        mock_win = MagicMock()
        mock_gw.getWindowsWithTitle.return_value = [mock_win]

        res = focus_window("earthcall")

        self.assertEqual(res, 0)
        mock_gw.getWindowsWithTitle.assert_called_once_with("EARTHCALL ENGINE")
        mock_win.activate.assert_called_once()

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.time.sleep")
    def test_focus_window_multiple_matches_selects_first(self, mock_sleep):
        mock_gw.getAllTitles.return_value = ["Earthcall Window 1", "Earthcall Window 2"]
        mock_win = MagicMock()
        mock_gw.getWindowsWithTitle.return_value = [mock_win]

        res = focus_window("Earthcall")

        self.assertEqual(res, 0)
        mock_gw.getWindowsWithTitle.assert_called_once_with("Earthcall Window 1")
        mock_win.activate.assert_called_once()


if __name__ == "__main__":
    unittest.main()
