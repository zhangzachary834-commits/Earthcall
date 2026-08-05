import asyncio
import websockets
import json
import threading

class EngineServer:
    def __init__(self, host="127.0.0.1", port=5001):
        self.host = host
        self.port = port
        self.connected_clients = set()
        self.loop = None

    async def handler(self, websocket, path):
        print(f"[EngineServer] New C++ Engine connected from {websocket.remote_address}")
        self.connected_clients.add(websocket)
        try:
            async for message in websocket:
                # In the future, this is where we will ingest high-frequency binary state
                # or JSON updates from the C++ Physics/Law engine.
                try:
                    data = json.loads(message)
                    print(f"[EngineServer] Received: {data}")
                    
                    # Echo back for testing
                    await websocket.send(json.dumps({"status": "received", "echo": data}))
                except json.JSONDecodeError:
                    print(f"[EngineServer] Received binary/invalid payload of length {len(message)}")
        except websockets.exceptions.ConnectionClosed:
            pass
        finally:
            print(f"[EngineServer] Engine disconnected {websocket.remote_address}")
            self.connected_clients.remove(websocket)

    async def _start_server(self):
        print(f"[EngineServer] Starting Raw WebSocket Server on {self.host}:{self.port}")
        async with websockets.serve(self.handler, self.host, self.port):
            await asyncio.Future()  # run forever

    def run_in_background(self):
        def _run():
            self.loop = asyncio.new_event_loop()
            asyncio.set_event_loop(self.loop)
            self.loop.run_until_complete(self._start_server())
            
        thread = threading.Thread(target=_run, daemon=True)
        thread.start()
        return thread

def start_engine_server(host="127.0.0.1", port=5001):
    server = EngineServer(host, port)
    server.run_in_background()
    return server
