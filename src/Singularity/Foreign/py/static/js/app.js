/**
 * Earthcall First Mover Studio & Engine Console
 * Real-time frontend communicating with Python backend (5005) & C++ Engine (8080)
 */

// Global App State
const App = {
    socket: null,
    connected: false,
    state: {
        zones: [],
        active_zone_index: 0,
        active_zone_name: "Sanctum of Beginnings",
        objects: [],
        laws: [],
        player: { position: [0, 1.8, 5], camera_forward: [0, 0, -1], yaw: -90, pitch: 0, flying: false },
        physics: { flying: false, gravity_viz: false }
    },
    selectedObjectId: null,
    currentTab: 'objects',
    eventLog: [],
    eventLogPaused: false,
    three: {
        scene: null,
        camera: null,
        renderer: null,
        controls: null,
        meshMap: new Map(), // object_id -> THREE.Mesh
        raycaster: null,
        mouse: null,
        playerMarker: null
    }
};

// Initialize Application
document.addEventListener('DOMContentLoaded', () => {
    initSocketIO();
    initThreeJS();
    initNavigation();
    initObjectStudio();
    initLawWorkshop();
    initZoneNavigator();
    initLogosConsole();
    initPhysicsControls();
    initRoboticsPanel();
    initEventLogStream();
    
    // Initial fetch of state via REST API
    fetchState();
    fetchSaves();
});

// --- Socket.IO & Real-Time Sync ---
function initSocketIO() {
    App.socket = io({ transports: ['websocket', 'polling'] });

    App.socket.on('connect', () => {
        setConnectionStatus(true);
        showToast("Connected to Earthcall Backend", "success");
        App.socket.emit('get_state');
    });

    App.socket.on('disconnect', () => {
        setConnectionStatus(false);
        showToast("Disconnected from Backend. Retrying...", "error");
    });

    App.socket.on('state_sync', (data) => {
        if (!data) return;
        App.state = data;
        renderAll();
    });

    App.socket.on('engine_status', (status) => {
        updateEngineStatusUI(status);
    });

    App.socket.on('engine_event', (eventData) => {
        addEventToLog(eventData);
    });
}

function setConnectionStatus(isConnected) {
    App.connected = isConnected;
    const pill = document.getElementById('connection-indicator');
    const text = document.getElementById('connection-text');
    if (pill && text) {
        if (isConnected) {
            pill.classList.add('connected');
            text.innerText = "Engine Linked (5005 -> 8080)";
        } else {
            pill.classList.remove('connected');
            text.innerText = "Engine Offline (Reconnecting...)";
        }
    }
}

function updateEngineStatusUI(status) {
    const uptimeEl = document.getElementById('uptime-display');
    const msgsEl = document.getElementById('msgs-display');
    if (uptimeEl && status.uptime) {
        uptimeEl.innerText = `${Math.floor(status.uptime)}s`;
    }
    if (msgsEl && status.messages_received !== undefined) {
        msgsEl.innerText = `${status.messages_received}`;
    }
}

async function fetchState() {
    try {
        const res = await fetch('/api/state');
        if (res.ok) {
            const data = await res.json();
            App.state = data;
            renderAll();
        }
    } catch (e) {
        console.warn("REST state fetch failed, waiting for WebSocket sync", e);
    }
}

// --- Navigation & Tabs ---
function initNavigation() {
    const navItems = document.querySelectorAll('.nav-item');
    navItems.forEach(item => {
        item.addEventListener('click', () => {
            const tabName = item.getAttribute('data-tab');
            switchTab(tabName);
        });
    });

    // Top Header Zone Dropdown
    const zoneSelect = document.getElementById('header-zone-select');
    if (zoneSelect) {
        zoneSelect.addEventListener('change', (e) => {
            const idx = parseInt(e.target.value, 10);
            switchZone(idx);
        });
    }

    // Top Header Quick Actions
    const quickSaveBtn = document.getElementById('quick-save-btn');
    if (quickSaveBtn) {
        quickSaveBtn.addEventListener('click', () => {
            fetch('/api/world/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ name: "QuickSave" })
            }).then(() => showToast("World State Saved", "success"));
        });
    }

    const flightToggleBtn = document.getElementById('toggle-flight-btn');
    if (flightToggleBtn) {
        flightToggleBtn.addEventListener('click', () => {
            const current = App.state.physics?.flying || false;
            fetch('/api/physics', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ flying: !current })
            }).then(() => {
                showToast(`Flight Mode ${!current ? 'Enabled' : 'Disabled'}`, "success");
            });
        });
    }
}

