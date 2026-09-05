/**
 * Earthcall LawGraphEngine — Spacious Interconnected Causation Graph
 * Features generous spacing, dynamic repulsion, layout modes (Force, Flow, Clusters),
 * and live auto-fit camera viewport.
 */
window.LawGraphEngine = {
    canvas: null,
    ctx: null,
    width: 800,
    height: 600,
    dpr: 1,
    nodes: [],
    links: [],
    nodeMap: new Map(),
    // Camera Viewport
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
        const filterText = (window.App?.lawSearchQuery || '').toLowerCase();
        const systemFilter = window.App?.lawSystemFilter || 'all';
        // 1. Create Spaced-out Law Nodes
        laws.forEach((law, idx) => {
            const cat = window.categorizeLaw ? window.categorizeLaw(law) : { key: 'custom', color: '#10b981' };
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
        // 2. Derive Event & Target Nodes & Interconnections
        laws.forEach(law => {
            if (!this.nodeMap.has(law.identifier)) return;
            const lawNode = this.nodeMap.get(law.identifier);
            const id = (law.identifier || '').toLowerCase();
            const expr = (law.expression || '').toLowerCase();
            // Trigger Event Node
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
            // Target Property Node
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
            // Metalaw Connection
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
        window.App.selectedLawGraphNode = node;
        const drawer = document.getElementById('graph-node-drawer');
        const titleEl = document.getElementById('drawer-node-title');
        const bodyEl = document.getElementById('drawer-node-body');
        if (!drawer || !titleEl || !bodyEl) return;
        drawer.classList.add('open');
        titleEl.innerText = node.name;
        if (node.kind === 'law') {
            const law = node.law;
            const cat = window.categorizeLaw ? window.categorizeLaw(law) : { color: '#10b981', icon: '⚡', name: 'Law', desc: '' };
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
                if (window.toggleLaw) window.toggleLaw(law.identifier, e.target.checked);
            });
            document.getElementById('drawer-inspect-eca-btn')?.addEventListener('click', () => {
                if (window.openEcaPipelineModal) window.openEcaPipelineModal(law);
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
        // Wide Repulsion
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
                // Explicit non-overlap collision separation
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
            // Gentle center gravity
            n1.vx -= n1.x * 0.0005;
            n1.vy -= n1.y * 0.0005;
        }
        // Link spring forces
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
        // Damped velocities
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
        const isSelected = window.App?.selectedLawGraphNode && (window.App.selectedLawGraphNode === s || window.App.selectedLawGraphNode === t);
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
        const isSelected = window.App?.selectedLawGraphNode === node;
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
EOF
ls -lh src/Singularity/Foreign/py/static/js/graph_engine.js