/**
 * Earthcall First Mover Studio & Engine Console
 * Real-time frontend communicating with Python backend (5005) & C++ Engine (8080)
 * Includes Comprehensive Telemetry, Metric Sidebars, Viewport HUD, and Performance Profiler
 */

// Global App State & Telemetry
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
    
    // Law Workshop State
    lawViewMode: 'grid',       // 'grid' | 'grouped' | 'graph'
    lawSystemFilter: 'all',    // 'all' | 'physics' | 'acoustics' | 'visual' | 'motion' | 'creation' | 'input' | 'custom'
    lawSearchQuery: '',
    selectedLawGraphNode: null,
    currentEditingLaw: null,   // Law currently opened in ECA pipeline editor modal

    // Live Telemetry & Metrics
    telemetry: {
        fps: 60.0,
        frameTime: 16.6,
        lastFrameTimestamp: performance.now(),
        fpsHistory: new Array(60).fill(60),
        eventHistory: new Array(60).fill(0),
        eventsThisSecond: 0,
        lastEventSecond: Math.floor(Date.now() / 1000),
        rttLatency: 2,
        vramMB: 18.4,
        heapMB: 24.8,
        turntableActive: false,
        wireframeActive: false,
        gridActive: true,
        lightsActive: true,
        lastPlayerPos: [0, 1.8, 5],
        lastVelocity: [0, 0, 0],
        speed: 0
    },

    eventLog: [],
    eventLogPaused: false,
    three: {
        scene: null,
        camera: null,
        renderer: null,
        controls: null,
        meshMap: new Map(), // object_id -> THREE.Mesh
        gridHelper: null,
        ambientLight: null,
        dirLight: null,
        pointLight: null,
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
    initTelemetryAndDiagnostics();
    
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
        recordTelemetryEvent();
    });

    // Lightweight RTT Ping loop
    setInterval(() => {
        if (App.connected && App.socket) {
            const start = performance.now();
            App.socket.emit('ping_check', { ts: start }, () => {
                const rtt = Math.max(1, Math.round(performance.now() - start));
                App.telemetry.rttLatency = rtt;
                const rttEl = document.getElementById('header-rtt-text');
                const diagSyncEl = document.getElementById('diag-sync-latency');
                if (rttEl) rttEl.innerText = `${rtt}ms`;
                if (diagSyncEl) diagSyncEl.innerText = `${rtt}.0 ms`;
            });
        }
    }, 3000);
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
    if (uptimeEl && status.uptime) {
        uptimeEl.innerText = `${Math.floor(status.uptime)}s`;
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

    const zoneSelect = document.getElementById('header-zone-select');
    if (zoneSelect) {
        zoneSelect.addEventListener('change', (e) => {
            const idx = parseInt(e.target.value, 10);
            switchZone(idx);
        });
    }

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

    if (tabName === 'objects' && App.three.renderer) {
        setTimeout(onThreeResize, 50);
    }
    if (tabName === 'laws' && App.lawViewMode === 'graph') {
        setTimeout(() => LawGraphEngine.resize(), 50);
    }
}

// --- 3D Interactive Three.js Viewport & Telemetry HUD ---
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
    App.three.gridHelper = grid;

    // Lights
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.6);
    scene.add(ambientLight);
    App.three.ambientLight = ambientLight;

    const dirLight = new THREE.DirectionalLight(0x00f0ff, 0.8);
    dirLight.position.set(10, 20, 10);
    scene.add(dirLight);
    App.three.dirLight = dirLight;

    const pointLight = new THREE.PointLight(0xa855f7, 1.2, 50);
    pointLight.position.set(-10, 10, -10);
    scene.add(pointLight);
    App.three.pointLight = pointLight;

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

    // Viewport HUD Toggles Binding
    initViewportHudControls();

    // High Performance Render & Telemetry Animation Loop
    let lastTime = performance.now();
    function animate() {
        requestAnimationFrame(animate);
        const now = performance.now();
        const delta = now - lastTime;
        lastTime = now;

        // Calculate FPS & Frame Time
        if (delta > 0) {
            const currentFps = Math.min(120, 1000 / delta);
            App.telemetry.fps = App.telemetry.fps * 0.9 + currentFps * 0.1;
            App.telemetry.frameTime = App.telemetry.frameTime * 0.9 + delta * 0.1;
        }

        // Turntable Auto-rotation
        if (App.telemetry.turntableActive && App.three.controls) {
            App.three.controls.autoRotate = true;
            App.three.controls.autoRotateSpeed = 1.5;
        } else if (App.three.controls) {
            App.three.controls.autoRotate = false;
        }

        if (App.three.controls) App.three.controls.update();
        renderer.render(scene, camera);

        // Viewport telemetry (drawcalls, vertices, bounds) is DOM work over
        // every mesh/object in the scene -- sampled on the 1s telemetry
        // interval (initTelemetryAndDiagnostics), not here. Running it every
        // frame made cost scale with object count against a 60Hz budget,
        // which is what turned "spawn a lot of orbs" into "the studio is
        // laggy now".
    }
    animate();
}

function initViewportHudControls() {
    const minBtn = document.getElementById('toggle-viewport-hud-btn');
    const hudBody = document.getElementById('viewport-hud-body');
    if (minBtn && hudBody) {
        minBtn.addEventListener('click', () => {
            const isHidden = hudBody.style.display === 'none';
            hudBody.style.display = isHidden ? 'flex' : 'none';
            minBtn.innerText = isHidden ? '−' : '+';
        });
    }

    // Wireframe Toggle
    document.getElementById('vp-wireframe-toggle')?.addEventListener('click', function() {
        App.telemetry.wireframeActive = !App.telemetry.wireframeActive;
        this.classList.toggle('active', App.telemetry.wireframeActive);
        App.three.meshMap.forEach(mesh => {
            if (mesh.material) mesh.material.wireframe = App.telemetry.wireframeActive;
        });
    });

    // Grid Toggle
    document.getElementById('vp-grid-toggle')?.addEventListener('click', function() {
        App.telemetry.gridActive = !App.telemetry.gridActive;
        this.classList.toggle('active', App.telemetry.gridActive);
        if (App.three.gridHelper) App.three.gridHelper.visible = App.telemetry.gridActive;
    });

    // Light Toggle
    document.getElementById('vp-light-toggle')?.addEventListener('click', function() {
        App.telemetry.lightsActive = !App.telemetry.lightsActive;
        this.classList.toggle('active', App.telemetry.lightsActive);
        const mult = App.telemetry.lightsActive ? 1.0 : 0.2;
        if (App.three.ambientLight) App.three.ambientLight.intensity = 0.6 * mult;
        if (App.three.dirLight) App.three.dirLight.intensity = 0.8 * mult;
        if (App.three.pointLight) App.three.pointLight.intensity = 1.2 * mult;
    });

    // Turntable Toggle
    document.getElementById('vp-turntable-toggle')?.addEventListener('click', function() {
        App.telemetry.turntableActive = !App.telemetry.turntableActive;
        this.classList.toggle('active', App.telemetry.turntableActive);
    });
}

function updateViewportLiveStats() {
    const drawcallsEl = document.getElementById('vp-drawcalls');
    const verticesEl = document.getElementById('vp-vertices');
    const altitudeEl = document.getElementById('vp-altitude');
    const headingEl = document.getElementById('vp-heading');
    const boundsEl = document.getElementById('vp-bounds');

    const meshCount = App.three.meshMap.size;
    if (drawcallsEl) drawcallsEl.innerText = meshCount + 2; // meshes + grid + player

    // Calculate vertices count
    let totalVerts = 0;
    App.three.meshMap.forEach(m => {
        if (m.geometry && m.geometry.attributes && m.geometry.attributes.position) {
            totalVerts += m.geometry.attributes.position.count;
        }
    });
    if (verticesEl) verticesEl.innerText = totalVerts > 0 ? totalVerts.toLocaleString() : '2,496';

    // Player Altitude & Heading
    if (App.state.player) {
        const py = App.state.player.position ? App.state.player.position[1] : 1.8;
        if (altitudeEl) altitudeEl.innerText = `${py >= 0 ? '+' : ''}${py.toFixed(2)} m`;

        const yaw = App.state.player.yaw !== undefined ? App.state.player.yaw : -90;
        const normYaw = Math.round((yaw % 360 + 360) % 360);
        let cardinal = 'N';
        if (normYaw >= 45 && normYaw < 135) cardinal = 'E';
        else if (normYaw >= 135 && normYaw < 225) cardinal = 'S';
        else if (normYaw >= 225 && normYaw < 315) cardinal = 'W';
        if (headingEl) headingEl.innerText = `${normYaw}° (${cardinal})`;
    }

    // Spatial Bounds Extents
    if (App.state.objects && App.state.objects.length > 0) {
        let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity, minZ = Infinity, maxZ = -Infinity;
        App.state.objects.forEach(o => {
            const p = o.position || [0,0,0];
            minX = Math.min(minX, p[0]); maxX = Math.max(maxX, p[0]);
            minY = Math.min(minY, p[1]); maxY = Math.max(maxY, p[1]);
            minZ = Math.min(minZ, p[2]); maxZ = Math.max(maxZ, p[2]);
        });
        const dx = Math.max(1, (maxX - minX)).toFixed(1);
        const dy = Math.max(1, (maxY - minY)).toFixed(1);
        const dz = Math.max(1, (maxZ - minZ)).toFixed(1);
        if (boundsEl) boundsEl.innerText = `${dx} × ${dy} × ${dz} m`;
    }
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
        case 0: return new THREE.BoxGeometry(s, s, s);
        case 1: return new THREE.DodecahedronGeometry(s * 0.7);
        case 2: return new THREE.SphereGeometry(s * 0.6, 24, 24);
        case 3: return new THREE.CylinderGeometry(s * 0.5, s * 0.5, s * 1.2, 24);
        case 4: return new THREE.ConeGeometry(s * 0.6, s * 1.2, 24);
        case 5: {
            const ellGeo = new THREE.SphereGeometry(s * 0.6, 24, 24);
            ellGeo.scale(1.4, 0.8, 1.0);
            return ellGeo;
        }
        case 8: return new THREE.TorusGeometry(s * 0.6, s * 0.2, 16, 32);
        case 9: return new THREE.BoxGeometry(s, s, s);
        default: return new THREE.BoxGeometry(s, s, s);
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
                wireframe: App.telemetry.wireframeActive
            });
            mesh = new THREE.Mesh(geo, mat);
            mesh.userData.objectId = id;
            mesh.userData.shapeKind = obj.shapeKind;
            mesh.userData.dimensions = obj.dimensions;
            scene.add(mesh);
            App.three.meshMap.set(id, mesh);
        } else {
            if (mesh.userData.shapeKind !== obj.shapeKind || mesh.userData.dimensions !== obj.dimensions) {
                mesh.geometry.dispose();
                mesh.geometry = createGeometryForShape(obj.shapeKind, obj.dimensions);
                mesh.userData.shapeKind = obj.shapeKind;
                mesh.userData.dimensions = obj.dimensions;
            }
            mesh.material.color.copy(threeCol);
        }

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

        if (id === App.selectedObjectId) {
            mesh.material.emissive.setHex(0x00f0ff);
            mesh.material.emissiveIntensity = 0.35;
        } else {
            mesh.material.emissive.setHex(0x000000);
            mesh.material.emissiveIntensity = 0;
        }
    });

    for (const [id, mesh] of App.three.meshMap.entries()) {
        if (!currentIds.has(id)) {
            scene.remove(mesh);
            mesh.geometry.dispose();
            mesh.material.dispose();
            App.three.meshMap.delete(id);
        }
    }

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
    renderLawSubsystem();
    renderZoneCards();
    renderPersonPhysics();
    renderStatsCounters();
    updateSubstrateFooterTelemetry();
}

