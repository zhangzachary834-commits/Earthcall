document.addEventListener('DOMContentLoaded', () => {
    const inputField = document.getElementById('utterance-input');
    const emitBtn = document.getElementById('emit-btn');
    const statusText = document.getElementById('status-text');
    const statusContainer = document.getElementById('connection-status');
    
    let ws = null;
    let clientId = "client_" + Math.random().toString(36).substr(2, 9);
    
    // Check if we are running under Emscripten (WASM Mode)
    const isWasmMode = typeof Module !== 'undefined' && Module.Earthcall_EmitUtterance;
    
    function setStatus(text, isConnected) {
        statusText.innerText = text;
        if (isConnected) {
            statusContainer.classList.add('connected');
        } else {
            statusContainer.classList.remove('connected');
        }
    }
    
    if (isWasmMode) {
        setStatus("WASM Attached", true);
        console.log("[Earthcall] Running in WASM Mode (Zero-Latency)");
    } else {
        // Native Network Mode
        connectWebSocket();
    }
    
    function connectWebSocket() {
        setStatus("Connecting to Native Engine...", false);
        ws = new WebSocket('ws://localhost:8080');
        
        ws.onopen = () => {
            console.log("[Earthcall] WebSocket Connected");
            setStatus("Native Server Connected", true);
        };
        
        ws.onclose = () => {
            console.warn("[Earthcall] WebSocket Disconnected. Retrying in 2s...");
            setStatus("Disconnected. Retrying...", false);
            setTimeout(connectWebSocket, 2000);
        };
        
        ws.onerror = (err) => {
            console.error("[Earthcall] WebSocket Error:", err);
        };
    }
    
    function emitUtterance() {
        const text = inputField.value.trim();
        if (!text) return;
        
        console.log(`[Earthcall] Emitting: "${text}"`);
        
        if (isWasmMode) {
            // Path B: Embind directly to C++
            Module.Earthcall_EmitUtterance(text, clientId);
        } else if (ws && ws.readyState === WebSocket.OPEN) {
            // Path A: WebSocket to Native Server
            const payload = {
                type: "utterance",
                payload: text,
                sourceClient: clientId
            };
            ws.send(JSON.stringify(payload));
        } else {
            console.error("[Earthcall] Engine is not connected.");
        }
        
        inputField.value = '';
    }
    
    emitBtn.addEventListener('click', emitUtterance);
    
    inputField.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            emitUtterance();
        }
    });

    inputField.addEventListener('input', () => {
        emitBtn.disabled = inputField.value.trim() === '';
    });
});
