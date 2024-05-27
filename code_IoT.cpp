#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// Definição dos pinos
#define TRIGGER_PIN 5     // Pino TRIG do Sensor Ultrassônico
#define ECHO_PIN 18       // Pino ECHO do Sensor Ultrassônico
#define BUZZER_PIN 15     // Pino para o Buzzer
#define LED_PIN_RED 2     // Pino para o LED vermelho
#define LED_PIN_GREEN 4   // Pino para o LED verde

const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";

WebServer server(80);

int count = 0;
bool isObjectDetected = false;
bool readyToCount = true;
bool buzzerActivated = false; // Estado do buzzer
const int limiteObjetos = 10; // Limite de objetos para ativar o buzzer

void handleRoot() {
  String html = "<html><body><h1>Contagem de Objetos</h1>";
  html += "<p>Objetos detectados: " + String(count) + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(9600); // Inicia a comunicação Serial
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_GREEN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW); // Garante que o buzzer comece desligado
  digitalWrite(LED_PIN_RED, LOW);
  digitalWrite(LED_PIN_GREEN, LOW);

  // Conecta à rede Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando à rede Wi-Fi...");
  }
  Serial.println("Conectado à rede Wi-Fi!");

  // Configura o servidor web
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Servidor web iniciado.");
}

void loop() {
  long duration, distance;
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) / 29.1; // Calcula a distância em centímetros

  if (distance > 0 && distance < 20) {
    if (!isObjectDetected && readyToCount) {
      isObjectDetected = true;
      readyToCount = false;
      count++;
      digitalWrite(LED_PIN_GREEN, HIGH); // Acende LED verde ao detectar objeto

      Serial.print("Objetos detectados: ");
      Serial.println(count);
      
      if(count >= limiteObjetos && !buzzerActivated) {
        digitalWrite(LED_PIN_RED, HIGH); // Acende LED vermelho ao atingir limite
        tone(BUZZER_PIN, 1000); // Ativa o buzzer continuamente
        buzzerActivated = true; // Marca que o buzzer foi ativado
      }
    }
  } else {
    if (isObjectDetected) {
      isObjectDetected = false;
      readyToCount = true;
      digitalWrite(LED_PIN_GREEN, LOW); // Apaga LED verde
    }
  }

  server.handleClient(); // Processa as requisições do servidor web

  delay(100); // Pequena pausa para estabilização
}
