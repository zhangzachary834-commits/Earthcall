import asyncio
import websockets
import json

async def test():
    async with websockets.connect("ws://127.0.0.1:8080") as ws:
        await ws.recv() # wait for initial sync
        cmd = {"type": "switch_zone", "name": "Cathedral of the Living Logos"}
        await ws.send(json.dumps(cmd))
        print("Sent switch_zone")
        while True:
            resp = await ws.recv()
            data = json.loads(resp)
            if data.get("type") == "state_sync":
                print("Received state_sync, active_zone_name:", data.get("active_zone_name"))
                break
            else:
                print("Received:", data.get("type"))

asyncio.run(test())
