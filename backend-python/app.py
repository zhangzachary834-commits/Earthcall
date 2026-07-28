import os

from flask import Flask, jsonify, render_template

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
    # debug=True was hard-coded here. It enables the Werkzeug debugger, which
    # hands an interactive Python console to anyone who can reach the port and
    # provoke a traceback — remote code execution, not a convenience. It is now
    # opt-in, and the server binds loopback rather than every interface.
    debug = os.environ.get('EARTHCALL_DEBUG', '').lower() in ('1', 'true', 'yes')
    app.run(host='127.0.0.1', port=int(os.environ.get('PORT', '5000')), debug=debug)