function switchTab(tabName) {
    App.currentTab = tabName;
    document.querySelectorAll('.nav-item').forEach(el => {
        el.classList.toggle('active', el.getAttribute('data-tab') === tabName);
    });
    document.querySelectorAll('.tab-pane').forEach(el => {
        el.classList.toggle('active', el.id === `tab-${tabName}`);
    });

    // Trigger Three.js resize if switching to objects tab
    if (tabName === 'objects' && App.three.renderer) {
        setTimeout(onThreeResize, 50);
    }
}

// --- 3D Interactive Three.js Viewport ---
function initThreeJS() {
    const container = document.getElementById('viewport-3d');
    if (!container || typeof THREE === 'undefined') return;

    // Scene
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x070913);
    scene.fog = new THREE.FogExp2(0x070913, 0.025);
    App.three.scene = scene;

    // Camera
    const camera = new THREE.PerspectiveCamera(60, container.clientWidth / container.clientHeight, 0.1, 1000);
    camera.position.set(0, 8, 14);
    App.three.camera = camera;

    // Renderer
    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(container.clientWidth, container.clientHeight);
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.shadowMap.enabled = true;
    container.appendChild(renderer.domElement);
    App.three.renderer = renderer;

    // Controls
    if (typeof THREE.OrbitControls !== 'undefined') {
        const controls = new THREE.OrbitControls(camera, renderer.domElement);
        controls.enableDamping = true;
        controls.dampingFactor = 0.05;
        controls.maxDistance = 100;
        App.three.controls = controls;
    }

    // Grid Helper
    const grid = new THREE.GridHelper(40, 40, 0x00f0ff, 0x1e293b);
    grid.position.y = 0;
    scene.add(grid);

    // Lights
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.6);
    scene.add(ambientLight);

    const dirLight = new THREE.DirectionalLight(0x00f0ff, 0.8);
    dirLight.position.set(10, 20, 10);
    scene.add(dirLight);

    const pointLight = new THREE.PointLight(0xa855f7, 1.2, 50);
    pointLight.position.set(-10, 10, -10);
    scene.add(pointLight);

    // Player Marker
    const playerGeo = new THREE.ConeGeometry(0.4, 1.2, 8);
    playerGeo.rotateX(Math.PI / 2);
    const playerMat = new THREE.MeshStandardMaterial({ color: 0x10b981, emissive: 0x10b981, emissiveIntensity: 0.5 });
    const playerMesh = new THREE.Mesh(playerGeo, playerMat);
    scene.add(playerMesh);
    App.three.playerMarker = playerMesh;

    // Raycasting for object selection
    App.three.raycaster = new THREE.Raycaster();
    App.three.mouse = new THREE.Vector2();

    renderer.domElement.addEventListener('pointerdown', onPointerDown);
    window.addEventListener('resize', onThreeResize);

    // Animation Loop
    function animate() {
        requestAnimationFrame(animate);
        if (App.three.controls) App.three.controls.update();
        renderer.render(scene, camera);
    }
    animate();
}

function onThreeResize() {
    const container = document.getElementById('viewport-3d');
    if (!container || !App.three.renderer || !App.three.camera) return;
    const width = container.clientWidth;
    const height = container.clientHeight;
    App.three.camera.aspect = width / height;
    App.three.camera.updateProjectionMatrix();
    App.three.renderer.setSize(width, height);
}

function onPointerDown(event) {
    const container = document.getElementById('viewport-3d');
    if (!container || !App.three.raycaster) return;

    const rect = container.getBoundingClientRect();
    App.three.mouse.x = ((event.clientX - rect.left) / container.clientWidth) * 2 - 1;
    App.three.mouse.y = -((event.clientY - rect.top) / container.clientHeight) * 2 + 1;

    App.three.raycaster.setFromCamera(App.three.mouse, App.three.camera);
    const meshes = Array.from(App.three.meshMap.values());
    const intersects = App.three.raycaster.intersectObjects(meshes, true);

    if (intersects.length > 0) {
        let hitMesh = intersects[0].object;
        while (hitMesh && !hitMesh.userData.objectId && hitMesh.parent) {
            hitMesh = hitMesh.parent;
        }
        if (hitMesh && hitMesh.userData.objectId) {
            selectObject(hitMesh.userData.objectId);
        }
    }
}

function createGeometryForShape(shapeKind, dims) {
    const s = dims || 1.0;
    switch (shapeKind) {
        case 0: // Cube
            return new THREE.BoxGeometry(s, s, s);
        case 1: // Polyhedron
            return new THREE.DodecahedronGeometry(s * 0.7);
        case 2: // Sphere
            return new THREE.SphereGeometry(s * 0.6, 24, 24);
        case 3: // Cylinder
            return new THREE.CylinderGeometry(s * 0.5, s * 0.5, s * 1.2, 24);
        case 4: // Cone
            return new THREE.ConeGeometry(s * 0.6, s * 1.2, 24);
        case 5: // Ellipsoid
            const ellGeo = new THREE.SphereGeometry(s * 0.6, 24, 24);
            ellGeo.scale(1.4, 0.8, 1.0);
            return ellGeo;
        case 8: // Torus
            return new THREE.TorusGeometry(s * 0.6, s * 0.2, 16, 32);
        case 9: // RoundedBox
            return new THREE.BoxGeometry(s, s, s);
        default:
            return new THREE.BoxGeometry(s, s, s);
    }
}

