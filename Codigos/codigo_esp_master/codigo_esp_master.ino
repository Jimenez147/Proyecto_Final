#include <WiFi.h>
#include <PubSubClient.h>

// Configuración de Wi-Fi
const char* ssid = "ok";           
const char* password = "12345678";  

// Configuración del sensor
const int mq9_pin = 34;  // pin para el sensor
const int valve_pin = 2; 
const float GAS_THRESHOLD = 7.0; 
float sen_value = 0;
float Volt = 0;
const float Max_v = 3.3;
const float adc_res = 4095.0;

// Configuración del broker MQTT
const char* mqttserver = "broker.hivemq.com";
const int mqttport = 1883;
const char* topic = "gass1";       
const char* valveTopic = "valvula"; 

const char* clientId = "clientId-OV7iHs15By";  

// variable de control 
bool fullyConnected = false;
bool gasDetected = false;
unsigned long previousMillis = 0;
const long blinkInterval = 500;  // intervalo de alerta
unsigned long lastPublish = 0;
const long publishInterval = 5000;
bool lastGasState = false;

WiFiClient espClient;
PubSubClient client(espClient);

void updateValvePin() {
  if (!fullyConnected) {
    // si no tenemos conesion apagado 
    digitalWrite(valve_pin, LOW);
    return;
  }
  
  if (gasDetected) {
    // Si hay gas, hacer parpadear
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      digitalWrite(valve_pin, !digitalRead(valve_pin));
    }
  } else {
    // si se conecto mantenerlo encendido 
    
    digitalWrite(valve_pin, HIGH);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (String(topic) == valveTopic) {
    if (message == "1") {
      gasDetected = true;
      Serial.println("Válvula activada manualmente");
    } else if (message == "2") {
      gasDetected = false;
      Serial.println("Válvula desactivada manualmente");
    }
  }
}

float calcularPorcentajeGas(float voltaje) {
  float RS = ((Max_v * 10.0)/voltaje) - 10.0; 
  float R0 = 6.5;  
  float ratio = RS/R0;
  
  float porcentaje = 100.0 * (1.0 - (ratio / 10.0));
  if (porcentaje < 0) porcentaje = 0;
  if (porcentaje > 100) porcentaje = 100;
  
  return porcentaje;
}

void setup() {
  Serial.begin(115200);
  
  pinMode(valve_pin, OUTPUT);
  digitalWrite(valve_pin, LOW);
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    updateValvePin();  
  }
  Serial.println("\nConectado a WiFi");
  
  // Configuración MQTT
  client.setServer(mqttserver, mqttport);
  client.setCallback(callback);  
  reconnect();
  
  Serial.println("Iniciando lectura");
  Serial.println("Calibrando Sensor...");
  delay(20000);
}

void loop() {
  if (!client.connected()) {
    fullyConnected = false;
    reconnect();
  } else if (WiFi.status() == WL_CONNECTED) {
    fullyConnected = true;
  }
  
  client.loop();
  
  unsigned long now = millis();
  
  // Leer sensor
  sen_value = 0;
  for(int i = 0; i < 10; i++) {
    sen_value += analogRead(mq9_pin);
    delay(100);
  }
  sen_value = sen_value / 10.0;
  Volt = (sen_value * Max_v) / adc_res;
  
  float gasPercentage = calcularPorcentajeGas(Volt);
  
  gasDetected = (gasPercentage > GAS_THRESHOLD);
  
  if (gasDetected) {
    if (!lastGasState) {
      client.publish(valveTopic, "1");
      Serial.println("¡Alerta! Nivel de gas superior al 3% - Válvula activada");
    }
  } else {
    if (lastGasState) {
      client.publish(valveTopic, "2");
      Serial.println("Enviando notificación de nivel normal");
    }
  }
  
  lastGasState = gasDetected;
  
  if (now - lastPublish >= publishInterval) {
    lastPublish = now;
    
    char msg[10];
    dtostrf(gasPercentage, 4, 2, msg);
    
    client.publish(topic, msg);
    
    Serial.print("ADC: ");
    Serial.print(sen_value);
    Serial.print(" | Volt: ");
    Serial.print(Volt, 3);
    Serial.print("V | Gas: ");
    Serial.print(gasPercentage);
    Serial.println("%");
  }
  
  updateValvePin();  
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    updateValvePin();  
    if (client.connect(clientId)) {
      Serial.println("Conectado al broker MQTT");
      client.subscribe(valveTopic);
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Reintentando en 2 segundos");
      delay(2000);
    }
  }
}
