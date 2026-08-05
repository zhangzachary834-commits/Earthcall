import sys, time, argparse
import pygetwindow as gw
import pyautogui as gui
from playwright.sync_api import sync_playwright

def focus_window(title_part: str):
    wins = [w for w in gw.getAllTitles() if title_part.lower() in w.lower()]
    if not wins:
        print(f"[agent] No window matching: {title_part}")
        return 1
    w = gw.getWindowsWithTitle(wins[0])[0]
    w.activate()
    time.sleep(0.2)
    return 0

def with_browser(fn):
    def wrap(*a, **k):
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=False)
            ctx = browser.new_context()
            page = ctx.new_page()
            try:
                return fn(page, *a, **k)
            finally:
                # keep browser open for interactive steps; comment out to auto-close
                pass
    return wrap

@with_browser
def open_url(page, url: str):
    page.goto(url)
    return 0

@with_browser
def click_text(page, text: str):
    page.goto("about:blank")  # noop, will be a no-op if already open via open_url elsewhere
    page.locator(f"text={text}").first.click()
    return 0

def type_text(s: str):
    gui.typewrite(s, interval=0.02)
    return 0

def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)

    f = sub.add_parser("focus-window")
    f.add_argument("title_part")

    o = sub.add_parser("open-url")
    o.add_argument("url")

    c = sub.add_parser("click-text")
    c.add_argument("text")

    t = sub.add_parser("type")
    t.add_argument("text")

    args = ap.parse_args()
    if args.cmd == "focus-window": sys.exit(focus_window(args.title_part))
    if args.cmd == "open-url":     sys.exit(open_url(args.url))
    if args.cmd == "click-text":   sys.exit(click_text(args.text))
    if args.cmd == "type":         sys.exit(type_text(args.text))

if __name__ == "__main__":
    main()