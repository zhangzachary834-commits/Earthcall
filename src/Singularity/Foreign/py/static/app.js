function fetchBackend() {
    console.log("🔥 fetchBackend() triggered!");

    fetch("/api/message")
        .then(response => response.text())
        .then(data => {
            console.log("✅ Response:", data);
            document.getElementById("response").innerText = data.message;
        })
        .catch(error => console.error("Error:", error));
}