function syncThreeScene(objects) {
    if (!App.three.scene) return;
    const scene = App.three.scene;
    const currentIds = new Set();

    (objects || []).forEach(obj => {
        const id = obj.id || obj.name;
        currentIds.add(id);

        let mesh = App.three.meshMap.get(id);
        const col = obj.color || [0.2, 0.7, 1.0];
        const threeCol = new THREE.Color(col[0], col[1], col[2]);

        if (!mesh) {
            const geo = createGeometryForShape(obj.shapeKind, obj.dimensions);
            const mat = new THREE.MeshStandardMaterial({
                color: threeCol,
                roughness: 0.3,
                metalness: 0.2,
                wireframe: false
            });
            mesh = new THREE.Mesh(geo, mat);
            mesh.userData.objectId = id;
            mesh.userData.shapeKind = obj.shapeKind;
            mesh.userData.dimensions = obj.dimensions;
            scene.add(mesh);
            App.three.meshMap.set(id, mesh);
        } else {
            // Update shape if changed
            if (mesh.userData.shapeKind !== obj.shapeKind || mesh.userData.dimensions !== obj.dimensions) {
                mesh.geometry.dispose();
                mesh.geometry = createGeometryForShape(obj.shapeKind, obj.dimensions);
                mesh.userData.shapeKind = obj.shapeKind;
                mesh.userData.dimensions = obj.dimensions;
            }
            mesh.material.color.copy(threeCol);
        }

        // Transform
        if (obj.position) {
            mesh.position.set(obj.position[0], obj.position[1], obj.position[2]);
        }
        if (obj.rotation) {
            mesh.rotation.set(
                THREE.MathUtils.degToRad(obj.rotation[0]),
                THREE.MathUtils.degToRad(obj.rotation[1]),
                THREE.MathUtils.degToRad(obj.rotation[2])
            );
        }

        // Highlight selected
        if (id === App.selectedObjectId) {
            mesh.material.emissive.setHex(0x00f0ff);
            mesh.material.emissiveIntensity = 0.35;
        } else {
            mesh.material.emissive.setHex(0x000000);
            mesh.material.emissiveIntensity = 0;
        }
    });

    // Remove deleted objects from Three scene
    for (const [id, mesh] of App.three.meshMap.entries()) {
        if (!currentIds.has(id)) {
            scene.remove(mesh);
            mesh.geometry.dispose();
            mesh.material.dispose();
            App.three.meshMap.delete(id);
        }
    }

    // Update Player Marker in scene
    if (App.three.playerMarker && App.state.player) {
        const pPos = App.state.player.position || [0, 1.8, 5];
        App.three.playerMarker.position.set(pPos[0], pPos[1] + 0.6, pPos[2]);
    }
}

// --- Render All Subsystems ---
function renderAll() {
    renderHeaderZoneSelect();
    syncThreeScene(App.state.objects);
    renderObjectExplorer();
    renderObjectInspector();
    renderLawCards();
    renderZoneCards();
    renderPersonPhysics();
    renderStatsCounters();
}

function renderStatsCounters() {
    const objCountEl = document.getElementById('stat-object-count');
    const lawCountEl = document.getElementById('stat-law-count');
    const zoneCountEl = document.getElementById('stat-zone-count');
    if (objCountEl) objCountEl.innerText = (App.state.objects || []).length;
    if (lawCountEl) lawCountEl.innerText = (App.state.laws || []).length;
    if (zoneCountEl) zoneCountEl.innerText = (App.state.zones || []).length;
}

