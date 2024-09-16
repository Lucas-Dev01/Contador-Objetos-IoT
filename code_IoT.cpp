#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Espalexa.h>

// Definição dos pinos
#define TRIGGER_PIN 5
#define ECHO_PIN 18
#define BUZZER_PIN 15
#define LED_PIN_RED 2
#define LED_PIN_GREEN 4

const char* ssid = "your_ssid";
const char* password = "your_password";

WebServer server(81);  // Usando a porta 81 para evitar conflitos com Espalexa
Espalexa espalexa;

int count = 0;
bool isObjectDetected = false;
bool readyToCount = true;
bool buzzerActivated = false;
bool contagemAtiva = false;
const int limiteObjetos = 10;
unsigned long ledTimer = 0;
const unsigned long ledInterval = 500;
bool ignoreFirstDetection = false; // Ignorar a primeira detecção após iniciar contagem

// Função para iniciar a contagem com indicação visual
void iniciarContagem(uint8_t brightness) {
  // Piscar LEDs verde e vermelho uma vez para indicar que a contagem foi iniciada
  digitalWrite(LED_PIN_GREEN, HIGH);
  digitalWrite(LED_PIN_RED, HIGH);
  delay(500);  // Mantém os LEDs acesos por 500ms
  digitalWrite(LED_PIN_GREEN, LOW);
  digitalWrite(LED_PIN_RED, LOW);
  
  contagemAtiva = true;
  ignoreFirstDetection = true; // Configura para ignorar a primeira detecção
  Serial.println("Contagem Iniciada");
}

void pararContagem(uint8_t brightness) {
  contagemAtiva = false;
  Serial.println("Contagem Parada");
}

void resetarContagem(uint8_t brightness) {
  count = 0;
  buzzerActivated = false;
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN_RED, LOW);
  Serial.println("Contagem Resetada");
}

void handleRoot() {
  // HTML com CSS aprimorado para uma interface mais moderna e profissional
  String html = "<!DOCTYPE html><html><head><title>Contador de Objetos</title>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f4f4f9; color: #333; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; }";
  html += ".container { background: #ffffff; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); border-radius: 10px; padding: 20px; text-align: center; max-width: 400px; width: 90%; }";
  html += "h1 { color: #333; margin-bottom: 20px; }";
  html += ".counter { font-size: 64px; color: #4CAF50; margin: 20px 0; font-weight: bold; }";
  html += ".button { background-color: #008CBA; border: none; color: white; padding: 15px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin-top: 20px; cursor: pointer; border-radius: 5px; transition: background-color 0.3s ease; }";
  html += ".button:hover { background-color: #005f6a; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Contagem de Objetos</h1>";
  html += "<div class='counter'><span id='count'>0</span></div>";
  html += "<button class='button' onclick=\"location.href='/reset'\">Resetar Contagem</button>";
  html += "</div>";
  html += "<script>setInterval(() => {fetch('/count').then(res => res.text()).then(data => {";
  html += "document.getElementById('count').innerText = data;});}, 1000);</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleCount() {
  server.send(200, "text/plain", String(count));
}

void handleReset() {
  count = 0;
  buzzerActivated = false;
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN_RED, LOW);
  Serial.println("Contagem resetada!");
  server.send(200, "text/html", "<html><body><h1>Contagem resetada!</h1><a href='/'>Voltar</a></body></html>");
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_GREEN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN_RED, LOW);
  digitalWrite(LED_PIN_GREEN, LOW);

  // Conectar ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando à rede Wi-Fi...");
  }
  Serial.println("Conectado à rede Wi-Fi!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP()); // Exibe o endereço IP no monitor serial

  // Configura Espalexa
  espalexa.addDevice("Iniciar Contagem", iniciarContagem);
  espalexa.addDevice("Parar Contagem", pararContagem);
  espalexa.addDevice("Resetar Contagem", resetarContagem);
  espalexa.begin();

  // Configura o servidor web na porta 81
  server.on("/", handleRoot);
  server.on("/count", handleCount);
  server.on("/reset", handleReset);
  server.begin();
  Serial.println("Servidor web iniciado na porta 81.");
}

void loop() {
  espalexa.loop();
  long duration, distance;
  
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) / 29.1;

  if (contagemAtiva && distance > 0 && distance < 20) {
    // Ignora a primeira detecção logo após iniciar a contagem
    if (ignoreFirstDetection) {
      ignoreFirstDetection = false;
      return; // Sai da função para não contar na primeira detecção
    }
    
    if (!isObjectDetected && readyToCount) {
      isObjectDetected = true;
      readyToCount = false;
      count++;
      digitalWrite(LED_PIN_GREEN, HIGH);
      Serial.print("Objetos detectados: ");
      Serial.println(count);
      
      if(count > limiteObjetos && !buzzerActivated) {
        tone(BUZZER_PIN, 1000);
        buzzerActivated = true;
      }
    }
  } else {
    if (isObjectDetected) {
      isObjectDetected = false;
      readyToCount = true;
      digitalWrite(LED_PIN_GREEN, LOW);
    }
  }

  if (count > limiteObjetos) {
    if (millis() - ledTimer >= ledInterval) {
      ledTimer = millis();
      digitalWrite(LED_PIN_RED, !digitalRead(LED_PIN_RED));
    }
  } else {
    digitalWrite(LED_PIN_RED, LOW);
  }

  server.handleClient();
  delay(100);
}
