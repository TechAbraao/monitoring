#include <WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiUdp.h>

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// DHT
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Luminosity
#define LDR_PIN 34  // GPIO34 — pino analógico (ADC) do LDR

// UDP (substitui CoAP)
WiFiUDP udp;

// Gateway Wokwi (OBRIGATÓRIO)
const char* serverIP = "10.13.37.1";  // IP do gateway Wokwi Private
const int serverPort = 5683;           // Porta padrão CoAP/UDP

// Timer — controla o intervalo de envio UDP (10s)
unsigned long lastSendTime = 0;
unsigned long sendInterval = 10000;

// Servidor HTTP na porta 80 — serve apenas a API JSON
WiFiServer server(80);


// ===============================
// ENVIO UDP (CoAP simplificado)
// ===============================
void sendData(String payload) {
  Serial.println("ENVIANDO UDP...");
  Serial.print("Destino: ");
  Serial.print(serverIP);
  Serial.print(":");
  Serial.println(serverPort);

  // Envia o payload JSON via UDP para o gateway Wokwi
  udp.beginPacket(serverIP, serverPort);
  udp.write((uint8_t*)payload.c_str(), payload.length());
  udp.endPacket();

  Serial.println("Payload enviado:");
  Serial.println(payload);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Inicializa o display OLED via I2C no endereço 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erro OLED");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();

  // Conecta ao Wi-Fi da rede Wokwi (sem senha)
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");

  // Configura o pino do LDR como entrada analógica
  pinMode(LDR_PIN, INPUT);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi OK");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  unsigned long now = millis();

  // Verifica se há um cliente HTTP conectado
  WiFiClient client = server.available();

  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET /esp32/monitoring") >= 0) {
      // Rota da API — retorna JSON com temperatura, umidade e luminosidade
      float temp = dht.readTemperature();
      float hum  = dht.readHumidity();
      int light = analogRead(LDR_PIN);  // Leitura ADC: 0 (escuro) a 4095 (claro)

      String json = "{\"temperatura\":" + String(temp, 2) +
              ",\"umidade\":"     + String(hum,  2) +
              ",\"luminosidade\":" + String(light) + "}";

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Access-Control-Allow-Origin: *");
      client.println("Connection: close");
      client.println();
      client.println(json);
    }

    client.stop();
  }

  // Bloco de envio UDP — dispara a cada 10 segundos
  if (now - lastSendTime > sendInterval) {
    lastSendTime = now;

    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();
    int light = analogRead(LDR_PIN);

    // Ignora leituras inválidas do DHT22
    if (isnan(temp) || isnan(hum)) {
      Serial.println("Erro DHT22");
      return;
    }

    String json = "{\"temperatura\":" + String(temp, 2) +
              ",\"umidade\":"     + String(hum,  2) +
              ",\"luminosidade\":" + String(light) + "}";

    sendData(json);

    // Atualiza o display OLED com temperatura e umidade
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("Temperatura:");
    display.setTextSize(2);
    display.print(temp, 1);
    display.println(" C");

    display.setTextSize(1);
    display.println("Umidade:");
    display.setTextSize(2);
    display.print(hum, 1);
    display.println(" %");

    display.display();
  }
}