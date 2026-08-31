import os
from flask import Blueprint, jsonify, request, current_app
from pathlib import Path

api_bp = Blueprint('api', __name__, url_prefix='/api')

def get_bridge():
    """Access the shared CppBridge instance from current_app or module."""
    return current_app.config.get('CPP_BRIDGE')

@api_bp.route('/status', methods=['GET'])
def get_status():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"connected": False, "status": "Bridge not initialized"}), 503
    return jsonify(bridge.get_status())

@api_bp.route('/state', methods=['GET'])
def get_state():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
    return jsonify(bridge.get_state())

@api_bp.route('/utterance', methods=['POST'])
def emit_utterance():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
    
    data = request.get_json(silent=True) or {}
    text = data.get('text', data.get('payload', '')).strip()
    if not text:
        return jsonify({"error": "Text payload is required"}), 400
        
    source = data.get('source', 'web_ui')
    target = data.get('target', '')
    
    success = bridge.emit_utterance(text, source, target)
    return jsonify({"status": "success" if success else "queued_or_fallback", "text": text})

@api_bp.route('/objects', methods=['GET'])
def list_objects():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
    state = bridge.get_state()
    return jsonify(state.get('objects', []))

@api_bp.route('/objects', methods=['POST'])
def spawn_object():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    data = request.get_json(silent=True) or {}
    shape = data.get('shape', data.get('shapeKind', 'Cube'))
    position = data.get('position', [0.0, 1.0, 0.0])
    color = data.get('color', [0.2, 0.7, 1.0])
    name = data.get('name', '')
    dimensions = float(data.get('dimensions', 1.0))
    material_id = data.get('materialId', 'material.default')
    
    success = bridge.spawn_object(
        shape=shape,
        position=position,
        color=color,
        name=name,
        dimensions=dimensions,
        material_id=material_id
    )
    return jsonify({"status": "ok", "success": success})

@api_bp.route('/objects/<object_id>', methods=['PUT'])
def update_object(object_id):
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    data = request.get_json(silent=True) or {}
    success = bridge.update_object(object_id, data)
    return jsonify({"status": "ok", "id": object_id, "success": success})

@api_bp.route('/objects/<object_id>', methods=['DELETE'])
def delete_object(object_id):
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    success = bridge.delete_object(object_id)
    return jsonify({"status": "ok", "id": object_id, "success": success})

@api_bp.route('/properties/write', methods=['POST'])
def write_property():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    data = request.get_json(silent=True) or {}
    target = data.get('target', '')
    prop = data.get('property', '')
    value = data.get('value')
    
    if not target or not prop or value is None:
        return jsonify({"error": "target, property, and value are required"}), 400
        
    success = bridge.write_property(target, prop, value)
    return jsonify({"status": "ok", "success": success})

@api_bp.route('/zones', methods=['GET'])
def list_zones():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
    state = bridge.get_state()
    return jsonify({
        "active_zone_index": state.get('active_zone_index', 0),
        "active_zone_name": state.get('active_zone_name', ''),
        "zones": state.get('zones', [])
    })

@api_bp.route('/zones/switch', methods=['POST'])
def switch_zone():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    data = request.get_json(silent=True) or {}
    index = data.get('index')
    name = data.get('name')
    
    success = bridge.switch_zone(index=index, name=name)
    return jsonify({"status": "ok", "success": success})

@api_bp.route('/zones', methods=['POST'])
def create_zone():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    data = request.get_json(silent=True) or {}
    name = data.get('name', 'New Zone')
    kind = data.get('kind', 'zone')
    
    success = bridge.create_zone(name=name, kind=kind)
    return jsonify({"status": "ok", "success": success})

@api_bp.route('/laws', methods=['GET'])
def list_laws():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
    state = bridge.get_state()
    return jsonify(state.get('laws', []))

@api_bp.route('/laws/<law_id>/toggle', methods=['POST'])
def toggle_law(law_id):
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    data = request.get_json(silent=True) or {}
    enabled = data.get('enabled', True)
    
    success = bridge.toggle_law(law_id, enabled)
    return jsonify({"status": "ok", "identifier": law_id, "enabled": enabled, "success": success})

@api_bp.route('/player/teleport', methods=['POST'])
def teleport_player():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    data = request.get_json(silent=True) or {}
    pos = data.get('position', [0.0, 1.8, 0.0])
    
    success = bridge.teleport_player(pos)
    return jsonify({"status": "ok", "position": pos, "success": success})

@api_bp.route('/physics', methods=['POST'])
def update_physics():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    data = request.get_json(silent=True) or {}
    flying = data.get('flying')
    gravity_viz = data.get('gravity_viz')
    
    success = bridge.set_physics(flying=flying, gravity_viz=gravity_viz)
    return jsonify({"status": "ok", "success": success})

@api_bp.route('/world/save', methods=['POST'])
def save_world():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    data = request.get_json(silent=True) or {}
    name = data.get('name', '')
    
    success = bridge.save_world(name=name)
    return jsonify({"status": "ok", "name": name, "success": success})

@api_bp.route('/saves', methods=['GET'])
def list_saves():
    repo_root = Path(__file__).resolve().parents[4]
    saves_dir = repo_root / "saves" / "worlds"
    
    save_files = []
    if saves_dir.exists():
        for f in saves_dir.glob("*.json"):
            save_files.append({
                "filename": f.name,
                "path": str(f),
                "size_kb": round(f.stat().st_size / 1024, 2),
                "modified": f.stat().st_mtime
            })
            
    return jsonify({"saves": save_files})

@api_bp.route('/command', methods=['POST'])
def execute_raw_command():
    bridge = get_bridge()
    if not bridge:
        return jsonify({"error": "Bridge not initialized"}), 503
        
    payload = request.get_json(silent=True) or {}
    success = bridge.send_command(payload)
    return jsonify({"status": "ok", "success": success})
