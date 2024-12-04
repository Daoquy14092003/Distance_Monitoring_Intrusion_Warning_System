#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Gau In T3";
const char* password = "unin2611";
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

#define TRIG_PIN 26 
#define ECHO_PIN 27
#define PIR_PIN 21
#define BUZZER_PIN 17
#define LED_PIN 15

long duration;
int distance;
bool motionDetected = false;

void setupWifi() {
  delay(10);
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttemptTime >= 10000) {  // Giới hạn thời gian chờ kết nối
      Serial.println("Failed to connect to WiFi within 10 seconds");
      return;
    }
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {  // Không cần username/password
      Serial.println("Connected to MQTT Broker!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(". Try again in 2 seconds.");
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0'; 
  String message = String((char*)payload);

  if (strcmp(topic, "Security/reset") == 0) { 
    if (message == "ALERT_OFF") {
      // Tắt LED và Buzzer
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Received ALERT_OFF: LED and Buzzer turned OFF");
    }
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  setupWifi();

  client.setServer(mqttServer, mqttPort);
  reconnectMQTT();  // Kết nối MQTT
  client.setCallback(callback); 
  client.subscribe("Security/reset"); 
}

long measureDistance() {
  long sum = 0;
  for (int i = 0; i < 5; i++) {  // Lấy trung bình 5 lần đo
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH);
    sum += duration * 0.034 / 2;  // Tính khoảng cách (cm)
    delay(50);  // Thời gian trễ giữa các lần đo
  }
  return sum / 5;  // Trả về giá trị trung bình
}

unsigned long previousMillis = 0;
const long interval = 1000;

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Đo khoảng cách và phát hiện chuyển động
    distance = measureDistance();
    motionDetected = digitalRead(PIR_PIN);

    if (motionDetected && distance < 50) {
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);
      client.publish("Security/warning", "Intruder detected!");
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
    }

    // Gửi dữ liệu khoảng cách và trạng thái chuyển động lên MQTT
    char distanceStr[8];
    snprintf(distanceStr, sizeof(distanceStr), "%d", distance);
    client.publish("Security/distance", String(distance).c_str());

    if (motionDetected) {
      client.publish("Security/motion", "Motion detected");
    } else {
      client.publish("Security/motion", "No motion");
    }
  }

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}