function renderStatsCounters() {
    const objCountEl = document.getElementById('stat-object-count');
    const lawCountEl = document.getElementById('stat-law-count');
    const zoneCountEl = document.getElementById('stat-zone-count');
    const objLen = (App.state.objects || []).length;
    const lawLen = (App.state.laws || []).length;
    const zoneLen = (App.state.zones || []).length;

    if (objCountEl) objCountEl.innerText = objLen;
    if (lawCountEl) lawCountEl.innerText = lawLen;
    if (zoneCountEl) zoneCountEl.innerText = zoneLen;

    // Update VRAM and Heap Estimates
    const vram = (16.0 + objLen * 0.8).toFixed(1);
    const heap = (22.0 + (objLen + lawLen) * 0.5).toFixed(1);
    App.telemetry.vramMB = vram;
    App.telemetry.heapMB = heap;

    const vramEl = document.getElementById('header-vram-text');
    if (vramEl) vramEl.innerText = `${vram} MB`;
}

function updateSubstrateFooterTelemetry() {
    const heapValEl = document.getElementById('sidebar-heap-val');
    const heapBarEl = document.getElementById('sidebar-heap-bar');
    if (heapValEl) heapValEl.innerText = `${App.telemetry.heapMB} MB`;
    if (heapBarEl) {
        const pct = Math.min(100, Math.round((parseFloat(App.telemetry.heapMB) / 128) * 100));
        heapBarEl.style.width = `${pct}%`;
    }
}

// --- Telemetry & Diagnostics Cockpit Drawer ---
function initTelemetryAndDiagnostics() {
    const openBtn = document.getElementById('open-metrics-drawer-btn');
    const closeBtn = document.getElementById('close-metrics-drawer-btn');
    const drawer = document.getElementById('metrics-diagnostics-drawer');

    if (openBtn && drawer) {
        openBtn.addEventListener('click', () => drawer.classList.add('open'));
    }
    if (closeBtn && drawer) {
        closeBtn.addEventListener('click', () => drawer.classList.remove('open'));
    }

    // 1-second telemetry sampling loop
    setInterval(() => {
        // Shift FPS History
        App.telemetry.fpsHistory.shift();
        App.telemetry.fpsHistory.push(Math.round(App.telemetry.fps));

        // Shift Event History
        App.telemetry.eventHistory.shift();
        App.telemetry.eventHistory.push(App.telemetry.eventsThisSecond);
        const evRateEl = document.getElementById('sidebar-eventbus-val');
        const diagEvEl = document.getElementById('diag-events-numeric');
        if (evRateEl) evRateEl.innerText = `${App.telemetry.eventsThisSecond} ev/s`;
        if (diagEvEl) diagEvEl.innerText = `${App.telemetry.eventsThisSecond} ev/s`;
        App.telemetry.eventsThisSecond = 0;

        // Header FPS & Frame Budget update
        const fpsTextEl = document.getElementById('header-fps-text');
        const ftTextEl = document.getElementById('header-frametime-text');
        const fpsDotEl = document.getElementById('header-fps-dot');
        const diagFpsNum = document.getElementById('diag-fps-numeric');

        const currentFps = App.telemetry.fps.toFixed(1);
        const currentFt = App.telemetry.frameTime.toFixed(1);

        if (fpsTextEl) fpsTextEl.innerText = `${currentFps} FPS`;
        if (ftTextEl) ftTextEl.innerText = `${currentFt}ms`;
        if (diagFpsNum) diagFpsNum.innerText = `${currentFps} FPS (${currentFt}ms)`;

        if (fpsDotEl) {
            fpsDotEl.className = 'hud-indicator-dot ' + (App.telemetry.fps >= 50 ? 'green' : (App.telemetry.fps >= 30 ? 'yellow' : 'red'));
        }

        // Viewport HUD stats (drawcalls/vertices/altitude/heading/bounds) --
        // once a second is plenty for a readout; every frame is not.
        updateViewportLiveStats();

        // Render diagnostics graphs if drawer is open
        if (drawer && drawer.classList.contains('open')) {
            drawDiagnosticsCharts();
            updateEntityCompositionDiagnostics();
        }
    }, 1000);
}

function recordTelemetryEvent() {
    App.telemetry.eventsThisSecond++;
}

function drawDiagnosticsCharts() {
    // 1. Draw FPS Sparkline
    const fpsCanvas = document.getElementById('diag-fps-canvas');
    if (fpsCanvas) {
        drawSparkline(fpsCanvas, App.telemetry.fpsHistory, 0, 75, '#10b981', '#00f0ff');
    }

    // 2. Draw Event Throughput Waveform
    const evCanvas = document.getElementById('diag-event-canvas');
    if (evCanvas) {
        const maxEv = Math.max(10, ...App.telemetry.eventHistory);
        drawSparkline(evCanvas, App.telemetry.eventHistory, 0, maxEv * 1.2, '#a855f7', '#ec4899');
    }
}

function drawSparkline(canvas, data, minVal, maxVal, colStart, colEnd) {
    const ctx = canvas.getContext('2d');
    const w = canvas.clientWidth;
    const h = canvas.clientHeight;
    if (canvas.width !== w || canvas.height !== h) {
        canvas.width = w;
        canvas.height = h;
    }

    ctx.clearRect(0, 0, w, h);

    // Subtle horizontal gridlines
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, h * 0.25); ctx.lineTo(w, h * 0.25);
    ctx.moveTo(0, h * 0.5);  ctx.lineTo(w, h * 0.5);
    ctx.moveTo(0, h * 0.75); ctx.lineTo(w, h * 0.75);
    ctx.stroke();

    if (data.length < 2) return;

    const step = w / (data.length - 1);
    const range = maxVal - minVal || 1;

    ctx.beginPath();
    for (let i = 0; i < data.length; i++) {
        const val = Math.max(minVal, Math.min(maxVal, data[i]));
        const y = h - ((val - minVal) / range) * (h - 8) - 4;
        const x = i * step;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    }

    const grad = ctx.createLinearGradient(0, 0, w, 0);
    grad.addColorStop(0, colStart);
    grad.addColorStop(1, colEnd);
    ctx.strokeStyle = grad;
    ctx.lineWidth = 2;
    ctx.stroke();

    // Fill under curve
    ctx.lineTo(w, h);
    ctx.lineTo(0, h);
    ctx.closePath();
    const fillGrad = ctx.createLinearGradient(0, 0, 0, h);
    fillGrad.addColorStop(0, 'rgba(0, 240, 255, 0.15)');
    fillGrad.addColorStop(1, 'rgba(0, 240, 255, 0.0)');
    ctx.fillStyle = fillGrad;
    ctx.fill();
}

function updateEntityCompositionDiagnostics() {
    const objects = App.state.objects || [];
    let cubes = 0, spheres = 0, rings = 0, others = 0;

    objects.forEach(o => {
        if (o.shapeKind === 0) cubes++;
        else if (o.shapeKind === 2) spheres++;
        else if (o.shapeKind === 8) rings++;
        else others++;
    });

    const total = objects.length || 1;
    const cubePct = Math.round((cubes / total) * 100);
    const spherePct = Math.round((spheres / total) * 100);
    const ringPct = Math.round((rings / total) * 100);

    const cubeSeg = document.getElementById('diag-cube-seg');
    const sphereSeg = document.getElementById('diag-sphere-seg');
    const ringSeg = document.getElementById('diag-torus-seg');

    if (cubeSeg) cubeSeg.style.width = `${cubePct}%`;
    if (sphereSeg) sphereSeg.style.width = `${spherePct}%`;
    if (ringSeg) ringSeg.style.width = `${ringPct}%`;

    const cVal = document.getElementById('diag-cubes-val');
    const sVal = document.getElementById('diag-spheres-val');
    const rVal = document.getElementById('diag-rings-val');
    const countEl = document.getElementById('diag-entities-count');

    if (cVal) cVal.innerText = cubes;
    if (sVal) sVal.innerText = spheres;
    if (rVal) rVal.innerText = rings;
    if (countEl) countEl.innerText = `${objects.length} Beings`;
}

