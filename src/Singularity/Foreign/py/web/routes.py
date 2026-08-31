from flask import Blueprint, render_template

web_bp = Blueprint('web', __name__)

@web_bp.route('/')
@web_bp.route('/objects')
@web_bp.route('/laws')
@web_bp.route('/zones')
@web_bp.route('/logos')
@web_bp.route('/physics')
@web_bp.route('/robotics')
@web_bp.route('/events')
def home():
    return render_template('index.html')
