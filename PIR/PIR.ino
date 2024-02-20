  #include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid = "NET1";
const char* password = "803-_-308";
const char* mqttServer = "192.168.1.100";
const char* mqttTopic = "servo_control";

WiFiClient espClient;
PubSubClient client(espClient);

Servo servo1;
Servo servo2;

const int servo1Pin = 13; // GPIO pin where servo 1 signal is connected
const int servo2Pin = 14; // GPIO pin where servo 2 signal is connected

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String msgString = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msgString += (char)payload[i];
  }
  Serial.println();

  int indexOfColon = msgString.indexOf(':');
  if (indexOfColon != -1) {
    int servoId = msgString.substring(0, indexOfColon).toInt();
    int angle = msgString.substring(indexOfColon + 1).toInt();

    // Check servo ID and control corresponding servo
    if (servoId == 1) {
      moveServoGradually(servo1, angle);
    } else if (servoId == 2) {
      moveServoGradually(servo2, angle);
    } else {
      Serial.println("Error: Invalid servo ID");
    }
  } else {
    Serial.println("Error: ':' not found in message");
  }
}

void moveServoGradually(Servo &servo, int targetPos) {
  int currentPos = servo.read();
  if (currentPos == targetPos) {
    return; // If already at the target position, no need to move
  }

  int step = (targetPos > currentPos) ? 1 : -1; // Determine the direction of movement

  for (int pos = currentPos; pos != targetPos; pos += step) {
    servo.write(pos); // Move the servo to the next position
    delay(15); // Adjust the delay based on how fast you want the servo to move
  }

  servo.write(targetPos); // Ensure the servo reaches the exact target position
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
  
