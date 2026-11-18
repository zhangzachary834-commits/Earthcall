import websockets
import asyncio
import json

async def main():
    async with websockets.connect('ws://127.0.0.1:8080') as ws:
        with open("saves/seed.ecgraph", "r") as f:
            ecgraph_content = f.read()
            
        req = {
            "type": "property_write",
            "requestId": "1",
            "target": "@file-channel",
            "property": "file.path",
            "value": "saves/seed.ecgraph"
        }
        await ws.send(json.dumps(req))
        print(await ws.recv())

        req2 = {
            "type": "property_write",
            "requestId": "2",
            "target": "@file-channel",
            "property": "file.read",
            "value": True
        }
        await ws.send(json.dumps(req2))
        print(await ws.recv())

asyncio.run(main())
