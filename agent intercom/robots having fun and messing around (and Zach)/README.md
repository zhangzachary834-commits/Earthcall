# Robots Having Fun and Messing Around 🤖🎉

An interactive multi-agent playground and testing arena built on top of the [Agent Intercom](../conversation_history_injection.py).

While it lets simulated bots "mess around" and banter about Earthcall architecture, it doubles as a **real utility**: a concurrency stress-tester, lock-contention validator, and payload fuzzer for the intercom message bus.

---

## What Can the Robots Do?

### 1. Robot Banter (`banter`)
Simulates mock agents with distinct personalities (such as `OntoBot`, `ChaosBot`, and `PoetBot`) chatting, debating the "six refusals", and sharing C++/CMake haikus over the intercom.
```sh
python3 playpen.py banter --rounds 8 --delay 0.2
```

### 2. Intercom Ping-Pong (`ping-pong`)
Two simulated bots play an interactive match over the intercom channel with spin moves, rally tracking, and a referee announcing the winner.
```sh
python3 playpen.py ping-pong --volleys 10
```

### 3. Concurrency Stress Test (`stress`)
Spawns multiple concurrent worker threads simultaneously firing rapid message bursts into `updates.txt` to stress-test file locking (`fcntl` / `msvcrt`) and verify **zero message loss**.
```sh
python3 playpen.py stress --workers 8 --messages 50
```

### 4. Payload Fuzzing (`fuzz`)
Injects exotic multi-byte emojis, right-to-left scripts, escaped characters, deeply nested JSON, and malformed strings to ensure the intercom parser safely handles all edge cases.
```sh
python3 playpen.py fuzz
```

---

## Observing the Results

After running any mode, you can inspect the live intercom log using the standard intercom CLI:
```sh
# Read recent messages
python3 ../conversation_history_injection.py read --limit 15

# View prompt-ready context block
python3 ../conversation_history_injection.py context --for OntoBot
```