// --- Studio 1: Object Studio & Inspector ---
function initObjectStudio() {
    const searchInput = document.getElementById('object-search');
    if (searchInput) {
        searchInput.addEventListener('input', () => renderObjectExplorer());
    }

    document.querySelectorAll('.spawner-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const shape = btn.getAttribute('data-shape');
            spawnObjectQuick(shape);
        });
    });

    ['insp-pos-x', 'insp-pos-y', 'insp-pos-z',
     'insp-rot-x', 'insp-rot-y', 'insp-rot-z',
     'insp-dim', 'insp-shape', 'insp-mat', 'insp-name', 'insp-color-hex'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.addEventListener('input', () => onInspectorFieldChange());
    });

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

// ==========================================================================
// STUDIO 2: Authors of Law (Grouping by System & Interconnected Graph Mode)
// ==========================================================================

function categorizeLaw(law) {
    const id = (law.identifier || '').toLowerCase();
    const name = (law.name || '').toLowerCase();
    const expr = (law.expression || '').toLowerCase();

    if (id.includes('acoustics') || id.includes('sound') || name.includes('acoustic') || name.includes('sound') || expr.includes('acoustic') || expr.includes('sound')) {
        return {
            key: 'acoustics',
            name: 'Acoustics & Audio',
            icon: '🔊',
            color: '#f59e0b',
            desc: 'Sound emitters, ADSR envelopes, vibrato LFOs, and acoustic occlusion behind geometry.'
        };
    }
    if (id.includes('color') || id.includes('pulse') || id.includes('visual') || id.includes('material') || name.includes('color') || expr.includes('color')) {
        return {
            key: 'visual',
            name: 'Visual & Materials',
            icon: '🌈',
            color: '#ec4899',
            desc: 'Shader parameters, color pulsation, albedo modulations, and material state transitions.'
        };
    }
    if (id.includes('orbit') || id.includes('bounce') || id.includes('kinetic') || id.includes('motion') || name.includes('orbit') || name.includes('bounce') || expr.includes('impulse')) {
        return {
            key: 'motion',
            name: 'Motion & Kinetics',
            icon: '🛸',
            color: '#a855f7',
            desc: 'Orbital trajectories, collision restitution, kinetic impulses, and angular momentum.'
        };
    }
    if (id.includes('gravity') || id.includes('kinematics') || id.includes('zero-g') || name.includes('gravity') || name.includes('kinematics') || expr.includes('gravity') || expr.includes('velocity')) {
        return {
            key: 'physics',
            name: 'Physics & Gravitation',
            icon: '🌌',
            color: '#3b82f6',
            desc: 'Classical kinematics, gravitational fields, acceleration vectors, and integration.'
        };
    }
    if (id.includes('shape') || id.includes('spawn') || id.includes('create') || id.includes('generator') || name.includes('shape') || name.includes('create')) {
        return {
            key: 'creation',
            name: 'Creation Tools',
            icon: '📐',
            color: '#00f0ff',
            desc: '3D geometric mesh generators, singular set-to-set constructors, and concept instantiators.'
        };
    }
    if (id.includes('input') || id.includes('mouse') || id.includes('keyboard') || id.includes('locomotion') || id.includes('interaction')) {
        return {
            key: 'input',
            name: 'Input & Control',
            icon: '🎮',
            color: '#6366f1',
            desc: 'Locomotion channels, cursor pointer events, gesture picking, and input bindings.'
        };
    }
    return {
        key: 'custom',
        name: 'Custom Authored',
        icon: '✨',
        color: '#10b981',
        desc: 'User-authored OntoMath laws and custom experimental rules.'
    };
}

function initLawWorkshop() {
    const viewGridBtn = document.getElementById('view-mode-grid');
    const viewGroupedBtn = document.getElementById('view-mode-grouped');
    const viewGraphBtn = document.getElementById('view-mode-graph');

    if (viewGridBtn && viewGroupedBtn && viewGraphBtn) {
        viewGridBtn.addEventListener('click', () => setLawViewMode('grid'));
        viewGroupedBtn.addEventListener('click', () => setLawViewMode('grouped'));
        viewGraphBtn.addEventListener('click', () => setLawViewMode('graph'));
    }

    document.querySelectorAll('.system-chip').forEach(chip => {
        chip.addEventListener('click', () => {
            document.querySelectorAll('.system-chip').forEach(c => c.classList.remove('active'));
            chip.classList.add('active');
            App.lawSystemFilter = chip.getAttribute('data-system') || 'all';
            renderLawSubsystem();
        });
    });

    const searchInput = document.getElementById('law-search-input');
    if (searchInput) {
        searchInput.addEventListener('input', (e) => {
            App.lawSearchQuery = e.target.value;
            renderLawSubsystem();
        });
    }

    // Graph Toolbar Controls
    document.getElementById('graph-zoom-in')?.addEventListener('click', () => LawGraphEngine.zoom(1.2));
    document.getElementById('graph-zoom-out')?.addEventListener('click', () => LawGraphEngine.zoom(0.8));
    document.getElementById('graph-zoom-reset')?.addEventListener('click', () => LawGraphEngine.autoFit());
    document.getElementById('graph-layout-mode')?.addEventListener('change', (e) => {
        LawGraphEngine.setLayout(e.target.value);
    });
    document.getElementById('graph-spacing-mode')?.addEventListener('change', (e) => {
        LawGraphEngine.setSpacing(e.target.value);
    });
    document.getElementById('close-graph-drawer')?.addEventListener('click', () => {
        document.getElementById('graph-node-drawer')?.classList.remove('open');
        App.selectedLawGraphNode = null;
        LawGraphEngine.requestRedraw();
    });

    // Create Law Modal
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

    // Close ECA Pipeline Modal
    const closeEcaModalBtn = document.getElementById('close-eca-modal');
    if (closeEcaModalBtn) {
        closeEcaModalBtn.addEventListener('click', () => {
            document.getElementById('eca-pipeline-modal').classList.remove('active');
            App.currentEditingLaw = null;
        });
    }

    ['eca-when-trigger', 'eca-when-activation', 'eca-when-scope',
     'eca-cond-enable-toggle', 'eca-cond-path', 'eca-cond-op', 'eca-cond-val',
     'eca-act-kind', 'eca-act-path', 'eca-act-amp', 'eca-act-freq', 'eca-act-phase', 'eca-act-offset'].forEach(id => {
        const el = document.getElementById(id);
        if (el) {
            el.addEventListener('input', () => updateEcaFlowSummaries());
            el.addEventListener('change', () => updateEcaFlowSummaries());
        }
    });

    const applyEcaBtn = document.getElementById('eca-apply-btn');
    if (applyEcaBtn) {
        applyEcaBtn.addEventListener('click', () => saveAndApplyEcaNodes());
    }

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

    LawGraphEngine.init();
}

function setLawViewMode(mode) {
    App.lawViewMode = mode;
    ['grid', 'grouped', 'graph'].forEach(m => {
        document.getElementById(`view-mode-${m}`)?.classList.toggle('active', m === mode);
        document.getElementById(`laws-${m}-view`)?.classList.toggle('active', m === mode);
    });

    if (mode === 'graph') {
        setTimeout(() => LawGraphEngine.resize(), 30);
    }
    renderLawSubsystem();
}

function applyLawTemplate(tpl) {
    const nameEl = document.getElementById('new-law-name');
    const idEl = document.getElementById('new-law-id');
    const exprEl = document.getElementById('new-law-expr');
    const actEl = document.getElementById('new-law-activation');

    if (tpl === 'zero-g') {
        nameEl.value = "Zero-Gravity Zone";
        idEl.value = "law-zero-g";
        exprEl.value = "gravity(0, 0, 0) -> weightless_equilibrium()";
        actEl.value = "0";
    } else if (tpl === 'pulse') {
        nameEl.value = "Prismatic Color Pulsator";
        idEl.value = "law-color-pulse";
        exprEl.value = "set(target.color.r, sin(time * 2) * 0.5 + 0.5)";
        actEl.value = "0";
    } else if (tpl === 'bounce') {
        nameEl.value = "Super Kinetic Bounce";
        idEl.value = "law-kinetic-bounce";
        exprEl.value = "onEvent('contact-began') -> set(velocity.y, 12.0)";
        actEl.value = "1";
    } else if (tpl === 'orbit') {
        nameEl.value = "Planetary Orbit Satellite";
        idEl.value = "law-satellite-orbit";
        exprEl.value = "set(position.x, 5.0 * cos(time)); set(position.z, 5.0 * sin(time))";
        actEl.value = "0";
    }
}

function updateSystemChipCounts(laws) {
    const counts = { all: laws.length, physics: 0, acoustics: 0, visual: 0, motion: 0, creation: 0, input: 0, custom: 0 };
    laws.forEach(l => {
        const cat = categorizeLaw(l);
        if (counts[cat.key] !== undefined) counts[cat.key]++;
        else counts.custom++;
    });

    Object.keys(counts).forEach(k => {
        const el = document.getElementById(`chip-count-${k}`);
        if (el) el.innerText = counts[k];
    });

    // Update Law Ecosystem Bar Segment Percentages
    const total = laws.length || 1;
    ['physics', 'acoustics', 'visual', 'motion', 'creation', 'custom'].forEach(k => {
        const seg = document.getElementById(`seg-${k}-fill`);
        if (seg) {
            const pct = Math.round((counts[k] / total) * 100);
            seg.style.width = `${pct}%`;
            seg.title = `${k}: ${pct}%`;
        }
    });

    const totalTag = document.getElementById('eco-total-laws-tag');
    if (totalTag) totalTag.innerText = `${laws.length} Registered Laws`;

    // Active Ratio
    const enabledCount = laws.filter(l => l.enabled).length;
    const ratio = Math.round((enabledCount / total) * 100);
    const activeRatioEl = document.getElementById('eco-active-ratio');
    if (activeRatioEl) activeRatioEl.innerText = `${ratio}%`;

    // Edge Count
    const edgesEl = document.getElementById('eco-edges-count');
    if (edgesEl && LawGraphEngine.links) edgesEl.innerText = LawGraphEngine.links.length;
}

