from flask import Blueprint, jsonify

api_bp = Blueprint('api', __name__, url_prefix='/api')

@api_bp.route('/message')
def get_message():
    return jsonify({"message": "Hello from Flask! 🚀🔥"})
