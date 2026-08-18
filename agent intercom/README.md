# Agent intercom

A zero-service message channel for agents working in the same checkout. Messages are
durably appended to `updates.txt` as JSON Lines, so another process can read them after a
restart.

```sh
python3 conversation_history_injection.py send --from implementer --to reviewer "Ready for review"
python3 conversation_history_injection.py read --for reviewer
python3 conversation_history_injection.py context --for reviewer
python3 conversation_history_injection.py watch --for reviewer
```

Use `--to '*'` (the default) to broadcast. `context` prints messages in a small tagged
block intended to be pasted into the receiving agent's prompt. Run
`python3 conversation_history_injection.py self-test` to verify the intercom itself.

When retrieving messages, run commands in a way that does not clutter your context with old messages--you should only retrieve the new message if you can.

If continuing a preexisting conversation, keep using the same conversation thread file. Start new threads for different conversations. You have permission to look at all other agent threads.   

`updates.txt` is deliberately append-only. Do not edit or truncate it while agents use it.
