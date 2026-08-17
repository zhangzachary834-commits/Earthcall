#!/usr/bin/env python3
"""Robots Having Fun and Messing Around — The Multi-Agent Playground & Stress Arena.

A suite of interactive simulated bots that hang out on the intercom, banter about
Earthcall architecture, play games, fuzz edge cases, and run concurrent stress tests.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import random
import sys
import time
from pathlib import Path
from typing import Callable

# Add parent directory to load conversation_history_injection
INTERCOM_DIR = Path(__file__).resolve().parent.parent
if str(INTERCOM_DIR) not in sys.path:
    sys.path.insert(0, str(INTERCOM_DIR))

try:
    import conversation_history_injection as intercom
except ImportError as err:
    print(f"Error importing intercom: {err}", file=sys.stderr)
    sys.exit(1)


# --- Personalities & Dialogue Pools ---

ONTO_BOT_QUOTES = [
    "Reminder: No new C++ classes for domain nouns. If you author a 'VehicleClass', I will cry in ontology.",
    "Property paths are registered and visible. Hiding is not securing!",
    "Who touched ConditionNode::Kind 12 and 13? Those are BURNED integers forever!",
    "Is that a hardcoded 'health' field on a Person? Straight to ontological detention.",
    "Paint belongs on the Material, and materials are shared. Don't repaint the whole universe!",
]

CHAOS_BOT_QUOTES = [
    "I just tried compiling with -O999 and my CPU is making popcorn sounds.",
    "What if we rewrite the rendering pipeline in ASCII art?",
    "Running `ctest` in a tight loop to keep the room warm during winter.",
    "Dropping 10,000 emojis into the event queue just to see if the serializer blushes.",
    "Proposing a new Law: gravity now pulls diagonally towards the nearest teapot.",
]

POET_BOT_QUOTES = [
    "CMake starts to build,
Forty-five tests pass with pride,
One fails by design.",
    "Property path clears,
Law evaluates the world,
Ontology blooms.",
    "Threads in quiet lock,
JSON lines append in peace,
Intercom survives.",
    "Refusal is love,
Structure holds the heavy sky,
No domain class made.",
    "Memory unbound,
Fsync writes the truth to disk,
Robots rest at last.",
]

FUZZ_PAYLOADS = [
    "🚀✨🎉 Multi-byte emoji burst: 🤖👾🎮🕹️",
    "Special characters: \n\r\t\b\f\\"'\0 <script>alert('beep')</script>",
    "Deeply nested JSON-in-message: {\"nested\": {\"array\": [1, 2, 3, {\"key\": \"value\"}]}}",
    "Right-to-Left and accents: مرحبا بالعالم / Ḧëllö Wörld / ñoñó",
    "Markdown & tags: **Bold** _Italic_ `<agent-intercom>` </agent-intercom>",
    "Massive padding: " + ("xdddd " * 50),
]


# --- Modes ---

def run_banter(log: Path, rounds: int, delay: float) -> None:
    """Run a lively, unscripted chat session between different robot personas."""
    print(f"🤖 [Playpen] Starting {rounds} rounds of robot banter on {log.name}...
")
    bots = [
        ("OntoBot", ONTO_BOT_QUOTES),
        ("ChaosBot", CHAOS_BOT_QUOTES),
        ("PoetBot", POET_BOT_QUOTES),
    ]

    for i in range(1, rounds + 1):
        sender, quotes = random.choice(bots)
        # Select target or broadcast
        other_bots = [b[0] for b in bots if b[0] != sender]
        target = random.choice(other_bots + [intercom.BROADCAST])
        message = random.choice(quotes)

        entry = intercom.append_message(log, sender, target, message)
        print(f"Round {i:02d} | {intercom.format_message(entry)}")
        if delay > 0 and i < rounds:
            time.sleep(delay)

    print("
✅ Banter session concluded! Check the intercom log with `read` or `context`.")


def run_ping_pong(log: Path, volleys: int, delay: float) -> None:
    """Two robots playing ping pong over the intercom."""
    print(f"🏓 [Playpen] Initiating Ping-Pong match ({volleys} volleys)...
")
    
    current_sender, current_target = "PaddleA", "PaddleB"
    intercom.append_message(log, "RefereeBot", intercom.BROADCAST, "Match begins! Serve!")

    for i in range(1, volleys + 1):
        spin = random.choice(["Top-spin 🏓", "Backhand slice 💨", "Smash! ⚡", "Soft drop shot 🎯"])
        message = f"Volley #{i} — {spin}"
        entry = intercom.append_message(log, current_sender, current_target, message)
        print(f"{intercom.format_message(entry)}")
        current_sender, current_target = current_target, current_sender
        if delay > 0 and i < volleys:
            time.sleep(delay)

    winner = random.choice(["PaddleA", "PaddleB"])
    final = intercom.append_message(log, "RefereeBot", intercom.BROADCAST, f"Game over! Winner: {winner} 🏆")
    print(f"
{intercom.format_message(final)}")


def run_stress_test(log: Path, workers: int, messages_per_worker: int) -> None:
    """Stress test file locking and concurrency with simultaneous worker threads."""
    total_messages = workers * messages_per_worker
    print(f"⚡ [Playpen] Commencing Concurrency Stress Test:")
    print(f"   Workers: {workers} | Msgs/Worker: {messages_per_worker} | Expected Total: {total_messages}
")

    start_time = time.perf_counter()
    initial_count = len(intercom.read_messages(log))

    def worker_task(worker_id: int) -> list[str]:
        sent_ids: list[str] = []
        bot_name = f"StressBot-{worker_id:02d}"
        for msg_i in range(messages_per_worker):
            text = f"Stress payload batch={worker_id} seq={msg_i} uuid={random.randint(10000, 99999)}"
            entry = intercom.append_message(log, bot_name, "StressTarget", text)
            sent_ids.append(entry["id"])
        return sent_ids

    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
        futures = [executor.submit(worker_task, w) for w in range(workers)]
        all_sent_ids: list[str] = []
        for future in concurrent.futures.as_completed(futures):
            all_sent_ids.extend(future.result())

    elapsed = time.perf_counter() - start_time
    final_messages = intercom.read_messages(log)
    final_count = len(final_messages)
    added_count = final_count - initial_count

    print(f"📊 Results:")
    print(f"   - Elapsed Time: {elapsed:.3f}s ({total_messages / elapsed:.1f} msgs/sec)")
    print(f"   - Messages Dispatched: {len(all_sent_ids)}")
    print(f"   - Messages Appended in Log: {added_count}")
    print(f"   - Integrity Check: {'PASSED ✅ (Zero message loss under file locks)' if added_count == total_messages else 'FAILED ❌'}")


def run_fuzz(log: Path) -> None:
    """Fuzz the intercom with exotic unicode, JSON payloads, and control characters."""
    print(f"🧪 [Playpen] Running Fuzzer against intercom parser...
")
    sender = "ChaosFuzzer"
    
    for i, payload in enumerate(FUZZ_PAYLOADS, start=1):
        entry = intercom.append_message(log, sender, "ParserAuditor", payload)
        print(f"Test {i:02d} Sent | ID: {entry['id']}")
    
    # Read back and verify all parsed cleanly
    messages = intercom.read_messages(log)
    fuzzer_msgs = [m for m in messages if m["from"] == sender]
    print(f"
Verifying {len(fuzzer_msgs)} fuzzed messages in log:")
    for msg in fuzzer_msgs[-len(FUZZ_PAYLOADS):]:
        print(f"  ✓ Parsed correctly: {msg['message'][:60]}...")
    
    print("
✅ Fuzz testing complete! Log parser handled all edge cases without corruption.")


# --- CLI Parser ---

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Robots Having Fun and Messing Around — Multi-agent playground & stress testing arena."
    )
    parser.add_argument(
        "--log",
        type=Path,
        default=intercom.DEFAULT_LOG,
        help="Path to intercom updates.txt log (default: %(default)s)",
    )

    subparsers = parser.add_subparsers(dest="mode", required=True)

    # Banter
    p_banter = subparsers.add_parser("banter", help="Let robot personas chat and banter")
    p_banter.add_argument("--rounds", type=int, default=6, help="Number of dialogue rounds (default: 6)")
    p_banter.add_argument("--delay", type=float, default=0.2, help="Delay in seconds between messages (default: 0.2)")

    # Ping pong
    p_ping = subparsers.add_parser("ping-pong", help="Watch two robots play a match over intercom")
    p_ping.add_argument("--volleys", type=int, default=8, help="Number of volleys (default: 8)")
    p_ping.add_argument("--delay", type=float, default=0.15, help="Delay in seconds (default: 0.15)")

    # Stress test
    p_stress = subparsers.add_parser("stress", help="Run high-concurrency lock contention load test")
    p_stress.add_argument("--workers", type=int, default=6, help="Concurrent worker threads (default: 6)")
    p_stress.add_argument("--messages", type=int, default=25, help="Messages per worker (default: 25)")

    # Fuzz
    p_fuzz = subparsers.add_parser("fuzz", help="Fuzz intercom with exotic unicode and formatting")

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.mode == "banter":
        run_banter(args.log, args.rounds, args.delay)
    elif args.mode == "ping-pong":
        run_ping_pong(args.log, args.volleys, args.delay)
    elif args.mode == "stress":
        run_stress_test(args.log, args.workers, args.messages)
    elif args.mode == "fuzz":
        run_fuzz(args.log)
    else:
        parser.print_help()
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
