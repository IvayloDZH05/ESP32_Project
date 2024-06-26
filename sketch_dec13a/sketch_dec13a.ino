  #include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <LittleFS.h>
#include "WiFi.h"
#include <StringArray.h>
#include <AsyncTCP.h>   
#include <ESPAsyncWebServer.h>
#include <ESP_Mail_Client.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>

// Replace with your network credentials
const char* ssid = "Nothing phone (1)";
const char* password = "ivoRD2005";
const char* mqttServer = "192.168.182.94";
const int mqttPort = 1883;
const char* mqttTopic = "servo_control";


#define emailSenderAccount    "esp326969@gmail.com"
#define emailSenderPassword   "wfczkmzpktlsxuqj"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "Заснета снимка от ESP32-CAM"

// Default Recipient Email Address
String inputMessage = "19212@uktc-bg.com";

WiFiClient espClient;
PubSubClient client(espClient);
WebSocketsServer webSocket = WebSocketsServer(81);
AsyncWebServer server(80);


uint8_t cam_num;
bool connected = false;
bool flashlightOn = false;
bool photoCaptureSuccess = false;
bool pirMotionDetected = false;  // Global variable to indicate motion detection

unsigned long lastMotionTime = 0;
const int motionCheckInterval = 10000;
unsigned long lastPIRCheckTime = 0;

bool userPresent = true;
unsigned long lastUserPresentTime = 0; // Variable to store the timestamp of the last "UserPresent" message
unsigned long lastUserNotPresentTime = 0; // Variable to store the timestamp of the last "UserNotPresent" message

bool emailSent = true;
boolean takeNewPhoto = false;

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define FLASHLIGHT_LED_PIN 4
#define PIR_PIN 14

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

// Photo File Name to save in LittleFS
#define FILE_PHOTO "photo.jpg"
#define FILE_PHOTO_PATH "/photo.jpg"

void handleWebSocketText(uint8_t num, uint8_t *payload, size_t length);

void configCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST;
    pinMode(FLASHLIGHT_LED_PIN, OUTPUT);

    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

       // Set the vertical flip to true
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, true);
    s->set_hmirror(s, true);
}

void liveCam(uint8_t num) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Frame buffer could not be acquired");
        return;
    }

    digitalWrite(FLASHLIGHT_LED_PIN, flashlightOn ? HIGH : LOW);

    webSocket.sendBIN(num, fb->buf, fb->len);
    esp_camera_fb_return(fb);
}




void handleWebSocketText(uint8_t num, uint8_t *payload, size_t length) {
    String command = String((char *)payload);
    command.trim();

    if (command == "toggleFlashlight") {
        flashlightOn = !flashlightOn;
    }

    if (command == "userPresent") {
        Serial.println("User present");
        // Check if there's a sufficient time gap since the last "UserPresent" message
        if (millis() - lastUserPresentTime >= 5000) { // Adjust the delay as needed
            // Set the user presence flag to true
            userPresent = true;
            // Update the timestamp for the current "UserPresent" message
            lastUserPresentTime = millis();
        }
    } else if (command == "userNotPresent") {
        Serial.println("User not present");
        // Check if there's a sufficient time gap since the last "UserNotPresent" message
        if (millis() - lastUserNotPresentTime >= 5000) { // Adjust the delay as needed
            // Set the user presence flag to false
            userPresent = false;
            // Update the timestamp for the current "UserNotPresent" message
            lastUserNotPresentTime = millis();
        }
    }
}



void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            cam_num = num;
            connected = true;
            break;
        case WStype_TEXT:
            handleWebSocketText(num, payload, length);
            break;
        case WStype_BIN:
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

// Replaces placeholder with DS18B20 values
String processor(const String& var){
  //Serial.println(var);
  if(var == "EMAIL_INPUT"){
    return inputMessage;
  }
  else if(var == "PER"){
    return "%";
  }
  return String();
}