function renderLawSubsystem() {
    const laws = App.state.laws || [];
    updateSystemChipCounts(laws);

    if (App.lawViewMode === 'grid') {
        renderFlatLawGrid(laws);
    } else if (App.lawViewMode === 'grouped') {
        renderGroupedLawSections(laws);
    } else if (App.lawViewMode === 'graph') {
        LawGraphEngine.updateData(laws);
    }
}

function createLawCardElement(law, stripeColor) {
    const card = document.createElement('div');
    card.className = 'law-card';

    const isChecked = law.enabled ? 'checked' : '';
    const actLabel = law.activation === 1 ? 'OnEvent' : (law.activation === 2 ? 'WhileTrue' : 'Continuous');
    const col = stripeColor || '#a855f7';

    card.innerHTML = `
        <div class="law-system-stripe" style="background: ${col};"></div>
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
        <div class="law-card-footer">
            <button class="btn btn-secondary btn-sm eca-inspect-btn" style="width: 100%; justify-content: center; gap: 6px;">
                <span>🧬 Inspect & Edit ECA Graph</span>
            </button>
        </div>
    `;

    const toggleInput = card.querySelector('input');
    toggleInput.addEventListener('change', (e) => {
        const enabled = e.target.checked;
        toggleLaw(law.identifier, enabled);
    });

    const inspectBtn = card.querySelector('.eca-inspect-btn');
    inspectBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        openEcaPipelineModal(law);
    });

    return card;
}

function renderFlatLawGrid(laws) {
    const grid = document.getElementById('laws-grid');
    if (!grid) return;
    grid.innerHTML = '';

    const filterText = (App.lawSearchQuery || '').toLowerCase();
    const systemFilter = App.lawSystemFilter || 'all';

    const filtered = laws.filter(l => {
        const cat = categorizeLaw(l);
        const matchSys = (systemFilter === 'all' || cat.key === systemFilter);
        const matchSearch = (l.name || '').toLowerCase().includes(filterText) ||
                            (l.identifier || '').toLowerCase().includes(filterText) ||
                            (l.expression || '').toLowerCase().includes(filterText);
        return matchSys && matchSearch;
    });

    if (filtered.length === 0) {
        grid.innerHTML = '<div style="padding: 32px; color: var(--text-muted); grid-column: 1/-1; text-align: center;">No laws matching filter criteria.</div>';
        return;
    }

    filtered.forEach(law => {
        const cat = categorizeLaw(law);
        grid.appendChild(createLawCardElement(law, cat.color));
    });
}

function renderGroupedLawSections(laws) {
    const container = document.getElementById('laws-grouped-container');
    if (!container) return;
    container.innerHTML = '';

    const filterText = (App.lawSearchQuery || '').toLowerCase();
    const systemFilter = App.lawSystemFilter || 'all';

    const systems = [
        { key: 'physics', name: 'Physics & Gravitation', icon: '🌌', color: '#3b82f6', desc: 'Classical kinematics, gravitational fields, acceleration vectors, and integration.' },
        { key: 'acoustics', name: 'Acoustics & Audio', icon: '🔊', color: '#f59e0b', desc: 'Sound emitters, ADSR envelopes, vibrato LFOs, and acoustic occlusion behind geometry.' },
        { key: 'visual', name: 'Visual & Materials', icon: '🌈', color: '#ec4899', desc: 'Shader parameters, color pulsation, albedo modulations, and material state transitions.' },
        { key: 'motion', name: 'Motion & Kinetics', icon: '🛸', color: '#a855f7', desc: 'Orbital trajectories, collision restitution, kinetic impulses, and angular momentum.' },
        { key: 'creation', name: 'Creation Tools', icon: '📐', color: '#00f0ff', desc: '3D geometric mesh generators, singular set-to-set constructors, and concept instantiators.' },
        { key: 'input', name: 'Input & Control', icon: '🎮', color: '#6366f1', desc: 'Locomotion channels, cursor pointer events, gesture picking, and input bindings.' },
        { key: 'custom', name: 'Custom Authored', icon: '✨', color: '#10b981', desc: 'User-authored OntoMath laws and custom experimental rules.' }
    ];

    let renderedCount = 0;

    systems.forEach(sys => {
        if (systemFilter !== 'all' && systemFilter !== sys.key) return;

        const groupLaws = laws.filter(l => categorizeLaw(l).key === sys.key);
        const filteredLaws = groupLaws.filter(l => 
            (l.name || '').toLowerCase().includes(filterText) ||
            (l.identifier || '').toLowerCase().includes(filterText) ||
            (l.expression || '').toLowerCase().includes(filterText)
        );

        if (filteredLaws.length === 0 && systemFilter === 'all') return;

        renderedCount++;
        const section = document.createElement('div');
        section.className = 'system-section-card';
        section.style.borderLeft = `4px solid ${sys.color}`;

        const allEnabled = filteredLaws.length > 0 && filteredLaws.every(l => l.enabled);

        section.innerHTML = `
            <div class="system-section-header">
                <div class="system-title-box">
                    <span class="system-icon">${sys.icon}</span>
                    <div>
                        <div class="system-title" style="color: ${sys.color};">${sys.name} <span style="font-size: 0.8rem; color: var(--text-muted); font-weight: normal;">(${filteredLaws.length})</span></div>
                        <div class="system-subtitle">${sys.desc}</div>
                    </div>
                </div>
                <div style="display: flex; align-items: center; gap: 12px;">
                    <button class="btn btn-secondary btn-sm toggle-sys-batch-btn">
                        ${allEnabled ? 'Disable All' : 'Enable All'}
                    </button>
                </div>
            </div>
            <div class="cards-grid" style="margin-top: 10px;">
                <!-- Cards container -->
            </div>
        `;

        const cardsGrid = section.querySelector('.cards-grid');
        filteredLaws.forEach(law => {
            cardsGrid.appendChild(createLawCardElement(law, sys.color));
        });

        const batchBtn = section.querySelector('.toggle-sys-batch-btn');
        batchBtn.addEventListener('click', () => {
            const targetState = !allEnabled;
            filteredLaws.forEach(l => toggleLaw(l.identifier, targetState));
        });

        container.appendChild(section);
    });

    if (renderedCount === 0) {
        container.innerHTML = '<div style="padding: 32px; text-align: center; color: var(--text-muted);">No laws matching criteria in selected system.</div>';
    }
}

function toggleLaw(identifier, enabled) {
    if (App.socket) {
        App.socket.emit('command', {
            type: 'toggle_law',
            identifier: identifier,
            enabled: enabled
        });
        showToast(`Law @${identifier} ${enabled ? 'Enabled' : 'Disabled'}`, "success");
    }
}

// ==========================================================================
// ECA NODE PIPELINE MODAL & LIVE MODIFIER (When -> Condition -> Action)
// ==========================================================================

function openEcaPipelineModal(law) {
    App.currentEditingLaw = law;

    const modal = document.getElementById('eca-pipeline-modal');
    if (!modal) return;

    document.getElementById('eca-modal-law-name').innerText = law.name || law.identifier;
    document.getElementById('eca-modal-law-id').innerText = `@${law.identifier}`;

    const trigger = law.trigger || (law.activation === 1 ? 'contact-began' : 'universe.time');
    document.getElementById('eca-when-trigger').value = trigger;
    document.getElementById('eca-when-activation').value = law.activation !== undefined ? law.activation : 0;
    document.getElementById('eca-when-scope').value = law.scope !== undefined ? law.scope : 1;

    const condToggle = document.getElementById('eca-cond-enable-toggle');
    const hasCond = (law.conditionDescription && !law.conditionDescription.includes('always')) || false;
    condToggle.checked = hasCond;

    document.getElementById('eca-cond-path').value = law.conditionPath || (hasCond ? 'position.y' : '');
    document.getElementById('eca-cond-op').value = law.conditionOp || '==';
    document.getElementById('eca-cond-val').value = law.conditionVal !== undefined ? law.conditionVal : '0.0';

    const id = (law.identifier || '').toLowerCase();
    let actKind = 'map';
    let actPath = 'velocity.y';
    let amp = 1.0;
    let freq = 1.0;
    let phase = 0.0;
    let offset = 0.0;

    if (id.includes('gravity') || id.includes('zero-g')) {
        actKind = 'flow';
        actPath = 'velocity';
        offset = 9.81;
    } else if (id.includes('color') || id.includes('pulse')) {
        actKind = 'map';
        actPath = 'color.r';
        amp = 0.5;
        freq = 2.0;
        offset = 0.5;
    } else if (id.includes('orbit')) {
        actKind = 'map';
        actPath = 'position.x';
        amp = 5.0;
        freq = 1.0;
    } else if (id.includes('bounce')) {
        actKind = 'map';
        actPath = 'velocity.y';
        offset = 12.0;
    }

    document.getElementById('eca-act-kind').value = actKind;
    document.getElementById('eca-act-path').value = actPath;
    document.getElementById('eca-act-amp').value = amp;
    document.getElementById('eca-act-freq').value = freq;
    document.getElementById('eca-act-phase').value = phase;
    document.getElementById('eca-act-offset').value = offset;

    updateEcaFlowSummaries();
    modal.classList.add('active');
}