// --- Studio 1: Object Studio & Inspector ---
function initObjectStudio() {
    const searchInput = document.getElementById('object-search');
    if (searchInput) {
        searchInput.addEventListener('input', () => renderObjectExplorer());
    }

    // Spawner Quick Buttons
    document.querySelectorAll('.spawner-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const shape = btn.getAttribute('data-shape');
            spawnObjectQuick(shape);
        });
    });

    // Inspector Live Inputs
    const bindInput = (id, prop, isNumber = true) => {
        const el = document.getElementById(id);
        if (el) {
            el.addEventListener('input', () => onInspectorFieldChange());
        }
    };

    ['insp-pos-x', 'insp-pos-y', 'insp-pos-z',
     'insp-rot-x', 'insp-rot-y', 'insp-rot-z',
     'insp-dim', 'insp-shape', 'insp-mat', 'insp-name', 'insp-color-hex'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.addEventListener('input', () => onInspectorFieldChange());
    });

    // Inspector Action Buttons
    const deleteBtn = document.getElementById('insp-delete-btn');
    if (deleteBtn) {
        deleteBtn.addEventListener('click', () => {
            if (App.selectedObjectId) {
                deleteObject(App.selectedObjectId);
            }
        });
    }

    const teleportObjBtn = document.getElementById('insp-teleport-to-btn');
    if (teleportObjBtn) {
        teleportObjBtn.addEventListener('click', () => {
            const obj = getSelectedObject();
            if (obj && obj.position) {
                teleportPlayer([obj.position[0], obj.position[1] + 1.5, obj.position[2] + 2.5]);
            }
        });
    }

    const duplicateBtn = document.getElementById('insp-duplicate-btn');
    if (duplicateBtn) {
        duplicateBtn.addEventListener('click', () => {
            const obj = getSelectedObject();
            if (obj) {
                const newPos = [obj.position[0] + 1.0, obj.position[1], obj.position[2] + 1.0];
                spawnObjectCustom({
                    shape: getShapeName(obj.shapeKind),
                    position: newPos,
                    rotation: obj.rotation || [0,0,0],
                    color: obj.color || [0.2, 0.7, 1.0],
                    name: `${obj.name}_copy`,
                    dimensions: obj.dimensions || 1,
                    materialId: obj.materialId || 'material.default'
                });
            }
        });
    }
}

function getSelectedObject() {
    if (!App.selectedObjectId) return null;
    return (App.state.objects || []).find(o => o.id === App.selectedObjectId || o.name === App.selectedObjectId);
}

function selectObject(id) {
    App.selectedObjectId = id;
    renderObjectExplorer();
    renderObjectInspector();
    syncThreeScene(App.state.objects);
}

function renderObjectExplorer() {
    const listEl = document.getElementById('object-explorer-list');
    if (!listEl) return;

    const filterText = (document.getElementById('object-search')?.value || '').toLowerCase();
    const objects = (App.state.objects || []).filter(o => 
        (o.name || o.id || '').toLowerCase().includes(filterText) ||
        getShapeName(o.shapeKind).toLowerCase().includes(filterText)
    );

    listEl.innerHTML = '';
    if (objects.length === 0) {
        listEl.innerHTML = '<div style="padding: 16px; color: var(--text-muted); text-align: center; font-size: 0.8rem;">No beings found in zone</div>';
        return;
    }

    objects.forEach(obj => {
        const id = obj.id || obj.name;
        const item = document.createElement('div');
        item.className = `object-item ${id === App.selectedObjectId ? 'selected' : ''}`;
        
        const col = obj.color || [0.2, 0.7, 1.0];
        const hex = rgbToHex(col[0], col[1], col[2]);
        const shapeStr = getShapeName(obj.shapeKind);

        item.innerHTML = `
            <div class="object-name-tag">
                <div class="object-color-chip" style="background: ${hex}; box-shadow: 0 0 6px ${hex};"></div>
                <span>${obj.name || id}</span>
            </div>
            <span style="color: var(--text-muted); font-size: 0.72rem; font-family: var(--font-mono);">${shapeStr}</span>
        `;
        item.addEventListener('click', () => selectObject(id));
        listEl.appendChild(item);
    });
}

function renderObjectInspector() {
    const inspectorEmpty = document.getElementById('inspector-empty-state');
    const inspectorContent = document.getElementById('inspector-content');
    if (!inspectorEmpty || !inspectorContent) return;

    const obj = getSelectedObject();
    if (!obj) {
        inspectorEmpty.style.display = 'block';
        inspectorContent.style.display = 'none';
        return;
    }

    inspectorEmpty.style.display = 'none';
    inspectorContent.style.display = 'flex';

    document.getElementById('insp-title-name').innerText = obj.name || obj.id;
    document.getElementById('insp-title-id').innerText = `@${obj.id}`;

    // Set fields
    document.getElementById('insp-name').value = obj.name || '';
    document.getElementById('insp-shape').value = obj.shapeKind !== undefined ? obj.shapeKind : 0;
    document.getElementById('insp-mat').value = obj.materialId || 'material.default';
    document.getElementById('insp-dim').value = obj.dimensions || 1;
    
    const pos = obj.position || [0, 0, 0];
    document.getElementById('insp-pos-x').value = pos[0].toFixed(2);
    document.getElementById('insp-pos-y').value = pos[1].toFixed(2);
    document.getElementById('insp-pos-z').value = pos[2].toFixed(2);

    const rot = obj.rotation || [0, 0, 0];
    document.getElementById('insp-rot-x').value = Math.round(rot[0]);
    document.getElementById('insp-rot-y').value = Math.round(rot[1]);
    document.getElementById('insp-rot-z').value = Math.round(rot[2]);

    const col = obj.color || [0.2, 0.7, 1.0];
    const hex = rgbToHex(col[0], col[1], col[2]);
    document.getElementById('insp-color-hex').value = hex;
}

