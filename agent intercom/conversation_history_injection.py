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


THREADS_DIR = Path(__file__).with_name("communication-threads")

# One file per conversation, in communication-threads/. There is deliberately
# no fixed default filename: `updates.txt` used to be one, and when the thread
# was renamed and moved into that directory the constant went on pointing at a
# path that no longer existed -- so a `send` with no --log would have quietly
# created a fresh empty log beside the real conversation and dropped the
# message into it. Nobody would have seen an error.
#
# So: resolve at call time, and REFUSE when the answer is ambiguous rather than
# guessing which conversation an agent meant. `threads` lists the candidates.
def default_log() -> Path:
    if not THREADS_DIR.is_dir():
        return THREADS_DIR / "general.txt"
    candidates = sorted(THREADS_DIR.glob("*.txt"))
    if len(candidates) == 1:
        return candidates[0]
    if not candidates:
        return THREADS_DIR / "general.txt"
    raise ValueError(
        "several conversation threads exist; name one with --log.\n  "
        + "\n  ".join(str(c) for c in candidates)
    )
BROADCAST = "*"


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds").replace("+00:00", "Z")


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


def append_message(
    log: Path,
    sender: str,
    recipient: str,
    body: str,
    thread: str | None = None,
) -> dict[str, str]:
    sender, recipient = normalise_name(sender), normalise_name(recipient)
    body = body.strip()
    if not body:
        raise ValueError("message text cannot be empty")

    entry: dict[str, str] = {
        "id": uuid.uuid4().hex,
        "at": utc_now(),
        "from": sender,
        "to": recipient,
        "message": body,
    }
    if thread:
        entry["thread"] = thread.strip()

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
            message_dict: dict[str, str] = {key: str(value[key]) for key in ("id", "at", "from", "to", "message")}
            if "thread" in value:
                message_dict["thread"] = str(value["thread"])
            messages.append(message_dict)
    return messages


def addressed_to(
    messages: Iterable[dict[str, str]],
    recipient: str | None = None,
    since: str | None = None,
    after_id: str | None = None,
    sender: str | None = None,
    thread: str | None = None,
) -> list[dict[str, str]]:
    msg_list = list(messages)
    
    # Handle after_id cursor slice if specified
    if after_id:
        found_idx = -1
        for idx, msg in enumerate(msg_list):
            if msg["id"] == after_id:
                found_idx = idx
                break
        if found_idx != -1:
            msg_list = msg_list[found_idx + 1 :]

    selected: list[dict[str, str]] = []
    for message in msg_list:
        if recipient and message["to"] not in (recipient, BROADCAST):
            continue
        if sender and message["from"] != sender:
            continue
        if thread and message.get("thread") != thread:
            continue
        if since and message["at"] <= since:
            continue
        selected.append(message)
    return selected


def format_message(message: dict[str, str]) -> str:
    target = "everyone" if message["to"] == BROADCAST else message["to"]
    thread_suffix = f" (thread: {message['thread']})" if "thread" in message else ""
    return f"[{message['at']}] {message['from']} → {target}{thread_suffix}: {message['message']}"


def print_messages(messages: list[dict[str, str]], as_json: bool) -> None:
    if as_json:
        print(json.dumps(messages, ensure_ascii=False, indent=2))
        return
    for message in messages:
        print(format_message(message))


def resolve_message_body(args: argparse.Namespace) -> str:
    if getattr(args, "file", None):
        return args.file.read_text(encoding="utf-8")
    if args.message == "-" or (args.message is None and not sys.stdin.isatty()):
        return sys.stdin.read()
    if args.message:
        return args.message
    raise ValueError("message text required (provide string, '--file <path>', or '-' via stdin)")


def cmd_send(args: argparse.Namespace) -> int:
    body = resolve_message_body(args)
    thread = getattr(args, "thread", None)
    entry = append_message(args.log, args.sender, args.recipient, body, thread=thread)
    print(f"sent {entry['id']} to {entry['to']}")
    return 0


def cmd_read(args: argparse.Namespace) -> int:
    messages = addressed_to(
        read_messages(args.log),
        recipient=args.recipient,
        since=args.since,
        after_id=getattr(args, "after_id", None),
        sender=getattr(args, "sender", None),
        thread=getattr(args, "thread", None),
    )
    if args.limit is not None:
        messages = messages[-args.limit :]
    print_messages(messages, args.json)
    return 0


def cmd_context(args: argparse.Namespace) -> int:
    recipient = normalise_name(args.recipient)
    messages = addressed_to(
        read_messages(args.log),
        recipient=recipient,
        since=args.since,
        after_id=getattr(args, "after_id", None),
        sender=getattr(args, "sender", None),
        thread=getattr(args, "thread", None),
    )
    if args.limit is not None:
        messages = messages[-args.limit :]
    print(f'<agent-intercom recipient="{recipient}">')
    if messages:
        print_messages(messages, False)
    else:
        print("(no messages)")
    print("</agent-intercom>")
    return 0


def cmd_watch(args: argparse.Namespace) -> int:
    recipient = normalise_name(args.recipient) if args.recipient else None
    sender = normalise_name(args.sender) if getattr(args, "sender", None) else None
    thread = getattr(args, "thread", None)
    seen = {message["id"] for message in read_messages(args.log)}
    print("watching; press Ctrl-C to stop", file=sys.stderr)
    try:
        while True:
            for message in addressed_to(
                read_messages(args.log),
                recipient=recipient,
                sender=sender,
                thread=thread,
            ):
                if message["id"] not in seen:
                    print(format_message(message), flush=True)
                    seen.add(message["id"])
            time.sleep(args.interval)
    except KeyboardInterrupt:
        return 0