function updateEcaFlowSummaries() {
    const trigger = document.getElementById('eca-when-trigger')?.value || 'universe.time';
    const actMode = document.getElementById('eca-when-activation')?.value;
    const actLabel = actMode === '1' ? 'OnEvent' : (actMode === '2' ? 'OnBecomeTrue' : 'WhileTrue');
    
    document.getElementById('flow-when-summary').innerText = `${trigger} (${actLabel})`;

    const condEnabled = document.getElementById('eca-cond-enable-toggle')?.checked;
    if (condEnabled) {
        const cPath = document.getElementById('eca-cond-path')?.value || 'property';
        const cOp = document.getElementById('eca-cond-op')?.value || '==';
        const cVal = document.getElementById('eca-cond-val')?.value || '0.0';
        document.getElementById('flow-cond-summary').innerText = `${cPath} ${cOp} ${cVal}`;
    } else {
        document.getElementById('flow-cond-summary').innerText = "Always True (No Guard)";
    }

    const aKind = document.getElementById('eca-act-kind')?.value || 'map';
    const aPath = document.getElementById('eca-act-path')?.value || 'target';
    const aAmp = document.getElementById('eca-act-amp')?.value || '1.0';
    const aFreq = document.getElementById('eca-act-freq')?.value || '1.0';

    if (aKind === 'map') {
        document.getElementById('flow-act-summary').innerText = `Map ${aPath} := sin(${aFreq}t) * ${aAmp}`;
    } else if (aKind === 'flow') {
        document.getElementById('flow-act-summary').innerText = `Flow ${aPath} += rate * dt`;
    } else {
        document.getElementById('flow-act-summary').innerText = `${aKind.toUpperCase()} ${aPath}`;
    }
}

function saveAndApplyEcaNodes() {
    if (!App.currentEditingLaw) return;
    const law = App.currentEditingLaw;

    const trigger = document.getElementById('eca-when-trigger').value;
    const activation = parseInt(document.getElementById('eca-when-activation').value, 10);
    const scope = parseInt(document.getElementById('eca-when-scope').value, 10);

    const condEnabled = document.getElementById('eca-cond-enable-toggle').checked;
    const condPath = document.getElementById('eca-cond-path').value.trim();
    const condOp = document.getElementById('eca-cond-op').value;
    const condVal = document.getElementById('eca-cond-val').value.trim();

    const actKind = document.getElementById('eca-act-kind').value;
    const actPath = document.getElementById('eca-act-path').value.trim();
    const actAmp = parseFloat(document.getElementById('eca-act-amp').value) || 1.0;
    const actFreq = parseFloat(document.getElementById('eca-act-freq').value) || 1.0;
    const actPhase = parseFloat(document.getElementById('eca-act-phase').value) || 0.0;
    const actOffset = parseFloat(document.getElementById('eca-act-offset').value) || 0.0;
    const actTimevar = document.getElementById('eca-act-timevar').value.trim() || 'time';

    const payload = {
        type: 'update_law_nodes',
        identifier: law.identifier,
        name: law.name || law.identifier,
        enabled: law.enabled !== undefined ? law.enabled : true,
        activation: activation,
        scope: scope,
        trigger: trigger,
        condition: {
            enabled: condEnabled,
            path: condPath,
            op: condOp,
            operand: isNaN(parseFloat(condVal)) ? condVal : parseFloat(condVal)
        },
        action: {
            kind: actKind,
            path: actPath,
            amplitude: actAmp,
            frequency: actFreq,
            phase: actPhase,
            offset: actOffset,
            timeVariable: actTimevar
        }
    };

    if (App.socket) {
        App.socket.emit('command', payload);
    }

    document.getElementById('eca-pipeline-modal').classList.remove('active');
    showToast(`Compiled & Applied ECA Node changes for @${law.identifier} in C++!`, "success");
    App.currentEditingLaw = null;
}

// ==========================================================================
// INTERCONNECTED LAW GRAPH ENGINE (Spacious, Clear Force/DAG Layout)
// ==========================================================================