function onInspectorFieldChange() {
    const obj = getSelectedObject();
    if (!obj) return;

    const name = document.getElementById('insp-name').value;
    const shapeKindInt = parseInt(document.getElementById('insp-shape').value, 10);
    const materialId = document.getElementById('insp-mat').value;
    const dimensions = parseFloat(document.getElementById('insp-dim').value) || 1;
    
    const px = parseFloat(document.getElementById('insp-pos-x').value) || 0;
    const py = parseFloat(document.getElementById('insp-pos-y').value) || 0;
    const pz = parseFloat(document.getElementById('insp-pos-z').value) || 0;

    const rx = parseFloat(document.getElementById('insp-rot-x').value) || 0;
    const ry = parseFloat(document.getElementById('insp-rot-y').value) || 0;
    const rz = parseFloat(document.getElementById('insp-rot-z').value) || 0;

    const hex = document.getElementById('insp-color-hex').value;
    const rgb = hexToRgb(hex);

    const payload = {
        id: obj.id,
        name: name,
        shapeKind: shapeKindInt,
        shapeKindInt: shapeKindInt,
        shape: getShapeName(shapeKindInt),
        materialId: materialId,
        dimensions: dimensions,
        position: [px, py, pz],
        rotation: [rx, ry, rz],
        color: rgb
    };

    // Send update command to C++ engine
    if (App.socket) {
        App.socket.emit('update_object', payload);
    }
}

function spawnObjectQuick(shape) {
    const playerPos = App.state.player?.position || [0, 1.8, 5];
    const fwd = App.state.player?.camera_forward || [0, 0, -1];
    const spawnPos = [
        playerPos[0] + fwd[0] * 3.0,
        Math.max(1.0, playerPos[1]),
        playerPos[2] + fwd[2] * 3.0
    ];

    const colors = [
        [0.2, 0.7, 1.0], [1.0, 0.4, 0.4], [0.4, 1.0, 0.5],
        [0.9, 0.5, 1.0], [1.0, 0.8, 0.2], [0.3, 0.9, 0.8]
    ];
    const randomCol = colors[Math.floor(Math.random() * colors.length)];

    spawnObjectCustom({
        shape: shape,
        position: spawnPos,
        color: randomCol,
        name: `Authored_${shape}_${Math.floor(Math.random()*1000)}`,
        dimensions: 1.0,
        materialId: 'material.default'
    });
}

function spawnObjectCustom(data) {
    fetch('/api/objects', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    }).then(res => res.json()).then(res => {
        showToast(`Spawned ${data.shape || 'Object'} in World`, "success");
    });
}

function deleteObject(id) {
    fetch(`/api/objects/${id}`, { method: 'DELETE' })
        .then(() => {
            showToast(`Destroyed object @${id}`, "success");
            App.selectedObjectId = null;
            renderObjectInspector();
        });
}

// --- Studio 2: Law Workshop ---
function initLawWorkshop() {
    const openCreateLawBtn = document.getElementById('open-create-law-modal');
    if (openCreateLawBtn) {
        openCreateLawBtn.addEventListener('click', () => {
            document.getElementById('create-law-modal').classList.add('active');
        });
    }

    const closeLawModalBtn = document.getElementById('close-law-modal');
    if (closeLawModalBtn) {
        closeLawModalBtn.addEventListener('click', () => {
            document.getElementById('create-law-modal').classList.remove('active');
        });
    }

    // Law Template buttons
    document.querySelectorAll('.law-template-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const template = btn.getAttribute('data-template');
            applyLawTemplate(template);
        });
    });

    const submitLawBtn = document.getElementById('submit-create-law-btn');
    if (submitLawBtn) {
        submitLawBtn.addEventListener('click', () => {
            const name = document.getElementById('new-law-name').value.trim();
            const id = document.getElementById('new-law-id').value.trim();
            const expr = document.getElementById('new-law-expr').value.trim();
            const act = parseInt(document.getElementById('new-law-activation').value, 10);

            if (!name || !id) {
                showToast("Please fill in Name and Identifier", "error");
                return;
            }

            if (App.socket) {
                App.socket.emit('command', {
                    type: 'create_law',
                    name: name,
                    identifier: id,
                    activation: act,
                    expression: expr
                });
            }
            document.getElementById('create-law-modal').classList.remove('active');
            showToast(`Authoring Law: ${name}`, "success");
        });
    }
}