def cmd_self_test(_: argparse.Namespace) -> int:
    with tempfile.TemporaryDirectory() as directory:
        log = Path(directory) / "updates.txt"
        m1 = append_message(log, "builder", "reviewer", "ready for review", thread="feature-x")
        m2 = append_message(log, "builder", BROADCAST, "tests pass")
        m3 = append_message(log, "reviewer", "builder", "acknowledged", thread="feature-x")
        
        # Recipient filtering
        reviewer_messages = addressed_to(read_messages(log), recipient="reviewer")
        assert [m["message"] for m in reviewer_messages] == ["ready for review", "tests pass"]
        assert len(read_messages(log)) == 3
        
        # After-ID cursor test
        after_m1 = addressed_to(read_messages(log), after_id=m1["id"])
        assert [m["id"] for m in after_m1] == [m2["id"], m3["id"]]

        # Thread filtering test
        thread_messages = addressed_to(read_messages(log), thread="feature-x")
        assert [m["id"] for m in thread_messages] == [m1["id"], m3["id"]]

        # Sender filtering test
        from_reviewer = addressed_to(read_messages(log), sender="reviewer")
        assert len(from_reviewer) == 1 and from_reviewer[0]["id"] == m3["id"]

    print("intercom self-test passed")
    return 0


def parser() -> argparse.ArgumentParser:
    command = argparse.ArgumentParser(description=__doc__)
    command.add_argument("--log", type=Path, default=None,
                         help="JSONL thread file (default: the single thread in "
                              "communication-threads/, refusing if there are several)")
    subcommands = command.add_subparsers(dest="command", required=True)

    send = subcommands.add_parser("send", help="append one message")
    send.add_argument("--from", dest="sender", required=True, help="sending agent")
    send.add_argument("--to", dest="recipient", default=BROADCAST, help="recipient, or * for everyone")
    send.add_argument("--thread", dest="thread", help="optional thread/topic identifier")
    send.add_argument("--file", type=Path, help="read message body from a file")
    send.add_argument("message", nargs="?", default=None, help="message text (or '-' / pipe via stdin)")
    send.set_defaults(func=cmd_send)

    read = subcommands.add_parser("read", help="read messages")
    read.add_argument("--for", dest="recipient", help="show messages for this recipient and broadcasts")
    read.add_argument("--from", dest="sender", help="filter messages from a specific sender")
    read.add_argument("--thread", dest="thread", help="filter messages by thread/topic")
    read.add_argument("--since", help="strict ISO-8601 timestamp cursor")
    read.add_argument("--after-id", dest="after_id", help="show messages after this message ID")
    read.add_argument("--limit", type=int, help="keep the newest N messages")
    read.add_argument("--json", action="store_true", help="emit a JSON array")
    read.set_defaults(func=cmd_read)

    context = subcommands.add_parser("context", help="emit recipient-filtered prompt context")
    context.add_argument("--for", dest="recipient", required=True, help="receiving agent")
    context.add_argument("--from", dest="sender", help="filter messages from a specific sender")
    context.add_argument("--thread", dest="thread", help="filter messages by thread/topic")
    context.add_argument("--since", help="strict ISO-8601 timestamp cursor")
    context.add_argument("--after-id", dest="after_id", help="show messages after this message ID")
    context.add_argument("--limit", type=int, default=30, help="keep the newest N messages (default: 30)")
    context.set_defaults(func=cmd_context)

    watch = subcommands.add_parser("watch", help="print newly appended messages")
    watch.add_argument("--for", dest="recipient", help="only show one recipient and broadcasts")
    watch.add_argument("--from", dest="sender", help="filter messages from a specific sender")
    watch.add_argument("--thread", dest="thread", help="filter messages by thread/topic")
    watch.add_argument("--interval", type=float, default=0.5, help="poll seconds (default: 0.5)")
    watch.set_defaults(func=cmd_watch)

    threads = subcommands.add_parser("threads", help="list conversation threads")
    threads.set_defaults(func=cmd_threads)

    self_test = subcommands.add_parser("self-test", help="exercise send, filtering, and persistence")
    self_test.set_defaults(func=cmd_self_test)
    return command


def cmd_threads(args: argparse.Namespace) -> int:
    """List the conversation threads, so an agent can find the live one."""
    if not THREADS_DIR.is_dir():
        print(f"no {THREADS_DIR.name}/ directory yet")
        return 0
    found = sorted(THREADS_DIR.glob("*.txt"))
    if not found:
        print(f"no threads in {THREADS_DIR.name}/")
        return 0
    for path in found:
        messages = read_messages(path)
        last = messages[-1]["at"] if messages else "-"
        print(f"{len(messages):4d} msg  last {last}  {path.name}")
    return 0


def main() -> int:
    args = parser().parse_args()
    try:
        # `threads` and `self-test` do not read the conversation log, and
        # `threads` is the command you run precisely BECAUSE the default is
        # ambiguous -- resolving it first made the disambiguator refuse to run
        # whenever there was something to disambiguate.
        if getattr(args, "log", None) is None and args.command not in ("threads", "self-test"):
            args.log = default_log()
        return args.func(args)
    except (OSError, ValueError) as error:
        print(f"intercom: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
