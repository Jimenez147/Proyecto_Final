//esclavo ready 

#include <WiFi.h>
#include <PubSubClient.h>

// Configuración de Wi-Fi
const char* ssid = "ok";           
const char* password = "12345678";  
const int output_pin = 27;
const int output_pin2 = 2;

// Configuración del broker MQTT
const char* mqttserver = "broker.hivemq.com";
const int mqttport = 1883;
const char* topic = "valvula";       
const char* clientId = "clientId-TytxkfoJ9R";  

WiFiClient espClient;
PubSubClient client(espClient);

// Variable para controlar el estado de conexión
bool fullyConnected = false;
unsigned long previousMillis = 0;
const long blinkInterval = 200;  

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (message == "1") {
    digitalWrite(output_pin, LOW);
    Serial.println("Pin activado");
  } else if (message == "2") {
    digitalWrite(output_pin, HIGH);
    Serial.println("Pin desactivado");
  }
}

void updateStatusLED() {
  if (WiFi.status() == WL_CONNECTED && client.connected()) {
    if (!fullyConnected) {
      fullyConnected = true;
      digitalWrite(output_pin2, HIGH);  
    }
  } else {
    fullyConnected = false;
    // Hacer parpadear el LED cuando no está conectado
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      digitalWrite(output_pin2, !digitalRead(output_pin2));  
  }
}
}

void setup() {
  Serial.begin(115200);
  
  pinMode(output_pin, OUTPUT);
  pinMode(output_pin2, OUTPUT);
 
  digitalWrite(output_pin, LOW);
//  digitalWrite(output_pin, HIGH);
//  delay(1000);
//  digitalWrite(output_pin, LOW);
//  delay(1000);
//  digitalWrite(output_pin, HIGH);
//  delay(1000);
//  digitalWrite(output_pin, LOW);
//  delay(1000);
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    updateStatusLED();  // Actualiza el LED mientras intenta conectarse
  }
  Serial.println("\nConectado a WiFi");
  
  // Configuración MQTT
  client.setServer(mqttserver, mqttport);
  client.setCallback(callback);  
  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  updateStatusLED();  
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    updateStatusLED(); 
    if (client.connect(clientId)) {
      Serial.println("Conectado al broker MQTT");
      client.subscribe(topic);  
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Reintentando en 2 segundos");
      delay(2000);
    }
  }
}