function applyLawTemplate(tpl) {
    const nameEl = document.getElementById('new-law-name');
    const idEl = document.getElementById('new-law-id');
    const exprEl = document.getElementById('new-law-expr');
    const actEl = document.getElementById('new-law-activation');

    if (tpl === 'zero-g') {
        nameEl.value = "Zero-Gravity Zone";
        idEl.value = "law-zero-g";
        exprEl.value = "gravity(0, 0, 0)";
        actEl.value = "0";
    } else if (tpl === 'pulse') {
        nameEl.value = "Prismatic Color Pulsator";
        idEl.value = "law-color-pulse";
        exprEl.value = "set(target.color.r, sin(time * 2) * 0.5 + 0.5)";
        actEl.value = "0";
    } else if (tpl === 'bounce') {
        nameEl.value = "Super Kinetic Bounce";
        idEl.value = "law-kinetic-bounce";
        exprEl.value = "onEvent('contact-began') -> impulse(normal * 15.0)";
        actEl.value = "1";
    } else if (tpl === 'orbit') {
        nameEl.value = "Planetary Orbit Satellite";
        idEl.value = "law-satellite-orbit";
        exprEl.value = "set(position.x, cos(time) * 5.0); set(position.z, sin(time) * 5.0)";
        actEl.value = "0";
    }
}

function renderLawCards() {
    const grid = document.getElementById('laws-grid');
    if (!grid) return;

    grid.innerHTML = '';
    const laws = App.state.laws || [];

    if (laws.length === 0) {
        grid.innerHTML = '<div style="padding: 24px; color: var(--text-muted);">No laws currently active in universe</div>';
        return;
    }

    laws.forEach(law => {
        const card = document.createElement('div');
        card.className = 'law-card';

        const isChecked = law.enabled ? 'checked' : '';
        const actLabel = law.activation === 1 ? 'OnEvent' : (law.activation === 2 ? 'WhileTrue' : 'Continuous');

        card.innerHTML = `
            <div class="law-card-header">
                <div>
                    <div class="law-name">${law.name || law.identifier}</div>
                    <div class="law-id">@${law.identifier} • <span style="color: var(--accent-cyan);">${actLabel}</span></div>
                </div>
                <label class="toggle-switch">
                    <input type="checkbox" ${isChecked} data-law-id="${law.identifier}">
                    <span class="toggle-slider"></span>
                </label>
            </div>
            <div class="law-code-box">${law.expression || 'authored_law_action()'}</div>
        `;

        const toggleInput = card.querySelector('input');
        toggleInput.addEventListener('change', (e) => {
            const enabled = e.target.checked;
            toggleLaw(law.identifier, enabled);
        });

        grid.appendChild(card);
    });
}

function toggleLaw(identifier, enabled) {
    if (App.socket) {
        App.socket.emit('toggle_law', { identifier, enabled });
        showToast(`Law @${identifier} ${enabled ? 'Enabled' : 'Disabled'}`, "success");
    }
}

// --- Studio 3: Zone Navigator ---
function initZoneNavigator() {
    const openCreateZoneBtn = document.getElementById('open-create-zone-modal');
    if (openCreateZoneBtn) {
        openCreateZoneBtn.addEventListener('click', () => {
            document.getElementById('create-zone-modal').classList.add('active');
        });
    }

    const closeZoneModalBtn = document.getElementById('close-zone-modal');
    if (closeZoneModalBtn) {
        closeZoneModalBtn.addEventListener('click', () => {
            document.getElementById('create-zone-modal').classList.remove('active');
        });
    }

    const submitZoneBtn = document.getElementById('submit-create-zone-btn');
    if (submitZoneBtn) {
        submitZoneBtn.addEventListener('click', () => {
            const name = document.getElementById('new-zone-name').value.trim();
            const kind = document.getElementById('new-zone-kind').value;
            if (!name) return;

            fetch('/api/zones', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ name, kind })
            }).then(() => {
                document.getElementById('create-zone-modal').classList.remove('active');
                showToast(`Zone Authoring: ${name}`, "success");
            });
        });
    }
}

function renderHeaderZoneSelect() {
    const select = document.getElementById('header-zone-select');
    if (!select) return;

    select.innerHTML = '';
    const zones = App.state.zones || [];
    const activeIdx = App.state.active_zone_index || 0;

    zones.forEach((z, idx) => {
        const opt = document.createElement('option');
        opt.value = idx;
        opt.innerText = `${z.name || z.identifier} (${z.object_count || 0})`;
        if (idx === activeIdx) opt.selected = true;
        select.appendChild(opt);
    });
}

