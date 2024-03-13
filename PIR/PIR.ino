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
      servo1.write(angle);
    } else if (servoId == 2) {
      servo2.write(angle);
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
    
    // Generate unique client ID based on ESP32's MAC address
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
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
  servo1.attach(33); // GPIO pin where servo 1 signal is connected
  servo2.attach(14); // GPIO pin where servo 2 signal is connected
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
  
