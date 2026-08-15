document.addEventListener('DOMContentLoaded', () => {
    const form = document.getElementById('utterance-form');
    const inputField = document.getElementById('utterance-input');
    const statusText = document.getElementById('status-text');
    const statusContainer = document.getElementById('connection-status');
    const logContainer = document.getElementById('log-container');
    
    let ws = null;
    let clientId = "client_" + Math.random().toString(36).substr(2, 9);
    
    function setStatus(text, state) {
        statusText.innerText = text;
        statusContainer.className = 'status-indicator'; // Reset
        if (state) statusContainer.classList.add(state);
    }

    function addLog(message, type = 'system') {
        const entry = document.createElement('div');
        entry.className = `log-entry ${type}`;
        
        const timestamp = new Date().toLocaleTimeString([], { hour12: false });
        entry.innerHTML = `<span style="opacity:0.5; margin-right:8px;">[${timestamp}]</span> ${message}`;
        
        logContainer.appendChild(entry);
        logContainer.scrollTop = logContainer.scrollHeight;
    }
    
    function connectWebSocket() {
        setStatus("Connecting...", "connecting");
        
        // Connect to Earthcall Native WebSocket Server
        ws = new WebSocket('ws://localhost:8080');
        
        ws.onopen = () => {
            console.log("[Earthcall] WebSocket Connected");
            setStatus("Connected", "connected");
            addLog("Successfully connected to Earthcall Engine.", "system");
        };
        
        ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                // Handle different incoming message types
                if (data.type === 'utterance_response' || data.type === 'event') {
                    addLog(`Engine: ${data.payload || JSON.stringify(data)}`, 'server');
                } else {
                    addLog(`Received: ${event.data}`, 'server');
                }
            } catch (e) {
                addLog(`Received raw data: ${event.data}`, 'server');
            }
        };

        ws.onclose = () => {
            console.warn("[Earthcall] WebSocket Disconnected. Retrying in 2s...");
            setStatus("Disconnected", "error");
            addLog("Connection lost. Reconnecting...", "system");
            setTimeout(connectWebSocket, 2000);
        };
        
        ws.onerror = (err) => {
            console.error("[Earthcall] WebSocket Error:", err);
            setStatus("Error", "error");
        };
    }
    
    // Start Connection
    connectWebSocket();
    
    // Handle ConstructedBeing Submission
    form.addEventListener('submit', (e) => {
        e.preventDefault();
        
        const text = inputField.value.trim();
        if (!text) return;
        
        // Add to log visually
        addLog(`You uttered: "${text}"`, 'user');
        
        // Send via WebSocket
        if (ws && ws.readyState === WebSocket.OPEN) {
            const payload = {
                type: "utterance",
                payload: text,
                sourceClient: clientId
            };
            ws.send(JSON.stringify(payload));
        } else {
            addLog("Error: Engine is not connected. Cannot emit.", "system");
        }
        
        // Clear input
        inputField.value = '';
        inputField.focus();
    });
});