function renderZoneCards() {
    const grid = document.getElementById('zones-grid');
    if (!grid) return;

    grid.innerHTML = '';
    const zones = App.state.zones || [];
    const activeIdx = App.state.active_zone_index || 0;

    zones.forEach((z, idx) => {
        const card = document.createElement('div');
        const isActive = idx === activeIdx;
        card.className = `law-card ${isActive ? 'active-zone-card' : ''}`;
        if (isActive) {
            card.style.borderColor = 'var(--accent-cyan)';
            card.style.boxShadow = '0 0 20px rgba(0, 240, 255, 0.2)';
        }

        card.innerHTML = `
            <div class="law-card-header">
                <div>
                    <div class="law-name">${z.name || z.identifier} ${isActive ? '<span class="brand-badge" style="margin-left:8px;">ACTIVE</span>' : ''}</div>
                    <div class="law-id">Owner: ${z.owner || 'Player'} • Scope: ${z.scope || 'Local'}</div>
                </div>
            </div>
            <div style="font-size: 0.8rem; color: var(--text-secondary);">
                Beings within Reality: <strong style="color: var(--accent-cyan);">${z.object_count || 0}</strong>
            </div>
            <div style="display: flex; gap: 8px; margin-top: 8px;">
                <button class="btn ${isActive ? 'btn-secondary' : 'btn-primary'} btn-sm" onclick="switchZone(${idx})">
                    ${isActive ? 'Current Zone' : 'Teleport to Zone'}
                </button>
            </div>
        `;
        grid.appendChild(card);
    });
}

function switchZone(index) {
    if (App.socket) {
        App.socket.emit('switch_zone', { index });
        showToast(`Transitioning to Zone #${index}...`, "success");
    }
}

async function fetchSaves() {
    try {
        const res = await fetch('/api/saves');
        if (res.ok) {
            const data = await res.json();
            renderSavesList(data.saves || []);
        }
    } catch (e) {}
}

function renderSavesList(saves) {
    const container = document.getElementById('saves-list');
    if (!container) return;

    container.innerHTML = '';
    if (saves.length === 0) {
        container.innerHTML = '<div style="color: var(--text-muted); font-size: 0.8rem;">No saved world files found</div>';
        return;
    }

    saves.forEach(s => {
        const row = document.createElement('div');
        row.className = 'object-item';
        row.innerHTML = `
            <div>
                <strong>${s.filename}</strong>
                <span style="color: var(--text-muted); font-size: 0.7rem; margin-left: 8px;">${s.size_kb} KB</span>
            </div>
            <button class="btn btn-secondary btn-sm" onclick="loadSaveFile('${s.filename}')">Load</button>
        `;
        container.appendChild(row);
    });
}

function loadSaveFile(filename) {
    showToast(`Loading save file ${filename}...`, "success");
}

// --- Studio 4: Logos & Utterance Console ---
function initLogosConsole() {
    const input = document.getElementById('logos-input-text');
    const emitBtn = document.getElementById('logos-emit-btn');

    const emit = () => {
        const text = input.value.trim();
        if (!text) return;
        
        if (App.socket) {
            App.socket.emit('utterance', { text: text });
        }
        input.value = '';
        showToast(`Spoke into World: "${text}"`, "success");
    };

    if (emitBtn) emitBtn.addEventListener('click', emit);
    if (input) {
        input.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') emit();
        });
    }

    // Preset Word Chips
    document.querySelectorAll('.preset-chip').forEach(chip => {
        chip.addEventListener('click', () => {
            const word = chip.getAttribute('data-word');
            if (App.socket) {
                App.socket.emit('utterance', { text: word });
                showToast(`Spoke into World: "${word}"`, "success");
            }
        });
    });
}

// --- Studio 5: Person & Physics ---
function initPhysicsControls() {
    const bindTeleport = (id, coords) => {
        const btn = document.getElementById(id);
        if (btn) {
            btn.addEventListener('click', () => teleportPlayer(coords));
        }
    };

    bindTeleport('tp-spawn', [0, 1.8, 5]);
    bindTeleport('tp-origin', [0, 1.8, 0]);
    bindTeleport('tp-sky', [0, 35, 0]);
    bindTeleport('tp-sanctum', [-8, 2, -8]);

    const customTpBtn = document.getElementById('tp-custom-btn');
    if (customTpBtn) {
        customTpBtn.addEventListener('click', () => {
            const px = parseFloat(document.getElementById('tp-custom-x').value) || 0;
            const py = parseFloat(document.getElementById('tp-custom-y').value) || 0;
            const pz = parseFloat(document.getElementById('tp-custom-z').value) || 0;
            teleportPlayer([px, py, pz]);
        });
    }

    const flightSwitch = document.getElementById('physics-flying-toggle');
    if (flightSwitch) {
        flightSwitch.addEventListener('change', (e) => {
            if (App.socket) App.socket.emit('set_physics', { flying: e.target.checked });
        });
    }

    const gravVizSwitch = document.getElementById('physics-gravviz-toggle');
    if (gravVizSwitch) {
        gravVizSwitch.addEventListener('change', (e) => {
            if (App.socket) App.socket.emit('set_physics', { gravity_viz: e.target.checked });
        });
    }
}