const char* PARAM_INPUT_1 = "email_input";


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected!"); // Print a message on successful connection
      // Once connected, subscribe to the topic
      client.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  Serial.println("PIR Sensor Initialized");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  String IP = WiFi.localIP().toString();
  Serial.println("\nWiFi connected, IP address: " + IP);



if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    Serial.println("Make sure the file system was formatted successfully");
    return;
} else {
    Serial.println("LittleFS mounted successfully");

    // Print the list of files in LittleFS (for debugging purposes)
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.print("File: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
}

  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    // Set up MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

    // Attempt to connect to the MQTT broker
  if (client.connect("ESP32Client")) {
    Serial.println("Connected to MQTT broker");

    // Subscribe to the topic
    if (client.subscribe(mqttTopic)) {
      Serial.println("Subscribed to topic: " + String(mqttTopic));
    } else {
      Serial.println("Failed to subscribe to topic");
    }
  } else {
    Serial.println("Failed to connect to MQTT broker");
  }
  
  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    handleWebSocketEvent(num, type, payload, length);
  });

server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->send(LittleFS, "/CameraFeed.html", "text/html");
});

  server.on("/CameraFeed.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/CameraFeed.css", "text/css");
  });

    server.on("/CameraFeed.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/CameraFeed.js", "text/js");
  });


server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
    takeNewPhoto = true;

    // Set the flag to true to indicate a successful photo capture
    photoCaptureSuccess = true;

    // You can customize the success message based on your requirements
    String successMessage = "Photo captured and saved successfully.";

    // Send a response message to the client
    request->send(200, "text/plain", successMessage);
});

server.on("/action", HTTP_GET, handleFlashAction);

server.on("/email-photo", HTTP_GET, [](AsyncWebServerRequest *request) {
  emailSent = false;
  request->send(200, "text/plain", "Sending Photo");
});

server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Open the photo file for reading
    File file = LittleFS.open(FILE_PHOTO_PATH, "r");

    if (file && file.size() > 0) {
        // Set the content type based on the image format (e.g., JPEG)
        request->send(LittleFS, FILE_PHOTO_PATH, "image/jpeg");
    } else {
        // If the file doesn't exist or is empty, send a 404 Not Found response
        request->send(404, "text/plain", "Photo not found");
    }
    
});

server.on("/set-servo-angle", HTTP_GET, [](AsyncWebServerRequest *request){
    handleSetServoAngle(request);
});

  // Start server
  server.begin();
  configCamera();
}

void loop() {
  // Handle WebSocket events
  webSocket.loop();

  // Handle MQTT messages
  client.loop();

  if (connected) {
    liveCam(cam_num);
  }

  if (millis() - lastPIRCheckTime >= 15000) {
    checkPIRMotion();
    lastPIRCheckTime = millis();
  }

    if (takeNewPhoto) {
    capturePhotoSaveLittleFS  ();
    takeNewPhoto = false;
  }
  if (!emailSent){
    String emailMessage = "Вашата заснета снимка:";
    if(sendEmailNotification(emailMessage)) {
      Serial.println(emailMessage);
      emailSent = true;
    }
    else {
      Serial.println("Email failed to send");
    }    
  }
  delay(1);
}


// Define PIR_PIN only if it's not already defined
#ifndef PIR_PIN
#define PIR_PIN -1 // Use an invalid pin number if PIR_PIN is not defined
#endif

void checkPIRMotion() {
    // Check if PIR_PIN is defined
    #ifdef PIR_PIN
    int pirState = digitalRead(PIR_PIN);

    Serial.print("PIR State: ");
    Serial.println(pirState);

    if (!userPresent && pirState == HIGH) {
        Serial.println("Motion detected!");
        capturePhotoSaveLittleFS();
        sendEmailNotification("Движение беше засечено!!");
        delay(5000);
    }

    delay(100);
    #endif
}


bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open(FILE_PHOTO_PATH);
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

