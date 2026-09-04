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

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.sync_playwright")
    def test_with_browser_decorator(self, mock_sync_playwright):
        # Setup mock playwright structure
        mock_p = MagicMock()
        mock_browser = MagicMock()
        mock_ctx = MagicMock()
        mock_page = MagicMock()

        mock_sync_playwright.return_value.__enter__.return_value = mock_p
        mock_p.chromium.launch.return_value = mock_browser
        mock_browser.new_context.return_value = mock_ctx
        mock_ctx.new_page.return_value = mock_page

        # Create a dummy function to wrap
        dummy_func = MagicMock(return_value="success")

        # Apply decorator
        wrapped = earthcall_agent.with_browser(dummy_func)

        # Call the wrapped function
        result = wrapped("arg1", kwarg1="val1")

        # Assertions
        self.assertEqual(result, "success")
        mock_p.chromium.launch.assert_called_once_with(headless=False)
        mock_browser.new_context.assert_called_once()
        mock_ctx.new_page.assert_called_once()

        # Verify the dummy function received the page and original args
        dummy_func.assert_called_once_with(mock_page, "arg1", kwarg1="val1")

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.sync_playwright")
    def test_with_browser_decorator_exception(self, mock_sync_playwright):
        # Setup mock playwright structure
        mock_p = MagicMock()
        mock_browser = MagicMock()
        mock_ctx = MagicMock()
        mock_page = MagicMock()

        mock_sync_playwright.return_value.__enter__.return_value = mock_p
        mock_p.chromium.launch.return_value = mock_browser
        mock_browser.new_context.return_value = mock_ctx
        mock_ctx.new_page.return_value = mock_page

        # Create a dummy function that raises an exception
        dummy_func = MagicMock(side_effect=ValueError("Test Exception"))

        # Apply decorator
        wrapped = earthcall_agent.with_browser(dummy_func)

        # Call the wrapped function and assert exception is raised
        with self.assertRaises(ValueError) as context:
            wrapped("arg1")

        self.assertEqual(str(context.exception), "Test Exception")

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.sync_playwright")
    def test_open_url_success(self, mock_sync_playwright):
        # Setup mock playwright structure
        mock_p = MagicMock()
        mock_browser = MagicMock()
        mock_ctx = MagicMock()
        mock_page = MagicMock()

        mock_sync_playwright.return_value.__enter__.return_value = mock_p
        mock_p.chromium.launch.return_value = mock_browser
        mock_browser.new_context.return_value = mock_ctx
        mock_ctx.new_page.return_value = mock_page

        url_to_open = "https://example.com"
        result = earthcall_agent.open_url(url_to_open)

        self.assertEqual(result, 0)
        mock_page.goto.assert_called_once_with(url_to_open)

    @patch("src.Singularity.Foreign.py.agent.earthcall_agent.sync_playwright")
    def test_open_url_exception(self, mock_sync_playwright):
        # Setup mock playwright structure
        mock_p = MagicMock()
        mock_browser = MagicMock()
        mock_ctx = MagicMock()
        mock_page = MagicMock()

        mock_sync_playwright.return_value.__enter__.return_value = mock_p
        mock_p.chromium.launch.return_value = mock_browser
        mock_browser.new_context.return_value = mock_ctx
        mock_ctx.new_page.return_value = mock_page

        mock_page.goto.side_effect = Exception("Network Error")

        url_to_open = "https://invalid-url.com"
        with self.assertRaises(Exception) as context:
            earthcall_agent.open_url(url_to_open)

        self.assertEqual(str(context.exception), "Network Error")
        mock_page.goto.assert_called_once_with(url_to_open)


if __name__ == "__main__":
    unittest.main()