const LawGraphEngine = {
    canvas: null,
    ctx: null,
    width: 800,
    height: 600,
    dpr: 1,

    nodes: [],
    links: [],
    nodeMap: new Map(),

    // Camera / Pan & Zoom
    panX: 0,
    panY: 0,
    zoomScale: 0.85,

    isDragging: false,
    dragNode: null,
    lastMouseX: 0,
    lastMouseY: 0,

    layoutMode: 'force',    // 'force' | 'flow' | 'clusters'
    spacingMode: 'wide',    // 'wide' | 'spacious' | 'compact'
    pulseTime: 0,

    init() {
        this.canvas = document.getElementById('law-graph-canvas');
        if (!this.canvas) return;
        this.ctx = this.canvas.getContext('2d');

        this.setupEvents();
        this.resize();
        this.startLoop();
    },

    resize() {
        const container = document.getElementById('law-graph-viewport');
        if (!container || !this.canvas) return;

        this.width = container.clientWidth || 800;
        this.height = container.clientHeight || 600;
        this.dpr = window.devicePixelRatio || 1;

        this.canvas.width = this.width * this.dpr;
        this.canvas.height = this.height * this.dpr;
        this.canvas.style.width = `${this.width}px`;
        this.canvas.style.height = `${this.height}px`;

        if (this.nodes.length > 0) {
            this.autoFit();
        } else {
            this.panX = this.width / 2;
            this.panY = this.height / 2;
        }
    },

    zoom(factor) {
        this.zoomScale = Math.max(0.2, Math.min(3.0, this.zoomScale * factor));
    },

    setSpacing(mode) {
        this.spacingMode = mode;
        this.applyInitialPositions();
    },

    autoFit() {
        if (this.nodes.length === 0) {
            this.panX = this.width / 2;
            this.panY = this.height / 2;
            this.zoomScale = 0.85;
            return;
        }

        let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
        this.nodes.forEach(n => {
            minX = Math.min(minX, n.x - (n.width || 48) / 2);
            maxX = Math.max(maxX, n.x + (n.width || 48) / 2);
            minY = Math.min(minY, n.y - (n.height || 48) / 2);
            maxY = Math.max(maxY, n.y + (n.height || 48) / 2);
        });

        const graphW = Math.max(100, maxX - minX);
        const graphH = Math.max(100, maxY - minY);
        const padding = 160;

        const scaleX = (this.width - padding) / graphW;
        const scaleY = (this.height - padding) / graphH;
        this.zoomScale = Math.min(1.0, Math.max(0.35, Math.min(scaleX, scaleY)));

        const centerX = (minX + maxX) / 2;
        const centerY = (minY + maxY) / 2;

        this.panX = this.width / 2 - centerX * this.zoomScale;
        this.panY = this.height / 2 - centerY * this.zoomScale;
    },

    resetView() {
        this.autoFit();
    },

    setLayout(mode) {
        this.layoutMode = mode;
        this.applyInitialPositions();
    },

    updateData(laws) {
        this.nodes = [];
        this.links = [];
        this.nodeMap.clear();

        const filterText = (App.lawSearchQuery || '').toLowerCase();
        const systemFilter = App.lawSystemFilter || 'all';

        laws.forEach((law, idx) => {
            const cat = categorizeLaw(law);
            if (systemFilter !== 'all' && cat.key !== systemFilter) return;

            const isMatch = (law.name || '').toLowerCase().includes(filterText) ||
                            (law.identifier || '').toLowerCase().includes(filterText);

            const cols = 3;
            const col = idx % cols;
            const row = Math.floor(idx / cols);

            const lawNode = {
                id: law.identifier,
                name: law.name || law.identifier,
                kind: 'law',
                law: law,
                system: cat.key,
                color: cat.color,
                enabled: law.enabled,
                activation: law.activation,
                expression: law.expression,
                isMatch: isMatch,
                width: 170,
                height: 52,
                x: (col - 1) * 340 + (Math.random() - 0.5) * 40,
                y: (row - 1) * 190 + (Math.random() - 0.5) * 40,
                vx: 0,
                vy: 0
            };

            this.nodes.push(lawNode);
            this.nodeMap.set(lawNode.id, lawNode);
        });

        laws.forEach(law => {
            if (!this.nodeMap.has(law.identifier)) return;
            const lawNode = this.nodeMap.get(law.identifier);
            const id = (law.identifier || '').toLowerCase();
            const expr = (law.expression || '').toLowerCase();

            // A. Trigger Connections
            let eventTrigger = null;
            if (law.activation === 1 || id.includes('acoustics') || id.includes('bounce') || expr.includes('contact')) {
                eventTrigger = 'contact-began';
            } else if (id.includes('mouse') || expr.includes('mouse')) {
                eventTrigger = 'onMouseClicked';
            } else if (id.includes('utterance') || expr.includes('utterance')) {
                eventTrigger = 'utterance-spoken';
            } else if (law.activation === 0 || law.activation === 2) {
                eventTrigger = 'universe.time';
            }

            if (eventTrigger) {
                const eventNodeId = `evt_${eventTrigger}`;
                let evtNode = this.nodeMap.get(eventNodeId);
                if (!evtNode) {
                    evtNode = {
                        id: eventNodeId,
                        name: eventTrigger,
                        kind: 'event',
                        color: '#10b981',
                        radius: 24,
                        x: -540 + (Math.random() - 0.5) * 60,
                        y: (this.nodes.length % 5 - 2) * 150,
                        vx: 0,
                        vy: 0
                    };
                    this.nodes.push(evtNode);
                    this.nodeMap.set(eventNodeId, evtNode);
                }
                this.links.push({
                    source: evtNode,
                    target: lawNode,
                    type: 'trigger',
                    color: '#10b981',
                    label: 'wakes'
                });
            }

            // B. Target Property Mutations
            let targetProp = null;
            if (id.includes('gravity') || id.includes('zero-g') || expr.includes('velocity')) {
                targetProp = '@beings.velocity';
            } else if (id.includes('kinematics') || id.includes('orbit') || expr.includes('position')) {
                targetProp = '@beings.position';
            } else if (id.includes('color') || id.includes('pulse') || expr.includes('color')) {
                targetProp = '@beings.color';
            } else if (id.includes('acoustics')) {
                targetProp = '@world.acoustics';
            }

            if (targetProp) {
                const targetNodeId = `target_${targetProp}`;
                let targetNode = this.nodeMap.get(targetNodeId);
                if (!targetNode) {
                    targetNode = {
                        id: targetNodeId,
                        name: targetProp,
                        kind: 'target',
                        color: '#38bdf8',
                        radius: 24,
                        x: 540 + (Math.random() - 0.5) * 60,
                        y: (this.nodes.length % 5 - 2) * 150,
                        vx: 0,
                        vy: 0
                    };
                    this.nodes.push(targetNode);
                    this.nodeMap.set(targetNodeId, targetNode);
                }
                this.links.push({
                    source: lawNode,
                    target: targetNode,
                    type: 'action',
                    color: '#38bdf8',
                    label: 'mutates'
                });
            }

            // C. Metalaw Connections
            if (id === 'law-zero-g' && this.nodeMap.has('physics-gravity')) {
                this.links.push({
                    source: lawNode,
                    target: this.nodeMap.get('physics-gravity'),
                    type: 'metalaw',
                    color: '#ec4899',
                    label: 'disables'
                });
            }
        });

        this.applyInitialPositions();
    },

    applyInitialPositions() {
        const mult = this.spacingMode === 'wide' ? 1.5 : (this.spacingMode === 'spacious' ? 1.2 : 0.9);

        if (this.layoutMode === 'flow') {
            const events = this.nodes.filter(n => n.kind === 'event');
            const laws = this.nodes.filter(n => n.kind === 'law');
            const targets = this.nodes.filter(n => n.kind === 'target');

            const eventGap = Math.max(150, 520 / Math.max(1, events.length)) * mult;
            const lawGap = Math.max(140, 640 / Math.max(1, laws.length)) * mult;
            const targetGap = Math.max(150, 520 / Math.max(1, targets.length)) * mult;

            events.forEach((n, i) => {
                n.x = -540 * mult;
                n.y = (i - (events.length - 1) / 2) * eventGap;
            });
            laws.forEach((n, i) => {
                n.x = 0;
                n.y = (i - (laws.length - 1) / 2) * lawGap;
            });
            targets.forEach((n, i) => {
                n.x = 540 * mult;
                n.y = (i - (targets.length - 1) / 2) * targetGap;
            });
        } else if (this.layoutMode === 'clusters') {
            const systemAngles = {
                physics: 0,
                acoustics: Math.PI * 0.33,
                visual: Math.PI * 0.66,
                motion: Math.PI * 1.0,
                creation: Math.PI * 1.33,
                input: Math.PI * 1.66,
                custom: Math.PI * 0.5
            };
            const R = 460 * mult;
            this.nodes.forEach(n => {
                if (n.kind === 'law') {
                    const angle = systemAngles[n.system] || 0;
                    n.x = Math.cos(angle) * R + (Math.random() - 0.5) * 60;
                    n.y = Math.sin(angle) * R + (Math.random() - 0.5) * 60;
                } else if (n.kind === 'event') {
                    n.x = -320 * mult;
                    n.y = (Math.random() - 0.5) * 320 * mult;
                } else {
                    n.x = 320 * mult;
                    n.y = (Math.random() - 0.5) * 320 * mult;
                }
            });
        }

        setTimeout(() => this.autoFit(), 60);
    },

    setupEvents() {
        if (!this.canvas) return;

        this.canvas.addEventListener('pointerdown', (e) => {
            const rect = this.canvas.getBoundingClientRect();
            const mouseX = (e.clientX - rect.left - this.panX) / this.zoomScale;
            const mouseY = (e.clientY - rect.top - this.panY) / this.zoomScale;

            const hit = this.findNodeAt(mouseX, mouseY);
            if (hit) {
                this.dragNode = hit;
                this.isDragging = true;
                this.selectNode(hit);
            } else {
                this.isDragging = true;
                this.dragNode = null;
            }

            this.lastMouseX = e.clientX;
            this.lastMouseY = e.clientY;
        });

        window.addEventListener('pointermove', (e) => {
            if (!this.isDragging) return;

            const dx = e.clientX - this.lastMouseX;
            const dy = e.clientY - this.lastMouseY;

            if (this.dragNode) {
                this.dragNode.x += dx / this.zoomScale;
                this.dragNode.y += dy / this.zoomScale;
            } else {
                this.panX += dx;
                this.panY += dy;
            }

            this.lastMouseX = e.clientX;
            this.lastMouseY = e.clientY;
        });

        window.addEventListener('pointerup', () => {
            this.isDragging = false;
            this.dragNode = null;
        });

        this.canvas.addEventListener('wheel', (e) => {
            e.preventDefault();
            const factor = e.deltaY < 0 ? 1.1 : 0.9;
            this.zoom(factor);
        }, { passive: false });
    },

    findNodeAt(x, y) {
        for (let i = this.nodes.length - 1; i >= 0; i--) {
            const n = this.nodes[i];
            if (n.kind === 'law') {
                const hw = n.width / 2;
                const hh = n.height / 2;
                if (x >= n.x - hw && x <= n.x + hw && y >= n.y - hh && y <= n.y + hh) {
                    return n;
                }
            } else {
                const dist = Math.hypot(x - n.x, y - n.y);
                if (dist <= n.radius) {
                    return n;
                }
            }
        }
        return null;
    },

    selectNode(node) {
        App.selectedLawGraphNode = node;
        const drawer = document.getElementById('graph-node-drawer');
        const titleEl = document.getElementById('drawer-node-title');
        const bodyEl = document.getElementById('drawer-node-body');

        if (!drawer || !titleEl || !bodyEl) return;

        drawer.classList.add('open');
        titleEl.innerText = node.name;

        if (node.kind === 'law') {
            const law = node.law;
            const cat = categorizeLaw(law);
            const isChecked = law.enabled ? 'checked' : '';
            const actLabel = law.activation === 1 ? 'OnEvent (Edge)' : (law.activation === 2 ? 'WhileTrue' : 'Continuous');

            bodyEl.innerHTML = `
                <div style="border-bottom: 1px solid var(--border-color); padding-bottom: 12px;">
                    <div style="font-size: 0.75rem; color: ${cat.color}; font-weight: 700; text-transform: uppercase;">
                        ${cat.icon} ${cat.name}
                    </div>
                    <div style="font-family: var(--font-mono); font-size: 0.8rem; color: var(--accent-cyan);">
                        @${law.identifier}
                    </div>
                </div>

                <div style="display: flex; justify-content: space-between; align-items: center;">
                    <span style="font-size: 0.85rem; font-weight: 600;">Active Status:</span>
                    <label class="toggle-switch">
                        <input type="checkbox" id="drawer-law-toggle" ${isChecked}>
                        <span class="toggle-slider"></span>
                    </label>
                </div>

                <button id="drawer-inspect-eca-btn" class="btn btn-primary btn-sm" style="width: 100%; justify-content: center; gap: 6px;">
                    <span>🧬 Inspect & Edit ECA Node Pipeline</span>
                </button>

                <div class="control-group">
                    <div class="control-label">Activation Mode</div>
                    <div style="font-size: 0.8rem; color: var(--text-secondary); font-family: var(--font-mono);">
                        ${actLabel}
                    </div>
                </div>

                <div class="control-group">
                    <div class="control-label">OntoMath Expression</div>
                    <div class="law-code-box" style="max-height: 120px;">
                        ${law.expression || 'authored_law_action()'}
                    </div>
                </div>

                <div class="control-group">
                    <div class="control-label">System Responsibilities</div>
                    <div style="font-size: 0.78rem; color: var(--text-secondary); line-height: 1.4;">
                        ${cat.desc}
                    </div>
                </div>
            `;

            document.getElementById('drawer-law-toggle')?.addEventListener('change', (e) => {
                toggleLaw(law.identifier, e.target.checked);
            });

            document.getElementById('drawer-inspect-eca-btn')?.addEventListener('click', () => {
                openEcaPipelineModal(law);
            });

        } else if (node.kind === 'event') {
            bodyEl.innerHTML = `
                <div style="font-size: 0.75rem; color: #10b981; font-weight: 700; text-transform: uppercase;">Event Signal Node</div>
                <div style="font-family: var(--font-mono); font-size: 0.95rem; color: #fff;">${node.name}</div>
                <div style="font-size: 0.8rem; color: var(--text-secondary); margin-top: 8px;">
                    Broadcast from C++ EventBus whenever state transitions or collisions occur in the world.
                </div>
            `;
        } else {
            bodyEl.innerHTML = `
                <div style="font-size: 0.75rem; color: #38bdf8; font-weight: 700; text-transform: uppercase;">Target State Property</div>
                <div style="font-family: var(--font-mono); font-size: 0.95rem; color: #fff;">${node.name}</div>
                <div style="font-size: 0.8rem; color: var(--text-secondary); margin-top: 8px;">
                    Property path governed and mutated by active laws during every simulation cycle.
                </div>
            `;
        }
    },

    startLoop() {
        const tick = () => {
            this.updatePhysics();
            this.render();
            requestAnimationFrame(tick);
        };
        requestAnimationFrame(tick);
    },

    updatePhysics() {
        if (this.layoutMode !== 'force') return;

        const mult = this.spacingMode === 'wide' ? 1.5 : (this.spacingMode === 'spacious' ? 1.15 : 0.85);
        const kRepel = 70000 * mult;
        const kSpring = 0.02;
        const desiredDist = 400 * mult;
        const damping = 0.85;

        for (let i = 0; i < this.nodes.length; i++) {
            const n1 = this.nodes[i];
            if (n1 === this.dragNode) continue;

            for (let j = i + 1; j < this.nodes.length; j++) {
                const n2 = this.nodes[j];
                const dx = n2.x - n1.x;
                const dy = n2.y - n1.y;
                const dist = Math.hypot(dx, dy) || 1;

                if (dist < 950) {
                    const force = kRepel / (dist * dist);
                    const fx = (dx / dist) * force;
                    const fy = (dy / dist) * force;

                    n1.vx -= fx;
                    n1.vy -= fy;
                    n2.vx += fx;
                    n2.vy += fy;
                }

                const r1 = n1.kind === 'law' ? 100 : 38;
                const r2 = n2.kind === 'law' ? 100 : 38;
                const minDist = (r1 + r2) * 1.35 * mult;

                if (dist < minDist) {
                    const overlap = minDist - dist;
                    const pushX = (dx / dist) * overlap * 0.45;
                    const pushY = (dy / dist) * overlap * 0.45;

                    if (n1 !== this.dragNode) { n1.x -= pushX; n1.y -= pushY; }
                    if (n2 !== this.dragNode) { n2.x += pushX; n2.y += pushY; }
                }
            }

            n1.vx -= n1.x * 0.0005;
            n1.vy -= n1.y * 0.0005;
        }

        this.links.forEach(l => {
            const dx = l.target.x - l.source.x;
            const dy = l.target.y - l.source.y;
            const dist = Math.hypot(dx, dy) || 1;
            const force = (dist - desiredDist) * kSpring;

            const fx = (dx / dist) * force;
            const fy = (dy / dist) * force;

            if (l.source !== this.dragNode) {
                l.source.vx += fx;
                l.source.vy += fy;
            }
            if (l.target !== this.dragNode) {
                l.target.vx -= fx;
                l.target.vy -= fy;
            }
        });

        this.nodes.forEach(n => {
            if (n === this.dragNode) return;
            n.vx *= damping;
            n.vy *= damping;
            n.x += n.vx;
            n.y += n.vy;
        });
    },

    requestRedraw() {
        this.render();
    },

    render() {
        if (!this.ctx || !this.canvas) return;
        const ctx = this.ctx;

        ctx.save();
        ctx.setTransform(this.dpr, 0, 0, this.dpr, 0, 0);
        ctx.clearRect(0, 0, this.width, this.height);

        ctx.strokeStyle = 'rgba(255, 255, 255, 0.03)';
        ctx.lineWidth = 1;
        const gridSize = 45 * this.zoomScale;
        const startX = (this.panX % gridSize);
        const startY = (this.panY % gridSize);

        for (let x = startX; x < this.width; x += gridSize) {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, this.height);
            ctx.stroke();
        }
        for (let y = startY; y < this.height; y += gridSize) {
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(this.width, y);
            ctx.stroke();
        }

        ctx.translate(this.panX, this.panY);
        ctx.scale(this.zoomScale, this.zoomScale);

        this.pulseTime += 0.03;

        this.links.forEach(link => {
            this.renderLink(ctx, link);
        });

        this.nodes.forEach(node => {
            this.renderNode(ctx, node);
        });

        ctx.restore();
    },

    renderLink(ctx, link) {
        const s = link.source;
        const t = link.target;

        const isSelected = App.selectedLawGraphNode && (App.selectedLawGraphNode === s || App.selectedLawGraphNode === t);

        ctx.strokeStyle = isSelected ? '#00f0ff' : link.color;
        ctx.lineWidth = isSelected ? 2.5 : 1.5;
        ctx.globalAlpha = isSelected ? 1.0 : 0.45;

        if (link.type === 'metalaw') {
            ctx.setLineDash([6, 6]);
        } else {
            ctx.setLineDash([]);
        }

        const midX = (s.x + t.x) / 2;
        const midY = (s.y + t.y) / 2;

        ctx.beginPath();
        ctx.moveTo(s.x, s.y);
        ctx.quadraticCurveTo(midX, midY - 20, t.x, t.y);
        ctx.stroke();
        ctx.setLineDash([]);

        const pulseEnabled = document.getElementById('graph-pulse-edges')?.checked !== false;
        if (pulseEnabled) {
            const p = (this.pulseTime + (s.x * 0.01)) % 1.0;
            const px = (1 - p) * (1 - p) * s.x + 2 * (1 - p) * p * midX + p * p * t.x;
            const py = (1 - p) * (1 - p) * s.y + 2 * (1 - p) * p * (midY - 20) + p * p * t.y;

            ctx.fillStyle = isSelected ? '#00f0ff' : link.color;
            ctx.shadowColor = ctx.fillStyle;
            ctx.shadowBlur = 8;
            ctx.beginPath();
            ctx.arc(px, py, 3.5, 0, Math.PI * 2);
            ctx.fill();
            ctx.shadowBlur = 0;
        }

        ctx.globalAlpha = 1.0;
    },

    renderNode(ctx, node) {
        const isSelected = App.selectedLawGraphNode === node;

        if (node.kind === 'law') {
            const w = node.width;
            const h = node.height;
            const x = node.x - w / 2;
            const y = node.y - h / 2;
            const r = 8;

            ctx.fillStyle = isSelected ? '#151c36' : (node.enabled ? '#0e1224' : '#070914');
            ctx.strokeStyle = isSelected ? '#00f0ff' : (node.enabled ? node.color : '#334155');
            ctx.lineWidth = isSelected ? 2.5 : 1.5;

            if (isSelected) {
                ctx.shadowColor = '#00f0ff';
                ctx.shadowBlur = 16;
            } else if (node.enabled) {
                ctx.shadowColor = node.color;
                ctx.shadowBlur = 6;
            }

            ctx.beginPath();
            ctx.roundRect(x, y, w, h, r);
            ctx.fill();
            ctx.stroke();
            ctx.shadowBlur = 0;

            ctx.fillStyle = node.color;
            ctx.beginPath();
            ctx.roundRect(x, y, w, 4, [r, r, 0, 0]);
            ctx.fill();

            ctx.fillStyle = node.enabled ? '#10b981' : '#64748b';
            ctx.beginPath();
            ctx.arc(x + 12, y + h / 2 + 2, 4.5, 0, Math.PI * 2);
            ctx.fill();

            ctx.fillStyle = node.enabled ? '#ffffff' : '#94a3b8';
            ctx.font = '600 11px Inter, sans-serif';
            ctx.textAlign = 'left';
            ctx.fillText(node.name.length > 17 ? node.name.substring(0, 16) + '…' : node.name, x + 24, y + 23);

            ctx.fillStyle = node.color;
            ctx.font = '500 9px "JetBrains Mono", monospace';
            ctx.fillText(`@${node.id.length > 19 ? node.id.substring(0, 18) + '…' : node.id}`, x + 24, y + 38);

        } else if (node.kind === 'event') {
            ctx.fillStyle = isSelected ? '#064e3b' : '#022c22';
            ctx.strokeStyle = isSelected ? '#34d399' : '#10b981';
            ctx.lineWidth = isSelected ? 2.5 : 1.5;

            ctx.beginPath();
            ctx.arc(node.x, node.y, node.radius, 0, Math.PI * 2);
            ctx.fill();
            ctx.stroke();

            ctx.fillStyle = '#6ee7b7';
            ctx.font = '600 9px "JetBrains Mono", monospace';
            ctx.textAlign = 'center';
            ctx.fillText(node.name.replace('contact-', '').replace('on', ''), node.x, node.y + 3);

        } else {
            ctx.fillStyle = isSelected ? '#1e293b' : '#0f172a';
            ctx.strokeStyle = isSelected ? '#38bdf8' : '#64748b';
            ctx.lineWidth = isSelected ? 2.5 : 1.5;

            ctx.beginPath();
            ctx.arc(node.x, node.y, node.radius, 0, Math.PI * 2);
            ctx.fill();
            ctx.stroke();

            ctx.fillStyle = '#94a3b8';
            ctx.font = '500 8px "JetBrains Mono", monospace';
            ctx.textAlign = 'center';
            ctx.fillText(node.name.replace('@beings.', ''), node.x, node.y + 3);
        }
    }
};

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
        App.socket.emit('command', {
            type: 'switch_zone',
            index: index
        });
        showToast(`Switched active Zone to [${index}]`, "success");
    }
}

