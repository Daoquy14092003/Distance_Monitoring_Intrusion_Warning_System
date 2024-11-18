#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Gau In T3";
const char* password = "unin2611";
const char* mqttServer = "YOUR_MQTT_BROKER";
const int mqttPort = 1883;
const char* mqttUser = "YOUR_MQTT_USER";
const char* mqttPassword = "YOUR_MQTT_PASSWORD";

WiFiClient espClient;
PubSubClient client(espClient);

#define TRIG_PIN 12
#define ECHO_PIN 13
#define PIR_PIN 27
#define BUZZER_PIN 14
#define LED_PIN 26

long duration;
int distance;
bool motionDetected = false;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT Broker!");
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  motionDetected = digitalRead(PIR_PIN);

  if (motionDetected && distance < 50) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    client.publish("home/security/alert", "Intruder detected!");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
  }

  // Send distance and motion status to MQTT
  char distanceStr[8];
  snprintf(distanceStr, sizeof(distanceStr), "%d", distance);
  client.publish("home/security/distance", distanceStr);
  
  if (motionDetected) {
    client.publish("home/security/motion", "Motion detected");
  } else {
    client.publish("home/security/motion", "No motion");
  }

  delay(1000); // Adjust delay as necessary
}
