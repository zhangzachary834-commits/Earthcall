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
                    
                    if data.get("type") == "request_ai_action":
                        context = data.get("context", "")
                        target_id = data.get("target_singular_id", "unknown_object")
                        
                        # Start streaming task
                        asyncio.create_task(self.stream_ai_action(websocket, context, target_id))
                    else:
                        # Echo back for testing
                        await websocket.send(json.dumps({"status": "received", "echo": data}))
                except json.JSONDecodeError:
                    print(f"[EngineServer] Received binary/invalid payload of length {len(message)}")
        except websockets.exceptions.ConnectionClosed:
            pass
        finally:
            print(f"[EngineServer] Engine disconnected {websocket.remote_address}")
            self.connected_clients.remove(websocket)

    async def stream_ai_action(self, websocket, context, target_id):
        # We assume ai_service is available in the module path (started from app.py)
        try:
            from api.ai_service import generate_utterance
            
            async for chunk in generate_utterance(context, target_id):
                payload = {
                    "type": "ai_utterance_stream",
                    "target_singular_id": target_id,
                    "text": chunk
                }
                try:
                    await websocket.send(json.dumps(payload))
                except websockets.exceptions.ConnectionClosed:
                    break
        except Exception as e:
            print(f"[EngineServer] AI stream error: {e}")

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

    def broadcast(self, payload: dict):
        if not self.loop or not self.connected_clients:
            return
            
        message = json.dumps(payload)
        
        async def _send_all():
            for ws in self.connected_clients:
                try:
                    await ws.send(message)
                except websockets.exceptions.ConnectionClosed:
                    pass
                    
        asyncio.run_coroutine_threadsafe(_send_all(), self.loop)

def start_engine_server(host="127.0.0.1", port=5001):
    server = EngineServer(host, port)
    server.run_in_background()
    return server
