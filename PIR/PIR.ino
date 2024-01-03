#include <WiFi.h>
#include <PubSubClient.h>

const char *ssid = "Rado's WIFI";
const char *password = "NePipaiNeta6496";
const char *mqttServer = "192.168.0.112";
const int mqttPort = 1883;
const char *mqttMotionTopic = "motion-detected";
const char *mqttStatusTopic = "esp32-cam-status";

const int pirPin = 19;  // Replace with the pin to which your PIR sensor is connected
const int ledPin = 21;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMotionTime = 0;  // Variable to store the time of the last motion detection
const unsigned long motionCooldown = 5000;  // Cooldown period in milliseconds (adjust as needed)
bool esp32CAMActive = true;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Set up PIR sensor
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Set up MQTT
  client.setServer(mqttServer, mqttPort);
    if (client.connect("PIRClient")) {
    Serial.println("Connected to MQTT broker");

    // Subscribe to the topic
    if (client.subscribe(mqttMotionTopic)) {
    Serial.println("Subscribed to motion topic: " + String(mqttMotionTopic));
  } else {
    Serial.println("Failed to subscribe to motion topic");
  }
  
  if (client.subscribe(mqttStatusTopic)) {
    Serial.println("Subscribed to status topic: " + String(mqttStatusTopic));
  } else {
    Serial.println("Failed to subscribe to status topic");
  }
  
  // Blink the LED to indicate successful MQTT broker connection
  blinkLED(ledPin, 3);  // Blink three times
  } else {
    Serial.println("Failed to connect to MQTT broker");
  }
}

void loop() {
  int pirState = digitalRead(pirPin);
  unsigned long currentTime = millis();

  if (pirState == HIGH && (currentTime - lastMotionTime > motionCooldown)) {
    Serial.println("Motion detected!");
    sendMotionMessage();  // Trigger sending a message to ESP32-CAM
    lastMotionTime = currentTime;  // Update the last motion time
    delay(motionCooldown);  // Cooldown period
  }

  client.loop();
  delay(100); // Adjust delay as needed for your application
}

void sendMotionMessage() {
  if (client.connected()) {
    Serial.println("Sending motion message to ESP32-CAM");
    if (client.publish(mqttMotionTopic, "Motion Detected!")) {
      Serial.println("Published motion message successfully");
      // Blink the LED to indicate successful motion message publication
      blinkLED(ledPin, 1);  // Blink once
    } else {
      Serial.println("Failed to publish motion message");
    }
  } else {
    Serial.println("Not connected to MQTT broker. Reconnecting...");
    // Attempt to reconnect to the MQTT broker
    reconnect();
  }
}

void blinkLED(int pin, int times) {
  for (int i = 0; i < times; ++i) {
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
    delay(500);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("PIRClient")) {
      Serial.println("connected");
      // Subscribe to the topic
      if (client.subscribe(mqttMotionTopic)) {
      Serial.println("Subscribed to motion topic: " + String(mqttMotionTopic));
    } else {
      Serial.println("Failed to subscribe to motion topic");
    }
    if (client.subscribe(mqttStatusTopic)) {
      Serial.println("Subscribed to status topic: " + String(mqttStatusTopic));
    } else {
      Serial.println("Failed to subscribe to status topic");
    }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Add code here to handle deep sleep initiation (e.g., send a message to ESP32-CAM)
void handleDeepSleep() {
  Serial.println("Going to sleep now");
  delay(1000);

  // Check MQTT connection status before sending sleep notification
  if (client.connected()) {
    // Add code here to send an MQTT message to notify ESP32-CAM before deep sleep
    notifyESP32CAMBeforeSleep();
  } else {
    Serial.println("Not connected to MQTT broker. Reconnecting...");
    // Attempt to reconnect to the MQTT broker
    reconnect();
  }
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}


// Add code here to send an MQTT message to notify ESP32-CAM before deep sleep
void notifyESP32CAMBeforeSleep() {
  if (client.connected()) {
    Serial.println("Sending sleep notification to ESP32-CAM");
    if (client.publish(mqttStatusTopic, "Going to sleep")) {
      Serial.println("Published sleep notification successfully");
      // Blink the LED to indicate successful sleep notification
      blinkLED(ledPin, 2);  // Blink twice
    } else {
      Serial.println("Failed to publish sleep notification");
    }
  } else {
    Serial.println("Not connected to MQTT broker. Reconnecting...");
    // Attempt to reconnect to the MQTT broker
    reconnect();
  }
}
