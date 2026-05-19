from app.resources.api import api
from app.resources.web import web
from flask import Flask

def create_app() -> Flask:
    app = Flask(__name__)
    
    app.register_blueprint(api)
    app.register_blueprint(web)
    
    return app
