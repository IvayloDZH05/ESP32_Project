#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid = "Nothing phone (1)";
const char* password = "ivoRD2005";
const char* mqttServer = "192.168.182.94";
const char* mqttTopic = "servo_control";

WiFiClient espClient;
PubSubClient client(espClient);

Servo servo1;
Servo servo2;

struct ServoControl {
    Servo* servo;           // Pointer to servo
    int currentAngle;       // Current angle of the servo
    int targetAngle;        // Target angle to reach
    unsigned long lastUpdate;  // Last update time
    int updateInterval;     // Time between updates in milliseconds
};

ServoControl servoControls[2];

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
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

void moveServo(ServoControl &servoControl) {
    if (millis() - servoControl.lastUpdate > servoControl.updateInterval) {
        if (servoControl.currentAngle < servoControl.targetAngle) {
            servoControl.currentAngle++;
            servoControl.servo->write(servoControl.currentAngle);
        } else if (servoControl.currentAngle > servoControl.targetAngle) {
            servoControl.currentAngle--;
            servoControl.servo->write(servoControl.currentAngle);
        }
        servoControl.lastUpdate = millis();
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String msgString = "";
  for (int i = 0; i < length; i++) {
    msgString += (char)payload[i];
  }
  Serial.println(msgString);
  
  int indexOfColon = msgString.indexOf(':');
  if (indexOfColon != -1) {
    int servoId = msgString.substring(0, indexOfColon).toInt() - 1; // assuming servoId is either 1 or 2
    int targetAngle = msgString.substring(indexOfColon + 1).toInt();
    
    if (servoId == 0 || servoId == 1) {
      servoControls[servoId].targetAngle = targetAngle;
    } else {
      Serial.println("Error: Invalid servo ID");
    }
  } else {
    Serial.println("Error: ':' not found in message");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += WiFi.macAddress();
    if (client.connect(clientId.c_str())) {
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
  setup_wifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  servo1.attach(33); // GPIO pin where servo 1 is connected
  servo2.attach(14); // GPIO pin where servo 2 is connected

  servoControls[0] = {&servo1, 90, 90, millis(), 15}; // initialize servo1 to mid position 90 degrees
  servoControls[1] = {&servo2, 90, 90, millis(), 15}; // initialize servo2 to mid position 90 degrees
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Continuously update servo positions smoothly
  moveServo(servoControls[0]);
  moveServo(servoControls[1]);
}