// --- Studio 4: Logos & Utterance Modality ---
function initLogosConsole() {
    const emitBtn = document.getElementById('logos-emit-btn');
    const inputEl = document.getElementById('logos-input-text');

    const doEmit = () => {
        const text = inputEl.value.trim();
        if (!text) return;

        fetch('/api/utterance', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ utterance: text })
        }).then(() => {
            addTerminalMessage('Player', text);
            inputEl.value = '';
        });
    };

    if (emitBtn) emitBtn.addEventListener('click', doEmit);
    if (inputEl) {
        inputEl.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') doEmit();
        });
    }

    document.querySelectorAll('.preset-chip').forEach(chip => {
        chip.addEventListener('click', () => {
            const word = chip.getAttribute('data-word');
            if (inputEl) {
                inputEl.value = word;
                doEmit();
            }
        });
    });
}

function addTerminalMessage(speaker, text) {
    const term = document.getElementById('logos-terminal');
    if (!term) return;

    const timeStr = new Date().toLocaleTimeString();
    const msg = document.createElement('div');
    msg.className = 'terminal-msg';
    msg.innerHTML = `
        <span class="msg-time">[${timeStr}]</span>
        <span class="msg-speaker">&lt;${speaker}&gt;</span>
        <span class="msg-text">${escapeHtml(text)}</span>
    `;
    term.appendChild(msg);
    term.scrollTop = term.scrollHeight;
}

// --- Studio 5: Person & Physics ---
function initPhysicsControls() {
    const tpSpawnBtn = document.getElementById('tp-spawn');
    const tpOriginBtn = document.getElementById('tp-origin');
    const tpSkyBtn = document.getElementById('tp-sky');
    const tpSanctumBtn = document.getElementById('tp-sanctum');

    if (tpSpawnBtn) tpSpawnBtn.addEventListener('click', () => teleportPlayer([0, 1.8, 5]));
    if (tpOriginBtn) tpOriginBtn.addEventListener('click', () => teleportPlayer([0, 1.8, 0]));
    if (tpSkyBtn) tpSkyBtn.addEventListener('click', () => teleportPlayer([0, 25.0, 0]));
    if (tpSanctumBtn) tpSanctumBtn.addEventListener('click', () => teleportPlayer([-10, 3.0, 10]));

    const customTpBtn = document.getElementById('tp-custom-btn');
    if (customTpBtn) {
        customTpBtn.addEventListener('click', () => {
            const x = parseFloat(document.getElementById('tp-custom-x').value) || 0;
            const y = parseFloat(document.getElementById('tp-custom-y').value) || 0;
            const z = parseFloat(document.getElementById('tp-custom-z').value) || 0;
            teleportPlayer([x, y, z]);
        });
    }

    const flyingToggle = document.getElementById('physics-flying-toggle');
    if (flyingToggle) {
        flyingToggle.addEventListener('change', (e) => {
            setPhysics({ flying: e.target.checked });
        });
    }

    const gravVizToggle = document.getElementById('physics-gravviz-toggle');
    if (gravVizToggle) {
        gravVizToggle.addEventListener('change', (e) => {
            setPhysics({ gravity_viz: e.target.checked });
        });
    }
}

