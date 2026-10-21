#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Firebase_ESP_Client.h>

// =================== WiFi Configuration ===================
const char* ssid = "ssid"; //change this
const char* password = "pass"; //change this

// =================== Firebase Configuration ===================
#define API_KEY "api-key" //change this
#define DATABASE_URL "url.firebaseio.com" //change this
#define USER_EMAIL "example@gmail.com" //change this
#define USER_PASSWORD "pass" //change this

// =================== Python Server Configuration ===================
const char* python_server_ip = "10.43.27.49"; //you may only need to change this
const int python_server_port = 8000; //you may only need to change this

// =================== Firebase Objects ===================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// =================== Pin configuration (AI-Thinker ESP32-CAM) ===================
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

// =================== Flash LED ===================
#define LED_FLASH_PIN     4

// =================== Web Server ===================
WebServer server(81);

// =================== Global Variables ===================
unsigned long lastFirebaseCheck = 0;
const unsigned long firebaseCheckInterval = 500; 
unsigned long lastCaptureTime = 0;
const unsigned long captureCooldown = 2000; 

// =================== Camera Initialization ===================
bool initCamera() {
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
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera initialization error: 0x%x\n", err);
        return false;
    }
    
    Serial.println("Camera initialized successfully!");
    return true;
}

// =================== Firebase Initialization ===================
bool initFirebase() {
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    Serial.println("Connecting to Firebase...");
    int attempts = 0;
    while (!Firebase.ready() && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (Firebase.ready()) {
        Serial.println("Firebase connected!");
        return true;
    } else {
        Serial.println("Firebase connection error");
        return false;
    }
}

// =================== Send information to Firebase ===================
void sendCameraInfoToFirebase() {
    if (!Firebase.ready()) return;
    
    String ipAddress = WiFi.localIP().toString();
    
    if (Firebase.RTDB.setString(&fbdo, "baby_room/network/ip", ipAddress)) {
        Serial.println("IP sent to Firebase: " + ipAddress);
    }
    
    if (Firebase.RTDB.setInt(&fbdo, "baby_room/network/port", 81)) {
        Serial.println("Port sent to Firebase: 81");
    }
    
    String streamUrl = "http://" + ipAddress + ":81/stream";
    if (Firebase.RTDB.setString(&fbdo, "baby_room/network/stream_url", streamUrl)) {
        Serial.println("Stream URL sent: " + streamUrl);
    }
    
    Firebase.RTDB.setInt(&fbdo, "baby_room/camera/status", 1);
    Firebase.RTDB.setString(&fbdo, "baby_room/camera/last_seen", String(millis()));
    
    // Initialize capture_request to 0
    Firebase.RTDB.setInt(&fbdo, "baby_room/camera/capture_request", 0);
}

// =================== Check and process Firebase requests ===================
void checkFirebaseRequests() {
    if (!Firebase.ready()) return;
    
    // Read capture_request
    if (Firebase.RTDB.getInt(&fbdo, "baby_room/camera/capture_request")) {
        int captureRequest = fbdo.intData();
        
        // If capture_request == 1, trigger a capture
        if (captureRequest == 1) {
            // Check cooldown to avoid too frequent captures
            if (millis() - lastCaptureTime >= captureCooldown) {
                Serial.println("Capture requested by Firebase!");
                captureAndSend();
                lastCaptureTime = millis();
                
                // Set capture_request back to 0
                Firebase.RTDB.setInt(&fbdo, "baby_room/camera/capture_request", 0);
                Serial.println("capture_request reset to 0");
            } else {
                Serial.println("Capture ignored (cooldown)");
            }
        }
    } else {
        Serial.println("Firebase read error: " + fbdo.errorReason());
    }
    
    // Update last_seen periodically
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 30000) { // Every 30 seconds
        Firebase.RTDB.setString(&fbdo, "baby_room/camera/last_seen", String(millis()));
        lastUpdate = millis();
    }
}

