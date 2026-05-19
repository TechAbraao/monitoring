from dataclasses import dataclass
from dotenv import load_dotenv
import os

@dataclass
class Environment:
    ESP32_URL_MONITORING = os.getenv("ESP32_URL_MONITORING")
