from flask import Flask, render_template, jsonify, send_from_directory
import os

app = Flask(__name__, static_url_path='/static') 

# Serve HTML page
@app.route('/')
def home():
    return render_template('index.html'

# API Route for JavaScript
@app.route('/api/message')
def get_message():
    return jsonify({"message": "Hello from Flask! 🚀🔥"})

if __name__ == '__main__':
    app.run(debug=True)