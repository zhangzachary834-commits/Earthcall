function fetchBackend() {
    console.log("🔥 fetchBackend() triggered!");

    // .json(), not .text(): /api/message returns a JSON object, and reading
    // .message off the raw response STRING was undefined every time — the page
    // rendered the word "undefined" on every click.
    fetch("/api/message")
        .then(response => {
            if (!response.ok) {
                throw new Error(`${response.status} ${response.statusText}`);
            }
            return response.json();
        })
        .then(data => {
            console.log("✅ Response:", data);
            document.getElementById("response").innerText = data.message;
        })
        .catch(error => {
            console.error("Error:", error);
            document.getElementById("response").innerText = `Error: ${error.message}`;
        });
}
