#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Gau In T3";
const char* password = "unin2611";
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// Định nghĩa các chân kết nối
#define TRIG_PIN 26 
#define ECHO_PIN 27
#define PIR_PIN 21
#define BUZZER_PIN 17
#define LED_PIN 15

// Biến toàn cục
bool systemOn = false;  // Hệ thống bật hay tắt
bool intruderDetected = false;  // Đã phát hiện xâm nhập chưa
long duration;
int distance;
bool motionDetected = false;

// Thiết lập kết nối WiFi
void setupWifi() {
  delay(10);
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

// Kết nối đến MQTT Broker
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT Broker!");
      client.subscribe("Security/control");  // Đăng ký nhận lệnh điều khiển
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(". Try again in 2 seconds.");
      delay(2000);
    }
  }
}

// Callback khi nhận được lệnh điều khiển từ MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String command = "";
  for (int i = 0; i < length; i++) {
    command += (char)payload[i];
  }

  Serial.print("Command received: ");
  Serial.println(command);

  if (command == "OFF") {
    systemOn = false;  // Tắt hệ thống
    Serial.println("System OFF");
    digitalWrite(LED_PIN, LOW);  // Tắt LED
    digitalWrite(BUZZER_PIN, LOW);  // Tắt buzzer
  } else if (command == "ON") {
    systemOn = true;  // Bật hệ thống
    Serial.println("System ON");
  }
}

// Thiết lập kết nối ban đầu
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  setupWifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);  // Đăng ký callback
  reconnectMQTT();
}

// Đo khoảng cách từ cảm biến HC-SR04
long measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;  // Tính khoảng cách (cm)
}

// Hàm xử lý trong vòng lặp chính
void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Kiểm tra trạng thái hệ thống
  if (!systemOn) {
    // Nếu hệ thống tắt, tắt buzzer và LED, và không kiểm tra cảm biến
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    return;  // Dừng tiếp tục kiểm tra
  }

  // Đo khoảng cách và phát hiện chuyển động
  distance = measureDistance();
  motionDetected = digitalRead(PIR_PIN);

  if (motionDetected && distance < 50) {
    intruderDetected = true;  // Đã phát hiện xâm nhập
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    client.publish("Security/warning", "Intruder detected!");
  } else {
    intruderDetected = false;  // Không phát hiện xâm nhập
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
  }

  // Gửi dữ liệu khoảng cách và trạng thái chuyển động lên MQTT
  char distanceStr[8];
  snprintf(distanceStr, sizeof(distanceStr), "%d", distance);
  client.publish("Security/distance", distanceStr);
  
  if (motionDetected) {
    client.publish("Security/motion", "Motion detected");
  } else {
    client.publish("Security/motion", "No motion");
  }

  delay(1000);  // Đợi 1 giây trước khi đo lại
}
