import json
import socket
import threading
import datetime
from collections import deque
from flask import Blueprint, jsonify

api = Blueprint("api", __name__, url_prefix="/api")

leituras = deque(maxlen=50)
ultima_leitura = None
lock = threading.Lock()

def parsear_coap(dados: bytes) -> dict | None:
    if len(dados) < 4:
        return None
    try:
        tkl = dados[0] & 0x0F
        pos = 4 + tkl

        while pos < len(dados):
            if dados[pos] == 0xFF:
                pos += 1
                break
            delta = (dados[pos] >> 4) & 0x0F
            length = dados[pos] & 0x0F
            pos += 1
            if delta == 13:
                pos += 1
            elif delta == 14:
                pos += 2
            if length == 13:
                pos += 1
            elif length == 14:
                pos += 2
            pos += length

        payload_bytes = dados[pos:]
        if not payload_bytes:
            return None
        return json.loads(payload_bytes.decode("utf-8"))

    except Exception as e:
        print(f"[CoAP] Erro ao parsear: {e}")
        return None

def escutar_coap():
    global ultima_leitura

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", 5683))
    print("[CoAP] Escutando na porta UDP 5683...")

    while True:
        dados, endereco = sock.recvfrom(1024)
        print(f"[RAW] {len(dados)} bytes de {endereco}: {dados.hex()}")

        payload = parsear_coap(dados)
        if payload is None:
            try:
                payload = json.loads(dados.decode("utf-8"))
            except Exception:
                print(f"[UDP] Pacote inválido de {endereco}")
                continue

        temp = payload.get("temperatura", 999)
        umid = payload.get("umidade", 999)
        if temp > 60 or temp < -10 or umid > 99 or umid < 1:
            print(f"[UDP] Leitura fora do range, descartando: {payload}")
            continue

        payload["timestamp"] = datetime.datetime.now().strftime("%H:%M:%S")
        payload["ip"] = endereco[0]

        with lock:
            leituras.appendleft(payload)
            ultima_leitura = payload

        print(f"[UDP] {payload['timestamp']} | Temp: {temp}°C | Umid: {umid}%")

        resposta = bytes([0x60, 0x44, dados[2], dados[3]])
        sock.sendto(resposta, endereco)

def iniciar_udp():
    t = threading.Thread(target=escutar_coap, daemon=True)
    t.start()

@api.route("/monitoring", methods=["GET"])
def api_get_monitoring():
    with lock:
        return jsonify({"ultima": ultima_leitura, "historico": list(leituras)})