// =================== Send image to Python server ===================
bool sendImageToPythonServer(camera_fb_t *fb) {
    if (fb == NULL) {
        Serial.println("Empty image buffer");
        return false;
    }
    
    HTTPClient http;
    String url = "http://" + String(python_server_ip) + ":" + String(python_server_port) + "/upload";
    
    http.begin(url);
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("X-Camera-ID", "ESP32-CAM-01");
    
    int httpResponseCode = http.POST(fb->buf, fb->len);
    
    bool success = false;
    if (httpResponseCode > 0) {
        Serial.printf("Image sent to Python server. Code: %d\n", httpResponseCode);
        String response = http.getString();
        Serial.println("Response: " + response);
        success = true;
    } else {
        Serial.printf("Error sending to Python server. Code: %d\n", httpResponseCode);
        Serial.printf("Error: %s\n", http.errorToString(httpResponseCode).c_str());
        success = false;
    }
    
    http.end();
    return success;
}

// =================== Capture and send image ===================
void captureAndSend() {
    Serial.println("Capturing...");
    
    // Turn on the flash
    digitalWrite(LED_FLASH_PIN, HIGH);
    delay(50);
    
    // Capture
    camera_fb_t *fb = esp_camera_fb_get();
    
    // Turn off the flash
    digitalWrite(LED_FLASH_PIN, LOW);
    
    if (!fb) {
        Serial.println("Capture failed");
        return;
    }
    
    Serial.printf("Image captured: %zu bytes\n", fb->len);
    
    // Send to Python server
    bool sent = sendImageToPythonServer(fb);
    
    // Release the buffer
    esp_camera_fb_return(fb);
    
    if (sent) {
        Serial.println("Capture and send successful!");
    } else {
        Serial.println("Capture successful but send failed");
    }
}

// =================== STREAM handler ONLY ===================
void handleStream() {
    WiFiClient client = server.client();
    
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n";
    response += "Cache-Control: no-cache, no-store, must-revalidate\r\n";
    response += "Pragma: no-cache\r\n";
    response += "Expires: 0\r\n";
    response += "Connection: keep-alive\r\n";
    response += "Access-Control-Allow-Origin: *\r\n\r\n";
    
    client.print(response);
    
    while (client.connected()) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            delay(50);
            continue;
        }
        
        client.print("--frame\r\n");
        client.print("Content-Type: image/jpeg\r\n");
        client.print("Content-Length: " + String(fb->len) + "\r\n\r\n");
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        
        esp_camera_fb_return(fb);
        delay(50);
    }
}

// =================== 404 handler ===================
void handleNotFound() {
    server.send(404, "text/plain", "404 - Use /stream for the video feed");
}

// =================== SETUP ===================
void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println("\n\n=== ESP32-CAM Simple Stream Mode ===");
    
    // Flash LED configuration
    pinMode(LED_FLASH_PIN, OUTPUT);
    digitalWrite(LED_FLASH_PIN, LOW);
    
    // WiFi connection
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Camera initialization
    if (!initCamera()) {
        Serial.println("Fatal error: unable to initialize the camera");
        while(1) delay(1000);
    }
    
    // Firebase initialization
    if (!initFirebase()) {
        Serial.println("Firebase not connected - Degraded mode");
    }
    
    // Send info to Firebase
    sendCameraInfoToFirebase();
    
    // Route configuration - ONLY /stream
    server.on("/stream", handleStream);
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("Stream server started on port 81");
    Serial.print("Stream URL: http://");
    Serial.print(WiFi.localIP());
    Serial.println(":81/stream");
    
    Serial.println("\n=== Waiting for Firebase requests ===");
}

// =================== LOOP ===================
void loop() {
    server.handleClient();
    
    // Check Firebase periodically
    if (millis() - lastFirebaseCheck >= firebaseCheckInterval) {
        checkFirebaseRequests();
        lastFirebaseCheck = millis();
    }
    
    delay(10);
}