void capturePhotoSaveLittleFS(void) {
    // Turn on the flashlight
    digitalWrite(FLASHLIGHT_LED_PIN, HIGH);

    camera_fb_t *fb = NULL; // pointer
    bool ok = false;

    do {
        // Take a photo with the camera
        Serial.println("Taking a photo...");

        // Clean previous buffer
        fb = esp_camera_fb_get();
        esp_camera_fb_return(fb); // dispose the buffered image
        fb = NULL; // reset to capture errors
        // Get fresh image
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            delay(1000);
            ESP.restart();
        }
        // Photo file name
        Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
        File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);

        // Insert the data in the photo file
        if (!file) {
            Serial.println("Failed to open file in writing mode");
        }
        else {
            file.write(fb->buf, fb->len); // payload (image), payload length
            Serial.print("The picture has been saved in ");
            Serial.print(FILE_PHOTO_PATH);
            Serial.print(" - Size: ");
            Serial.print(fb->len);
            Serial.println(" bytes");
        }
        // Close the file
        file.close();
        esp_camera_fb_return(fb);

        // check if file has been correctly saved in LittleFS
        ok = checkPhoto(LittleFS);
    } while (!ok);

    // Turn off the flashlight after capturing the photo
    digitalWrite(FLASHLIGHT_LED_PIN, LOW);
}

bool sendEmailNotification(String emailMessage){
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = smtpServer;
  session.server.port = smtpServerPort;
  session.login.email = emailSenderAccount;
  session.login.password = emailSenderPassword;
  session.login.user_domain = "";

  /* Declare the message class */
  SMTP_Message message;

  /* Enable the chunked data transfer with pipelining for large message if server supported */
  message.enable.chunking = true;

  /* Set the message headers */
  message.sender.name = "Твоята алармена система";
  message.sender.email = emailSenderAccount;

  message.subject = emailSubject;
  message.addRecipient("Ивайло Джамбазов", inputMessage.c_str());

  String htmlMsg = emailMessage;
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  SMTP_Attachment att;

  att.descr.filename = FILE_PHOTO;
  att.descr.mime = "image/png"; 
  att.file.path = FILE_PHOTO_PATH;
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add attachment to the message */
  message.addAttachment(att);

  /* Connect to server with the session config */
  smtp.connect(&session);

  /* Start sending the Email and close the session */
  if (!MailClient.sendMail(&smtp, &message, true))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status){
  Serial.println(status.info());

  if (status.success())
  {
    Serial.println("----------------");
    Serial.printf("Message sent success: %d\n", status.completedCount());
    Serial.printf("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;

      Serial.printf("Message No: %d\n", i + 1);
      Serial.printf("Status: %s\n", result.completed ? "success" : "failed");
      Serial.printf("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      Serial.printf("Recipient: %s\n", result.recipients);
      Serial.printf("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
    
    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();

  }
}

void handleFlashAction(AsyncWebServerRequest *request) {
    String actionType;
    String actionValue;

    if (request->hasParam("actionType") && request->hasParam("actionValue")) {
        actionType = request->getParam("actionType")->value();
        actionValue = request->getParam("actionValue")->value();
    }

    if (actionType == "flashlight") {
        if (actionValue == "on") {
            flashlightOn = true;
        } else if (actionValue == "off") {
            flashlightOn = false;
        }
    }

    request->send(200, "text/plain", "OK");
}

void handleSetServoAngle(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_GET) {
        // Get the servo ID and angle from the request parameters
        String servoIdStr = request->arg("servoId");
        String angleStr = request->arg("angle");

        // Concatenate servo ID and angle into a single string
        String message = servoIdStr + ":" + angleStr;

        // Publish servo ID and angle via MQTT using the predefined topic
        String topic = mqttTopic;
        client.publish(topic.c_str(), message.c_str());

        // Print the sent message to the serial monitor
        Serial.print("Sent message over MQTT: ");
        Serial.println(message);

        // Send response to the client
        request->send(200, "text/plain", "Servo angle updated");
    } else {
        // If the request method is not GET, send a 405 Method Not Allowed response
        request->send(405, "text/plain", "Method Not Allowed");
    }
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

}
