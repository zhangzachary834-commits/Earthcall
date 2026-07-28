from flask import Flask, render_template, jsonify
import os

app = Flask(__name__, static_url_path='/static')

# Serve HTML page
@app.route('/')
def home():
    return render_template('index.html')

# API Route for JavaScript
@app.route('/api/message')
def get_message():
    return jsonify({"message": "Hello from Flask! 🚀🔥"})

if __name__ == '__main__':
    # debug=True enables the Werkzeug interactive debugger, which executes
    # arbitrary code from anyone who can reach the port. Opt in explicitly for
    # local work (EARTHCALL_DEBUG=1) rather than shipping it on by default.
    app.run(debug=os.environ.get('EARTHCALL_DEBUG') == '1')