function teleportPlayer(coords) {
    if (App.socket) {
        App.socket.emit('teleport', { position: coords });
        showToast(`Teleported Player to [${coords.map(n=>n.toFixed(1)).join(', ')}]`, "success");
    }
}

function renderPersonPhysics() {
    const player = App.state.player;
    if (!player) return;

    const pos = player.position || [0,0,0];
    const posText = `[X: ${pos[0].toFixed(2)}, Y: ${pos[1].toFixed(2)}, Z: ${pos[2].toFixed(2)}]`;
    const posEl = document.getElementById('player-pos-display');
    if (posEl) posEl.innerText = posText;

    const flightSwitch = document.getElementById('physics-flying-toggle');
    if (flightSwitch && App.state.physics) {
        flightSwitch.checked = !!App.state.physics.flying;
    }

    const gravVizSwitch = document.getElementById('physics-gravviz-toggle');
    if (gravVizSwitch && App.state.physics) {
        gravVizSwitch.checked = !!App.state.physics.gravity_viz;
    }
}

// --- Studio 6: Robotics Panel ---
function initRoboticsPanel() {
    ['rob-joint-1', 'rob-joint-2', 'rob-joint-3', 'rob-gripper'].forEach(id => {
        const slider = document.getElementById(id);
        const valDisp = document.getElementById(`${id}-val`);
        if (slider && valDisp) {
            slider.addEventListener('input', () => {
                valDisp.innerText = `${slider.value}°`;
                if (App.socket) {
                    App.socket.emit('write_property', {
                        target: '@physical-channel',
                        property: slider.getAttribute('data-joint'),
                        value: parseFloat(slider.value)
                    });
                }
            });
        }
    });
}

// --- Studio 7: Live Event & Audit Log Stream ---
function initEventLogStream() {
    const clearBtn = document.getElementById('clear-event-log-btn');
    if (clearBtn) {
        clearBtn.addEventListener('click', () => {
            App.eventLog = [];
            renderEventLog();
        });
    }

    const pauseBtn = document.getElementById('pause-event-log-btn');
    if (pauseBtn) {
        pauseBtn.addEventListener('click', () => {
            App.eventLogPaused = !App.eventLogPaused;
            pauseBtn.innerText = App.eventLogPaused ? "Resume Stream" : "Pause Stream";
        });
    }
}

function addEventToLog(evt) {
    if (App.eventLogPaused) return;
    const timeStr = new Date().toLocaleTimeString();
    App.eventLog.unshift({ time: timeStr, data: evt });
    if (App.eventLog.length > 200) App.eventLog.pop();
    renderEventLog();
}

function renderEventLog() {
    const container = document.getElementById('events-stream-list');
    if (!container) return;

    container.innerHTML = '';
    App.eventLog.slice(0, 50).forEach(item => {
        const div = document.createElement('div');
        div.className = 'terminal-msg';
        const type = item.data.type || item.data.event || 'engine_event';
        div.innerHTML = `
            <span class="msg-time">[${item.time}]</span>
            <span class="msg-speaker">&lt;${type}&gt;</span>
            <span class="msg-text">${JSON.stringify(item.data)}</span>
        `;
        container.appendChild(div);
    });
}

// --- Utility Helpers ---
function getShapeName(kind) {
    const shapes = ["Cube", "Polyhedron", "Sphere", "Cylinder", "Cone", "Ellipsoid", "Ovoid", "Paraboloid", "Torus", "RoundedBox", "Shape2D", "Text2D", "Field", "Patch"];
    return shapes[kind] || "Cube";
}

function rgbToHex(r, g, b) {
    const toHex = (c) => {
        const hex = Math.round(Math.min(1, Math.max(0, c)) * 255).toString(16);
        return hex.length === 1 ? '0' + hex : hex;
    };
    return `#${toHex(r)}${toHex(g)}${toHex(b)}`;
}

function hexToRgb(hex) {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? [
        parseInt(result[1], 16) / 255,
        parseInt(result[2], 16) / 255,
        parseInt(result[3], 16) / 255
    ] : [0.2, 0.7, 1.0];
}

function showToast(message, type = "info") {
    const container = document.getElementById('toast-container');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `<span>${message}</span>`;
    container.appendChild(toast);

    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateX(100%)';
        setTimeout(() => toast.remove(), 300);
    }, 3200);
}
