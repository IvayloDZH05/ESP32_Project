#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Servo.h>

const char* ssid = "NET1";
const char* password = "803-_-308";

const int servoPin = 13; // Change to the pin where your servo is connected
Servo myServo;

WebSocketsServer webSocket = WebSocketsServer(81);

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_BIN: {
      // Handle binary data (commands)
      if (length > 0) {
        char command = payload[0];
        handleServoCommand(command);
      }
      break;
    }
  }
}

void handleServoCommand(char command) {
  switch (command) {
    case 'L':
      // Move the servo to the left
      myServo.write(90); // Adjust the angle as needed
      break;
    case 'R':
      // Move the servo to the right
      myServo.write(180); // Adjust the angle as needed
      break;
    default:
      // Stop the servo or handle other commands as needed
      myServo.write(0);
  }
}

void setup() {
  Serial.begin(115200);
  myServo.attach(servoPin);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set up WebSocket server
  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);

  Serial.println("WebSocket server started");
}

void loop() {
  // Handle WebSocket events
  webSocket.loop();

  // Other loop code if needed
}
