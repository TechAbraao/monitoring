from flask import Blueprint,render_template
from app.utils import envs

web = Blueprint("web", __name__, url_prefix="/")

@web.route("/")
def dashboard():
    return render_template("base.jinja2", esp32_url_monitoring=envs.ESP32_URL_MONITORING)
