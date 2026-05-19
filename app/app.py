from app import create_app
from app.resources.api import iniciar_udp

app = create_app()

if __name__ == "__main__":
    iniciar_udp()
    app.run(host="0.0.0.0", port=5000, debug=False)
