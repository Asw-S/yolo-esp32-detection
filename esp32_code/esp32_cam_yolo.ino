#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Camera pins for ESP32-CAM AI Thinker
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// WiFi credentials
const char* ssid = "Asw_S";
const char* password = "There is no password";

// API endpoint (update with your Vercel deployment URL)
const char* serverUrl = "https://yolo-vercel-iqvu0oyeu-aswin-ss-projects-9cb5e51d.vercel.app/detect";

// LED pin for status indication
#define LED_PIN 4

// Variables for retry mechanism
int retryCount = 0;
const int maxRetries = 5;
const int retryDelay = 2000; // ms

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("ESP32-CAM YOLO Object Detection");
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize camera
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
  
  // Initialize with high resolution for better detection
  config.frame_size = FRAMESIZE_VGA; // 640x480
  config.jpeg_quality = 10; // Lower is higher quality (0-63)
  config.fb_count = 2;
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    blinkLED(5, 500); // Error indicator
    return;
  }
  Serial.println("Camera initialized successfully");
  
  // Connect to WiFi
  connectToWiFi();
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    blinkLED(2, 200); // Success indicator
  } else {
    Serial.println("\nFailed to connect to WiFi");
    blinkLED(3, 300); // Connection failure indicator
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    connectToWiFi();
    delay(1000);
    return;
  }
  
  // Capture image
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    blinkLED(3, 100);
    delay(1000);
    return;
  }
  
  Serial.printf("Image captured: %dx%d, %d bytes\n", fb->width, fb->height, fb->len);
  digitalWrite(LED_PIN, HIGH); // Turn on LED while processing
  
  // Send image to server for detection
  bool success = sendImageForDetection(fb);
  
  // Return the frame buffer to be reused
  esp_camera_fb_return(fb);
  
  digitalWrite(LED_PIN, LOW); // Turn off LED after processing
  
  // Wait before next capture
  delay(5000);
}

bool sendImageForDetection(camera_fb_t *fb) {
  HTTPClient http;
  bool success = false;
  retryCount = 0;
  
  while (!success && retryCount < maxRetries) {
    Serial.println("Sending image to server for detection...");
    http.begin(serverUrl);
    http.addHeader("Content-Type", "image/jpeg");
    
    // Send HTTP POST request with image data
    int httpResponseCode = http.POST(fb->buf, fb->len);
    
    if (httpResponseCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpResponseCode);
      String response = http.getString();
      
      // Process response
      if (httpResponseCode == 200) {
        success = true;
        processDetectionResponse(response);
      } else {
        Serial.println("Server error. Response:");
        Serial.println(response);
      }
    } else {
      Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
    
    if (!success) {
      retryCount++;
      if (retryCount < maxRetries) {
        Serial.printf("Retrying... (%d/%d)\n", retryCount, maxRetries);
        delay(retryDelay);
      }
    }
  }
  
  if (!success) {
    Serial.println("Failed to get valid detection results after maximum retries");
    blinkLED(4, 200); // Error indicator
  }
  
  return success;
}

void processDetectionResponse(String response) {
  // Create a JSON document to parse the response
  // Increase capacity based on your expected response size
  DynamicJsonDocument doc(8192);
  
  // Parse JSON response
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Check if detection was successful
  bool success = doc["success"];
  if (!success) {
    Serial.println("Detection failed on server side");
    return;
  }
  
  // Print processing time
  float processingTime = doc["processing_time"];
  Serial.print("Processing time: ");
  Serial.print(processingTime);
  Serial.println(" seconds");
  
  // Print detection results
  JsonArray detections = doc["detections"];
  Serial.print("Objects detected: ");
  Serial.println(detections.size());
  
  for (JsonObject detection : detections) {
    const char* className = detection["class"];
    float confidence = detection["confidence"];
    JsonObject box = detection["box"];
    
    int x1 = box["x1"];
    int y1 = box["y1"];
    int x2 = box["x2"];
    int y2 = box["y2"];
    
    Serial.print("- Class: ");
    Serial.print(className);
    Serial.print(", Confidence: ");
    Serial.print(confidence);
    Serial.print(", Box: [");
    Serial.print(x1);
    Serial.print(",");
    Serial.print(y1);
    Serial.print(",");
    Serial.print(x2);
    Serial.print(",");
    Serial.print(y2);
    Serial.println("]");
    
    // Here you can add code to react to specific detections
    // For example, if a person is detected with high confidence
    if (strcmp(className, "person") == 0 && confidence > 0.7) {
      // Take some action
      blinkLED(1, 1000); // Long blink for person detection
    }
  }
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}