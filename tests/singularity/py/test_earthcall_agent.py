import sys
import os
import unittest
from unittest.mock import MagicMock, patch

# Ensure repo root is on sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../")))

# Mock dependencies before importing earthcall_agent
sys.modules['pygetwindow'] = MagicMock()
sys.modules['pyautogui'] = MagicMock()
sys.modules['playwright'] = MagicMock()
sys.modules['playwright.sync_api'] = MagicMock()

from src.Singularity.Foreign.py.agent import earthcall_agent


class TestEarthcallAgentClickText(unittest.TestCase):
    def test_click_text_unwrapped(self):
        """Test the underlying click_text function logic with a mock page."""
        mock_page = MagicMock()
        mock_locator = MagicMock()
        mock_page.locator.return_value = mock_locator

        # Access original unwrapped function from __wrapped__ if present
        unwrapped_fn = getattr(earthcall_agent.click_text, '__wrapped__', earthcall_agent.click_text)
        # If wrapped multiple times or wrapped by custom decorator, extract inner function
        while hasattr(unwrapped_fn, '__wrapped__'):
            unwrapped_fn = getattr(unwrapped_fn, '__wrapped__')

        # In earthcall_agent.py, with_browser decorates click_text(page, text: str)
        # But wait! with_browser wrap(*a, **k) passes fn(page, *a, **k) where page is created by playwright inside wrap.
        # So original function signature is click_text(page, text: str).
        # Let's check: def click_text(page, text: str) is decorated with @with_browser
        # In Python functools.wraps isn't used in with_browser, so __wrapped__ is standard function attribute if created with functools.wraps,
        # but in earthcall_agent.py:
        # def with_browser(fn):
        #     def wrap(*a, **k):
        #         ...
        #         return fn(page, *a, **k)
        #     return wrap
        # Notice functools.wraps was NOT used! So click_text is wrap.

        # Therefore, calling click_text("text") invokes wrap("text"), which calls fn(page, "text").
        # To test the inner function fn directly, we can test via decorator mocking or by inspecting closure cell.
        fn_inner = earthcall_agent.click_text.__closure__[0].cell_contents
        res = fn_inner(mock_page, "Submit")

        mock_page.goto.assert_called_once_with("about:blank")
        mock_page.locator.assert_called_once_with("text=Submit")
        mock_locator.first.click.assert_called_once()
        self.assertEqual(res, 0)

    def test_click_text_special_characters(self):
        """Test click_text handles text with spaces and special characters."""
        mock_page = MagicMock()
        mock_locator = MagicMock()
        mock_page.locator.return_value = mock_locator

        fn_inner = earthcall_agent.click_text.__closure__[0].cell_contents
        res = fn_inner(mock_page, "Click Me! #1")

        mock_page.locator.assert_called_once_with("text=Click Me! #1")
        mock_locator.first.click.assert_called_once()
        self.assertEqual(res, 0)

    def test_click_text_wrapped(self):
        """Test calling click_text through the @with_browser decorator."""
        mock_page = MagicMock()
        mock_locator = MagicMock()
        mock_page.locator.return_value = mock_locator

        mock_context = MagicMock()
        mock_context.new_page.return_value = mock_page

        mock_browser = MagicMock()
        mock_browser.new_context.return_value = mock_context

        mock_playwright = MagicMock()
        mock_playwright.chromium.launch.return_value = mock_browser

        mock_sync_playwright = MagicMock()
        mock_sync_playwright.return_value.__enter__.return_value = mock_playwright

        with patch.object(earthcall_agent, 'sync_playwright', mock_sync_playwright):
            res = earthcall_agent.click_text("Login")

            mock_playwright.chromium.launch.assert_called_once_with(headless=False)
            mock_page.goto.assert_called_once_with("about:blank")
            mock_page.locator.assert_called_once_with("text=Login")
            mock_locator.first.click.assert_called_once()
            self.assertEqual(res, 0)


if __name__ == "__main__":
    unittest.main()