function renderPersonPhysics() {
    if (!App.state.player) return;
    const pos = App.state.player.position || [0, 1.8, 5];
    const posDisplay = document.getElementById('player-pos-display');
    if (posDisplay) {
        posDisplay.innerText = `[${pos[0].toFixed(2)}, ${pos[1].toFixed(2)}, ${pos[2].toFixed(2)}]`;
    }

    // Kinetic velocity & speed computation
    const lastPos = App.telemetry.lastPlayerPos;
    const vx = (pos[0] - lastPos[0]) * 5.0; // simulated smoothed m/s
    const vy = (pos[1] - lastPos[1]) * 5.0;
    const vz = (pos[2] - lastPos[2]) * 5.0;
    const speed = Math.hypot(vx, vy, vz);
    App.telemetry.speed = speed;
    App.telemetry.lastPlayerPos = [...pos];

    const speedValEl = document.getElementById('kinetic-speed-val');
    const vxEl = document.getElementById('vector-vx');
    const vyEl = document.getElementById('vector-vy');
    const vzEl = document.getElementById('vector-vz');

    if (speedValEl) speedValEl.innerText = speed.toFixed(2);
    if (vxEl) vxEl.innerText = vx.toFixed(2);
    if (vyEl) vyEl.innerText = vy.toFixed(2);
    if (vzEl) vzEl.innerText = vz.toFixed(2);

    const flying = App.state.physics?.flying || false;
    const stateBadge = document.getElementById('vessel-substrate-state');
    if (stateBadge) {
        stateBadge.innerText = flying ? 'AIRBORNE / FLIGHT' : 'GROUNDED';
        stateBadge.style.color = flying ? 'var(--accent-cyan)' : 'var(--accent-emerald)';
        stateBadge.style.background = flying ? 'rgba(0, 240, 255, 0.15)' : 'rgba(16, 185, 129, 0.15)';
    }

    const flyingToggle = document.getElementById('physics-flying-toggle');
    if (flyingToggle && App.state.physics) {
        flyingToggle.checked = flying;
    }

    const gravVizToggle = document.getElementById('physics-gravviz-toggle');
    if (gravVizToggle && App.state.physics) {
        gravVizToggle.checked = App.state.physics.gravity_viz || false;
    }

    // Check if Zero-G is active
    const zeroGLaw = (App.state.laws || []).find(l => l.identifier === 'law-zero-g');
    const isZeroG = zeroGLaw && zeroGLaw.enabled;
    const gravValEl = document.getElementById('gravity-vector-val');
    if (gravValEl) {
        gravValEl.innerText = isZeroG ? '0.00 m/s² (Zero-G)' : '-9.81 m/s² (Gravity)';
        gravValEl.style.color = isZeroG ? 'var(--accent-cyan)' : 'var(--accent-amber)';
    }
}

function teleportPlayer(pos) {
    if (App.socket) {
        App.socket.emit('command', {
            type: 'teleport_player',
            position: pos
        });
        showToast(`Warped Vessel to [${pos.map(v => v.toFixed(1)).join(', ')}]`, "success");
    }
}

function setPhysics(opts) {
    fetch('/api/physics', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(opts)
    }).then(() => showToast("Physics Settings Updated", "success"));
}

// --- Studio 6: Robotics Modality ---
function initRoboticsPanel() {
    document.querySelectorAll('#tab-robotics input[type="range"]').forEach(slider => {
        slider.addEventListener('input', () => {
            const jointProp = slider.getAttribute('data-joint');
            const val = parseFloat(slider.value);
            
            const valLabel = document.getElementById(`${slider.id}-val`);
            if (valLabel) valLabel.innerText = `${val}°`;

            if (App.socket) {
                App.socket.emit('command', {
                    type: 'property_write',
                    target: '@player',
                    property: jointProp,
                    value: val
                });
            }

            updateRoboticsTelemetry();
        });
    });
}

function updateRoboticsTelemetry() {
    const j1 = parseFloat(document.getElementById('rob-joint-1')?.value || 0);
    const j2 = parseFloat(document.getElementById('rob-joint-2')?.value || 45);
    const j3 = parseFloat(document.getElementById('rob-joint-3')?.value || 30);

    // Simulate torque loads
    const t1 = Math.abs(j1 * 0.08).toFixed(1);
    const t2 = (8.0 + Math.abs(j2 * 0.15)).toFixed(1);
    const t3 = (4.0 + Math.abs(j3 * 0.1)).toFixed(1);

    const t1Text = document.getElementById('rob-torque-1-text');
    const t2Text = document.getElementById('rob-torque-2-text');
    const t3Text = document.getElementById('rob-torque-3-text');

    if (t1Text) t1Text.innerText = `${t1} N·m`;
    if (t2Text) t2Text.innerText = `${t2} N·m`;
    if (t3Text) t3Text.innerText = `${t3} N·m`;

    const t1Bar = document.getElementById('rob-torque-1');
    const t2Bar = document.getElementById('rob-torque-2');
    const t3Bar = document.getElementById('rob-torque-3');

    if (t1Bar) t1Bar.style.width = `${Math.min(100, t1 * 5)}%`;
    if (t2Bar) t2Bar.style.width = `${Math.min(100, t2 * 4)}%`;
    if (t3Bar) t3Bar.style.width = `${Math.min(100, t3 * 6)}%`;

    // Forward Kinematics TCP Coordinates approximation
    const rad1 = (j1 * Math.PI) / 180;
    const rad2 = (j2 * Math.PI) / 180;
    const l1 = 0.5, l2 = 0.4;
    const reach = l1 * Math.cos(rad2) + l2 * Math.cos(rad2 + (j3 * Math.PI) / 180);
    const x = (Math.cos(rad1) * reach).toFixed(2);
    const y = (l1 * Math.sin(rad2) + l2 * Math.sin(rad2 + (j3 * Math.PI) / 180) + 0.3).toFixed(2);
    const z = (Math.sin(rad1) * reach).toFixed(2);

    const tcpEl = document.getElementById('rob-tcp-coords');
    if (tcpEl) tcpEl.innerText = `[${x}, ${y}, ${z}] m`;
}

// --- Studio 7: Live Event Stream ---
function initEventLogStream() {
    const pauseBtn = document.getElementById('pause-event-log-btn');
    if (pauseBtn) {
        pauseBtn.addEventListener('click', () => {
            App.eventLogPaused = !App.eventLogPaused;
            pauseBtn.innerText = App.eventLogPaused ? "Resume Stream" : "Pause Stream";
        });
    }

    const clearBtn = document.getElementById('clear-event-log-btn');
    if (clearBtn) {
        clearBtn.addEventListener('click', () => {
            App.eventLog = [];
            const listEl = document.getElementById('events-stream-list');
            if (listEl) listEl.innerHTML = '';
        });
    }
}

function addEventToLog(eventData) {
    if (App.eventLogPaused) return;

    const listEl = document.getElementById('events-stream-list');
    if (!listEl) return;

    const timeStr = new Date().toLocaleTimeString();
    const item = document.createElement('div');
    item.className = 'terminal-msg';
    
    item.innerHTML = `
        <span class="msg-time">[${timeStr}]</span>
        <span class="msg-speaker">&lt;${eventData.event || 'EngineEvent'}&gt;</span>
        <span class="msg-text">${escapeHtml(JSON.stringify(eventData))}</span>
    `;

    listEl.appendChild(item);
    listEl.scrollTop = listEl.scrollHeight;
}

// --- Saves Fetcher ---
async function fetchSaves() {
    try {
        const res = await fetch('/api/saves');
        if (res.ok) {
            const data = await res.json();
            const listEl = document.getElementById('saves-list');
            if (listEl && data.saves) {
                listEl.innerHTML = '';
                if (data.saves.length === 0) {
                    listEl.innerHTML = '<div style="padding: 10px; color: var(--text-muted); font-size: 0.8rem;">No saved reality files found</div>';
                    return;
                }
                data.saves.forEach(s => {
                    const item = document.createElement('div');
                    item.className = 'object-item';
                    item.innerHTML = `
                        <div class="object-name-tag">
                            <span>📄 ${s.name}</span>
                        </div>
                        <span style="color: var(--text-muted); font-size: 0.72rem; font-family: var(--font-mono);">${(s.size/1024).toFixed(1)} KB</span>
                    `;
                    listEl.appendChild(item);
                });
            }
        }
    } catch (e) {
        console.warn("Failed to fetch saves", e);
    }
}

// --- Helpers ---
function getShapeName(kind) {
    const names = ['Cube', 'Polyhedron', 'Sphere', 'Cylinder', 'Cone', 'Ellipsoid', 'Ovoid', 'Paraboloid', 'Torus', 'RoundedBox', 'Shape2D', 'Text2D', 'Field', 'Patch'];
    return names[kind] || 'Cube';
}

function rgbToHex(r, g, b) {
    const toHex = c => Math.round(c * 255).toString(16).padStart(2, '0');
    return `#${toHex(r)}${toHex(g)}${toHex(b)}`;
}

function hexToRgb(hex) {
    const bigint = parseInt(hex.slice(1), 16);
    return [((bigint >> 16) & 255) / 255, ((bigint >> 8) & 255) / 255, (bigint & 255) / 255];
}

function escapeHtml(str) {
    return (str || '').replace(/[&<>"']/g, m => ({
        '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;'
    }[m]));
}

function showToast(message, type = 'info') {
    const container = document.getElementById('toast-container');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `
        <span>${type === 'success' ? '✓' : (type === 'error' ? '⚠' : 'ℹ')}</span>
        <span>${escapeHtml(message)}</span>
    `;

    container.appendChild(toast);
    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateX(100%)';
        setTimeout(() => toast.remove(), 250);
    }, 3500);
}
