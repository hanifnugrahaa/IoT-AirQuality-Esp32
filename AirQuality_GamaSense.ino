#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ********************
// SETTINGS
// ********************

// WiFi settings
const char* ssid = "Mlue";
const char* password = "putrabaja99";

// MQTT settings
const char* mqtt_server = "broker.hivemq.com";
const char* topic = "airquality/gamasense";

// LED & BUZZER pins
const int ledRed = 15;
const int ledGreen = 2;
const int ledBlue = 18;
const int buzzer = 5;

// DHT11 settings
#define DHTPIN 19      
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQ-7 pin
const int mq7Pin = 4;

// Timer
unsigned long connectStartTime;

WiFiClient espClient;
PubSubClient client(espClient);

// ***********************************
//   WIFI CONNECTION WITH LED LOGIC
// ***********************************
void setupWiFi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    // RED LED blinking
    digitalWrite(ledRed, HIGH);
    delay(300);
    digitalWrite(ledRed, LOW);
    delay(300);

    Serial.println("Connecting to WiFi...");

    // Buzzer if >10s
    if (millis() - connectStartTime > 10000) {
      digitalWrite(buzzer, HIGH);
    }
  }

  Serial.println("WiFi connected!");
  digitalWrite(buzzer, LOW);
}

// ***********************************
// MQTT RECONNECT LOGIC
// ***********************************
void reconnect() {
  while (!client.connected()) {

    // WiFi OK but MQTT fail → BLUE ON
    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(ledBlue, HIGH);
      digitalWrite(ledGreen, LOW);
      digitalWrite(ledRed, LOW);
    }

    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT connected!");

      digitalWrite(ledGreen, HIGH);
      digitalWrite(ledRed, LOW);
      digitalWrite(ledBlue, LOW);
      digitalWrite(buzzer, LOW);

    } else {
      delay(1000);

      if (millis() - connectStartTime > 10000) {
        digitalWrite(buzzer, HIGH);
      }
    }
  }
}

// ***********************************
// REAL SENSOR READINGS
// ***********************************

float readMQ7() {
  int adc = analogRead(mq7Pin);
  float voltage = adc * (3.3 / 4095.0);   // ADC → Volt

  // kamu bisa kirim ADC langsung, atau voltase
  return voltage;  
}

float readTemperature() {
  return dht.readTemperature();
}

float readHumidity() {
  return dht.readHumidity();
}

// ***********************************
// SETUP
// ***********************************
void setup() {
  Serial.begin(115200);

  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(mq7Pin, INPUT);

  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledBlue, LOW);
  digitalWrite(buzzer, LOW);

  connectStartTime = millis();

  dht.begin();
  setupWiFi();

  client.setServer(mqtt_server, 1883);
}

// ***********************************
// LOOP (SENDING DATA)
// ***********************************
void loop() {
  if (!client.connected()) {
    reconnect();
  }

  // Final LED conditions
  if (WiFi.status() == WL_CONNECTED && client.connected()) {
    digitalWrite(ledGreen, HIGH);
    digitalWrite(ledRed, LOW);
    digitalWrite(ledBlue, LOW);
    digitalWrite(buzzer, LOW);
  }

  client.loop();

  // Send every 10 seconds
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 300000) {

    float co = readMQ7();
    float temp = readTemperature();
    float humidity = readHumidity();

    String payload = "{";
    payload += "\"co\":" + String(co);
    payload += ",\"temperature\":" + String(temp);
    payload += ",\"humidity\":" + String(humidity);
    payload += "}";

    client.publish(topic, payload.c_str());
    Serial.println("Data sent: " + payload);

    lastSend = millis();
  }
}
