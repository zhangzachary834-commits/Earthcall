// Earthcall Client Bridge
// Connects the HTML/JS frontend (and SocketIO) to the C++ Wasm engine

let socket = null;

window.initSocketIO = function() {
    console.log("Connecting to Python Backend via SocketIO...");
    socket = io(); // Connects to the same host that served the page

    socket.on("connect", () => {
        console.log("Connected to Multiplayer Hub (Socket ID:", socket.id + ")");
        
        // Let the C++ engine know we connected (example)
        if (Module && Module.receive_from_js) {
            Module.receive_from_js("Network", JSON.stringify({ type: "connected", id: socket.id }));
        }
    });

    socket.on("disconnect", () => {
        console.log("Disconnected from Multiplayer Hub");
    });

    // Listen for state syncs from the server
    socket.on("state_sync", (data) => {
        if (Module && Module.receive_from_js) {
            // Forward network events into the C++ engine via our embind function
            Module.receive_from_js("Network", JSON.stringify({ type: "state_sync", data: data }));
        }
    });
};

// Expose a function for C++ to call when it wants to send data to the server
// e.g. EM_ASM({ window.earthcall_receive(...) })
window.earthcall_receive = function(messageJsonStr) {
    try {
        const msg = JSON.parse(messageJsonStr);
        if (msg.type === "law_action") {
            // Send the law action to the Python backend
            if (socket) socket.emit("law_action", msg);
        } else {
            console.log("JS received message from C++:", msg);
            // Fallback for general networking
            if (socket) socket.emit("cpp_message", msg);
        }
    } catch (e) {
        console.error("Failed to parse message from C++:", messageJsonStr);
    }
};
