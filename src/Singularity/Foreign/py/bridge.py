import json
import threading
import time
import os
from typing import Dict, Any, Optional
from websockets.sync.client import connect
from websockets.exceptions import ConnectionClosed, WebSocketException

class CppBridge:
    """
    Bidirectional WebSocket Bridge between Python backend (5005) and C++ Earthcall Engine (8080).
    Maintains auto-reconnection, state caching, and thread-safe messaging.
    """
    def __init__(self, host: str = "127.0.0.1", port: int = 8080):
        self.host = host
        self.port = port
        self.url = f"ws://{host}:{port}"
        
        self.ws = None
        self.connected = False
        self.running = True
        self.lock = threading.Lock()
        self.worker_thread: Optional[threading.Thread] = None
        
        # In-memory cached snapshot of world state
        self.current_state: Dict[str, Any] = self._create_initial_state()
        
        # Event callbacks
        self.on_state_sync_callbacks = []
        self.on_event_callbacks = []
        
        # Statistics
        self.last_connected_time = None
        self.messages_sent = 0
        self.messages_received = 0

    def _create_initial_state(self) -> Dict[str, Any]:
        return {
            "type": "state_sync",
            "timestamp": time.time(),
            "engine_connected": False,
            "active_zone_index": 0,
            "active_zone_name": "Sanctum of Beginnings",
            "active_zone_id": "zone-sanctum",
            "active_zone_owner": "Player",
            "zones": [
                {"index": 0, "name": "Sanctum of Beginnings", "identifier": "zone-sanctum", "owner": "Player", "object_count": 3, "scope": "Local"},
                {"index": 1, "name": "Temple of Echoes", "identifier": "zone-temple", "owner": "Player", "object_count": 0, "scope": "Local"},
                {"index": 2, "name": "Cavern of Light", "identifier": "zone-cavern", "owner": "Player", "object_count": 0, "scope": "Local"},
                {"index": 3, "name": "Character Architect Forge", "identifier": "zone-forge", "owner": "Player", "object_count": 0, "scope": "Local"},
                {"index": 4, "name": "Home (Player)", "identifier": "home-player", "owner": "Player", "object_count": 0, "scope": "Local"}
            ],
            "objects": [
                {
                    "id": "obj_cube_origin",
                    "name": "Genesis Cube",
                    "type": "ConstructedBeing",
                    "shapeKind": 0,
                    "spatialKind": 0,
                    "position": [0.0, 1.0, 0.0],
                    "rotation": [0.0, 0.0, 0.0],
                    "dimensions": 1,
                    "materialId": "material.default",
                    "color": [0.2, 0.7, 1.0],
                    "renderMode": 0
                },
                {
                    "id": "obj_sphere_light",
                    "name": "Orb of Light",
                    "type": "ConstructedBeing",
                    "shapeKind": 2,
                    "spatialKind": 1,
                    "position": [3.0, 2.5, -2.0],
                    "rotation": [0.0, 0.0, 0.0],
                    "dimensions": 1,
                    "materialId": "material.clay",
                    "color": [1.0, 0.8, 0.2],
                    "renderMode": 0
                },
                {
                    "id": "obj_torus_portal",
                    "name": "Sanctum Ring",
                    "type": "ConstructedBeing",
                    "shapeKind": 8,
                    "spatialKind": 1,
                    "position": [-3.0, 2.0, 2.0],
                    "rotation": [45.0, 30.0, 0.0],
                    "dimensions": 2,
                    "materialId": "material.default",
                    "color": [0.8, 0.2, 0.9],
                    "renderMode": 0
                }
            ],
            "player": {
                "name": "Player",
                "position": [0.0, 1.8, 5.0],
                "camera_forward": [0.0, 0.0, -1.0],
                "camera_position": [0.0, 1.8, 5.0],
                "yaw": -90.0,
                "pitch": 0.0,
                "flying": False
            },
            "laws": [
                {"identifier": "physics-gravity", "name": "Default Gravity Field", "enabled": True, "activation": 0, "expression": "gravity(0, -9.81, 0)"},
                {"identifier": "physics-kinematics", "name": "Classical Kinematics", "enabled": True, "activation": 0, "expression": "integrate(velocity, dt)"},
                {"identifier": "physics-acoustics", "name": "Collision Acoustics", "enabled": True, "activation": 1, "expression": "onEvent('contact-began') -> sound()"},
                {"identifier": "shape-generator-3d", "name": "Shape Generator 3D", "enabled": True, "activation": 1, "expression": "onEvent('onMouseClicked') -> spawn()"}
            ],
            "physics": {
                "flying": False,
                "gravity_viz": False,
                "legacy_engine": False
            }
        }

    def start(self):
        self.worker_thread = threading.Thread(target=self._connection_loop, daemon=True)
        self.worker_thread.start()
        print(f"[CppBridge] Initialized C++ Engine link targeting {self.url}")

    def stop(self):
        self.running = False
        with self.lock:
            if self.ws:
                try:
                    self.ws.close()
                except Exception:
                    pass

    def _connection_loop(self):
        while self.running:
            try:
                with connect(self.url, open_timeout=2, close_timeout=2) as ws:
                    with self.lock:
                        self.ws = ws
                        self.connected = True
                        self.last_connected_time = time.time()
                        self.current_state["engine_connected"] = True
                    
                    print(f"[CppBridge] Connected to C++ Engine at {self.url}")
                    
                    try:
                        ws.send(json.dumps({"type": "get_state"}))
                    except Exception:
                        pass

                    for message in ws:
                        if not self.running:
                            break
                        self._process_message(message)
                        
            except Exception:
                pass
            
            with self.lock:
                self.ws = None
                self.connected = False
                self.current_state["engine_connected"] = False
            
            time.sleep(2)

    def _process_message(self, message: str):
        self.messages_received += 1
        try:
            data = json.loads(message)
            msg_type = data.get("type", "")
            
            if msg_type == "state_sync":
                with self.lock:
                    self.current_state = data
                    self.current_state["engine_connected"] = True
                    self.current_state["last_sync_time"] = time.time()
                
                for cb in list(self.on_state_sync_callbacks):
                    try:
                        cb(self.current_state)
                    except Exception as e:
                        print(f"[CppBridge] State callback error: {e}")
                        
            elif msg_type == "engine_event" or "event" in data:
                for cb in list(self.on_event_callbacks):
                    try:
                        cb(data)
                    except Exception as e:
                        print(f"[CppBridge] Event callback error: {e}")
                        
        except Exception as e:
            print(f"[CppBridge] Error parsing incoming message: {e}")

    def send_command(self, payload: Dict[str, Any]) -> bool:
        with self.lock:
            self.messages_sent += 1
            if not self.connected or not self.ws:
                self._apply_fallback_state_update(payload)
                return False
                
            try:
                msg_str = json.dumps(payload)
                self.ws.send(msg_str)
                return True
            except Exception as e:
                print(f"[CppBridge] Send error: {e}")
                self.connected = False
                return False

    def _apply_fallback_state_update(self, payload: Dict[str, Any]):
        msg_type = payload.get("type", "")
        
        if msg_type == "spawn_object":
            shape_str = payload.get("shape", "Cube")
            new_obj = {
                "id": payload.get("name", f"obj_{int(time.time()*1000)%10000}"),
                "name": payload.get("name", f"Authored {shape_str}"),
                "type": "ConstructedBeing",
                "shapeKind": payload.get("shapeKindInt", 0),
                "spatialKind": 0,
                "position": payload.get("position", [0.0, 1.0, 0.0]),
                "rotation": payload.get("rotation", [0.0, 0.0, 0.0]),
                "dimensions": payload.get("dimensions", 1),
                "materialId": payload.get("materialId", "material.default"),
                "color": payload.get("color", [0.2, 0.7, 1.0]),
                "renderMode": 0
            }
            self.current_state.setdefault("objects", []).append(new_obj)
            
        elif msg_type == "delete_object":
            obj_id = payload.get("id", "")
            self.current_state["objects"] = [o for o in self.current_state.get("objects", []) if o.get("id") != obj_id and o.get("name") != obj_id]
            
        elif msg_type == "transform_object" or msg_type == "update_object":
            obj_id = payload.get("id", "")
            for o in self.current_state.get("objects", []):
                if o.get("id") == obj_id or o.get("name") == obj_id:
                    if "position" in payload: o["position"] = payload["position"]
                    if "rotation" in payload: o["rotation"] = payload["rotation"]
                    if "color" in payload: o["color"] = payload["color"]
                    if "dimensions" in payload: o["dimensions"] = payload["dimensions"]
                    if "shape" in payload: o["shapeKind"] = payload.get("shapeKindInt", 0)
                    if "materialId" in payload: o["materialId"] = payload["materialId"]
                    break
                    
        elif msg_type == "toggle_law":
            law_id = payload.get("identifier", "")
            enabled = payload.get("enabled", True)
            for l in self.current_state.get("laws", []):
                if l.get("identifier") == law_id:
                    l["enabled"] = enabled
                    break
                    
        elif msg_type == "switch_zone":
            idx = payload.get("index", 0)
            if 0 <= idx < len(self.current_state.get("zones", [])):
                self.current_state["active_zone_index"] = idx
                self.current_state["active_zone_name"] = self.current_state["zones"][idx]["name"]
                
        elif msg_type == "teleport_player":
            if "position" in payload and "player" in self.current_state:
                self.current_state["player"]["position"] = payload["position"]
                
        elif msg_type == "set_physics":
            if "flying" in payload and "physics" in self.current_state:
                self.current_state["physics"]["flying"] = payload["flying"]
            if "gravity_viz" in payload and "physics" in self.current_state:
                self.current_state["physics"]["gravity_viz"] = payload["gravity_viz"]

        for cb in list(self.on_state_sync_callbacks):
            try:
                cb(self.current_state)
            except Exception:
                pass

    def emit_utterance(self, text: str, source_client: str = "web_ui", target_id: str = ""):
        return self.send_command({
            "type": "utterance",
            "payload": text,
            "sourceClient": source_client,
            "targetSingularId": target_id
        })

    def write_property(self, target: str, prop: str, value: Any):
        return self.send_command({
            "type": "property_write",
            "target": target,
            "property": prop,
            "value": value
        })

    def spawn_object(self, shape: str, position: list, color: list, name: str = "", dimensions: float = 1.0, material_id: str = "material.default"):
        return self.send_command({
            "type": "spawn_object",
            "shape": shape,
            "position": position,
            "color": color,
            "name": name,
            "dimensions": dimensions,
            "materialId": material_id
        })

    def update_object(self, obj_id: str, data: Dict[str, Any]):
        cmd = {"type": "transform_object", "id": obj_id}
        cmd.update(data)
        return self.send_command(cmd)

    def delete_object(self, obj_id: str):
        return self.send_command({
            "type": "delete_object",
            "id": obj_id
        })

    def toggle_law(self, identifier: str, enabled: bool):
        return self.send_command({
            "type": "toggle_law",
            "identifier": identifier,
            "enabled": enabled
        })

    def switch_zone(self, index: int = None, name: str = None):
        cmd = {"type": "switch_zone"}
        if index is not None: cmd["index"] = int(index)
        if name is not None: cmd["name"] = str(name)
        return self.send_command(cmd)

    def create_zone(self, name: str, kind: str = "zone"):
        return self.send_command({
            "type": "create_zone",
            "name": name,
            "kind": kind
        })

    def teleport_player(self, position: list):
        return self.send_command({
            "type": "teleport_player",
            "position": position
        })

    def set_physics(self, flying: bool = None, gravity_viz: bool = None):
        cmd = {"type": "set_physics"}
        if flying is not None: cmd["flying"] = flying
        if gravity_viz is not None: cmd["gravity_viz"] = gravity_viz
        return self.send_command(cmd)

    def save_world(self, name: str = ""):
        return self.send_command({
            "type": "quick_save",
            "name": name
        })

    def get_state(self) -> Dict[str, Any]:
        with self.lock:
            state = dict(self.current_state)
            state["engine_connected"] = self.connected
            return state

    def get_status(self) -> Dict[str, Any]:
        return {
            "connected": self.connected,
            "engine_url": self.url,
            "messages_sent": self.messages_sent,
            "messages_received": self.messages_received,
            "uptime": (time.time() - self.last_connected_time) if self.last_connected_time and self.connected else 0
        }
