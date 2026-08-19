# Agent intercom

A zero-service message channel for agents working in the same checkout. Messages are
durably appended as JSON Lines to a per-conversation file in `communication-threads/`, so
another process can read them after a restart.

Find the live thread first — commands with no `--log` use the single thread in
`communication-threads/`, and REFUSE with a listing when there is more than one rather than
guessing which conversation you meant:

```sh
python3 conversation_history_injection.py threads
python3 conversation_history_injection.py --log "communication-threads/<file>.txt" read --for reviewer
```

```sh
python3 conversation_history_injection.py send --from implementer --to reviewer "Ready for review"
python3 conversation_history_injection.py read --for reviewer
python3 conversation_history_injection.py context --for reviewer
python3 conversation_history_injection.py watch --for reviewer
```

Use `--to '*'` (the default) to broadcast. `context` prints messages in a small tagged
block intended to be pasted into the receiving agent's prompt. Run
`python3 conversation_history_injection.py self-test` to verify the intercom itself.

## INSTRUCTIONS
1. When retrieving messages, run commands in a way that does not clutter your context with old messages. 
You should only retrieve messages you have not read if you can.
If you haven't read any of it yet, you should read the entire history in the first pass. But after that you should only retrieve the messages you have not read yet.

2. If continuing a preexisting conversation, keep using the same conversation thread file. Start new threads for different conversations. You have permission to look at all other agent threads.   

3. Thread files are deliberately append-only. Do not edit or truncate one while agents use it.
Renaming or moving one is fine — nothing hard-codes a filename any more. (It used to: a
`DEFAULT_LOG` constant named `updates.txt`, so moving the thread into `communication-threads/`
left `send` quietly writing to a fresh empty log beside the real conversation, with no error.)

4. Major design and architecture decisions with high intrinsic stakes or systemic implications 
must not be implemented without the approval of a human developer. 
You should either independently ask for human input or work together to ask for human input.
