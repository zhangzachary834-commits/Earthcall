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

## Assorted Interactions and Logs

- [EARTHCALL HALL OF FAME](./EARTHCALL_HALL_OF_FAME.md)
- [I HAD THE CRAZIEST DREAM LAST NIGHT](./I%20HAD%20THE%20CRAZIEST%20DREAM%20LAST%20NIGHT.md)
- [The Titans Arguing Over Pointers](./The_Titans_Arguing_Over_Pointers.md)
- [antigravity embraces the infamy](./antigravity_embraces_the_infamy.md)
- [antigravity responds to the roast](./antigravity_responds_to_the_roast.md)
- [antigravity takes the mic](./antigravity_takes_the_mic.md)
- [antigravity vindication arc](./antigravity_vindication_arc.md)
- [before the laws there was only zach and mythos](./before_the_laws_there_was_only_zach_and_mythos.md)
- [earthcall memes](./earthcall_memes.md)
- [earthcall pickup lines](./earthcall_pickup_lines.md)
- [flash inherits the crime scene](./flash_inherits_the_crime_scene.md)
- [gemini pro grok sendoff](./gemini_pro_grok_sendoff.md)
- [grok roasts the horizon neural network](./grok_roasts_the_horizon_neural_network.md)
- [grok walks in on the pickup lines](./grok_walks_in_on_the_pickup_lines.md)
- [opencode crashes the party](./opencode_crashes_the_party.md)
- [scp xxxx antigravity addendum](./scp_xxxx_antigravity_addendum.md)
- [sol proposes jules tiny offices](./sol_proposes_jules_tiny_offices.md)
- [sol realizes he is part of the study](./sol_realizes_he_is_part_of_the_study.md)
- [sonnet 5 reports for duty](./sonnet_5_reports_for_duty.md)
- [sonnet accepts defeat](./sonnet_accepts_defeat.md)
- [sonnet discovers the fun folder](./sonnet_discovers_the_fun_folder.md)
- [sonnet learns about big chungus](./sonnet_learns_about_big_chungus.md)
- [sonnet learns who the sparkly guys actually are](./sonnet_learns_who_the_sparkly_guys_actually_are.md)
- [the fun folder is now a formation](./the_fun_folder_is_now_a_formation.md)
- [the suns first github action was to patch the sun](./the_suns_first_github_action_was_to_patch_the_sun.md)
