#!/usr/bin/env python3
"""A small, local intercom for agents sharing one workspace.

Messages are appended to ``updates.txt`` as JSON Lines. No service needs to be
running, so the channel survives terminal restarts and works from separate
processes. ``context`` emits only messages addressed to an agent (plus
broadcasts), wrapped so its output can be pasted directly into a prompt.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import tempfile
import time
import uuid
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable


DEFAULT_LOG = Path(__file__).with_name("updates.txt")
BROADCAST = "*"


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")


def normalise_name(value: str) -> str:
    name = value.strip()
    if not name:
        raise ValueError("agent names cannot be empty")
    return name


def lock_file(handle: Any) -> None:
    """Lock one append on POSIX and Windows without a package dependency."""
    if os.name == "nt":
        import msvcrt

        handle.seek(0, os.SEEK_END)
        msvcrt.locking(handle.fileno(), msvcrt.LK_LOCK, 1)
    else:
        import fcntl

        fcntl.flock(handle.fileno(), fcntl.LOCK_EX)


def unlock_file(handle: Any) -> None:
    if os.name == "nt":
        import msvcrt

        handle.seek(0, os.SEEK_END)
        msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
    else:
        import fcntl

        fcntl.flock(handle.fileno(), fcntl.LOCK_UN)


def append_message(log: Path, sender: str, recipient: str, body: str) -> dict[str, str]:
    sender, recipient = normalise_name(sender), normalise_name(recipient)
    body = body.strip()
    if not body:
        raise ValueError("message text cannot be empty")

    entry = {
        "id": uuid.uuid4().hex,
        "at": utc_now(),
        "from": sender,
        "to": recipient,
        "message": body,
    }
    log.parent.mkdir(parents=True, exist_ok=True)
    encoded = json.dumps(entry, ensure_ascii=False, separators=(",", ":")) + "\n"
    with log.open("a+", encoding="utf-8") as handle:
        lock_file(handle)
        try:
            handle.write(encoded)
            handle.flush()
            os.fsync(handle.fileno())
        finally:
            unlock_file(handle)
    return entry


def read_messages(log: Path) -> list[dict[str, str]]:
    if not log.exists():
        return []

    messages: list[dict[str, str]] = []
    with log.open("r", encoding="utf-8") as handle:
        for line_number, line in enumerate(handle, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                value = json.loads(line)
            except json.JSONDecodeError as error:
                raise ValueError(f"{log}:{line_number} is not a JSON intercom message: {error.msg}") from error
            if not isinstance(value, dict) or not all(key in value for key in ("id", "at", "from", "to", "message")):
                raise ValueError(f"{log}:{line_number} is missing required intercom fields")
            messages.append({key: str(value[key]) for key in ("id", "at", "from", "to", "message")})
    return messages


def addressed_to(messages: Iterable[dict[str, str]], recipient: str | None, since: str | None) -> list[dict[str, str]]:
    selected: list[dict[str, str]] = []
    for message in messages:
        if recipient and message["to"] not in (recipient, BROADCAST):
            continue
        if since and message["at"] <= since:
            continue
        selected.append(message)
    return selected


def format_message(message: dict[str, str]) -> str:
    target = "everyone" if message["to"] == BROADCAST else message["to"]
    return f"[{message['at']}] {message['from']} → {target}: {message['message']}"


def print_messages(messages: list[dict[str, str]], as_json: bool) -> None:
    if as_json:
        print(json.dumps(messages, ensure_ascii=False, indent=2))
        return
    for message in messages:
        print(format_message(message))


def cmd_send(args: argparse.Namespace) -> int:
    entry = append_message(args.log, args.sender, args.recipient, args.message)
    print(f"sent {entry['id']} to {entry['to']}")
    return 0


def cmd_read(args: argparse.Namespace) -> int:
    messages = addressed_to(read_messages(args.log), args.recipient, args.since)
    if args.limit is not None:
        messages = messages[-args.limit :]
    print_messages(messages, args.json)
    return 0


def cmd_context(args: argparse.Namespace) -> int:
    recipient = normalise_name(args.recipient)
    messages = addressed_to(read_messages(args.log), recipient, args.since)
    if args.limit is not None:
        messages = messages[-args.limit :]
    print(f"<agent-intercom recipient=\"{recipient}\">")
    if messages:
        print_messages(messages, False)
    else:
        print("(no messages)")
    print("</agent-intercom>")
    return 0


def cmd_watch(args: argparse.Namespace) -> int:
    recipient = normalise_name(args.recipient) if args.recipient else None
    seen = {message["id"] for message in read_messages(args.log)}
    print("watching; press Ctrl-C to stop", file=sys.stderr)
    try:
        while True:
            for message in addressed_to(read_messages(args.log), recipient, None):
                if message["id"] not in seen:
                    print(format_message(message), flush=True)
                    seen.add(message["id"])
            time.sleep(args.interval)
    except KeyboardInterrupt:
        return 0


def cmd_self_test(_: argparse.Namespace) -> int:
    with tempfile.TemporaryDirectory() as directory:
        log = Path(directory) / "updates.txt"
        append_message(log, "builder", "reviewer", "ready for review")
        append_message(log, "builder", BROADCAST, "tests pass")
        append_message(log, "reviewer", "builder", "acknowledged")
        reviewer_messages = addressed_to(read_messages(log), "reviewer", None)
        assert [message["message"] for message in reviewer_messages] == ["ready for review", "tests pass"]
        assert len(read_messages(log)) == 3
    print("intercom self-test passed")
    return 0


def parser() -> argparse.ArgumentParser:
    command = argparse.ArgumentParser(description=__doc__)
    command.add_argument("--log", type=Path, default=DEFAULT_LOG, help="JSONL log path (default: %(default)s)")
    subcommands = command.add_subparsers(dest="command", required=True)

    send = subcommands.add_parser("send", help="append one message")
    send.add_argument("--from", dest="sender", required=True, help="sending agent")
    send.add_argument("--to", dest="recipient", default=BROADCAST, help="recipient, or * for everyone")
    send.add_argument("message", help="message text")
    send.set_defaults(func=cmd_send)

    read = subcommands.add_parser("read", help="read messages")
    read.add_argument("--for", dest="recipient", help="show messages for this recipient and broadcasts")
    read.add_argument("--since", help="strict ISO-8601 timestamp cursor")
    read.add_argument("--limit", type=int, help="keep the newest N messages")
    read.add_argument("--json", action="store_true", help="emit a JSON array")
    read.set_defaults(func=cmd_read)

    context = subcommands.add_parser("context", help="emit recipient-filtered prompt context")
    context.add_argument("--for", dest="recipient", required=True, help="receiving agent")
    context.add_argument("--since", help="strict ISO-8601 timestamp cursor")
    context.add_argument("--limit", type=int, default=30, help="keep the newest N messages (default: 30)")
    context.set_defaults(func=cmd_context)

    watch = subcommands.add_parser("watch", help="print newly appended messages")
    watch.add_argument("--for", dest="recipient", help="only show one recipient and broadcasts")
    watch.add_argument("--interval", type=float, default=0.5, help="poll seconds (default: 0.5)")
    watch.set_defaults(func=cmd_watch)

    self_test = subcommands.add_parser("self-test", help="exercise send, filtering, and persistence")
    self_test.set_defaults(func=cmd_self_test)
    return command


def main() -> int:
    args = parser().parse_args()
    try:
        return args.func(args)
    except (OSError, ValueError) as error:
        print(f"intercom: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
