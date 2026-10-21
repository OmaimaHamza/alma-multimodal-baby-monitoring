/************ LIBRARIES ************/
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

/************ WIFI CONFIG ************/
#define WIFI_SSID "SSID" //change this to the actual wifi-ssid
#define WIFI_PASSWORD "pass" //same thing here write the password

/************ FIREBASE CONFIG ************/
#define API_KEY "api-key" //write here the api key of your database
#define DATABASE_URL "https://your-database-url.firebaseio.com" //we use a firebase database

/************ FIREBASE CONFIG ************/
#define USER_EMAIL "example@gmail.com" //change this
#define USER_PASS "pass" //change this

/************ FIREBASE OBJECTS ************/
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/************ SENSOR CONFIG ************/
#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ135_PIN 34
#define PIR_PIN 16
DHT dht(DHTPIN, DHTTYPE);
static const int servoPin = 13;
Servo servo1;
//for oled display
//SCL -> Pin 22
//SDA -> Pin 21

/************ TIMING ************/
unsigned long lastSend = 0;
const unsigned long interval = 30000; // 30 seconds

/************ OLED ************/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/************ motion logic ************/
unsigned long lastMotionTime = 0;
unsigned long lastPirSend = 0;
const unsigned long pirCooldown = 500; // 10 seconds
bool waitingForTimeout = false;
bool motionWasHigh = false;

/************ FUNCTION DECLARATIONS ************/
void connectToWiFi();
void initFirebase();
float readTemperature();
float readHumidity();
int readGas();
void sendDataToFirebase(float temp, float hum, int gas);
void showBigText(const char* text,const int size);
void ServoLogic();
void handleMotion();
void showALMAAnimation();

/************ SETUP ************/
void setup() {
  Serial.begin(115200);
  // Initialize OLED 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed!");
    for(;;); // Don't proceed, loop forever
  }
  Serial.println("OLED initialized!");
  // Display website name
  showBigText("ALMA",2);
  delay(200);
  pinMode(MQ135_PIN, INPUT);
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  servo1.attach(servoPin);
  servo1.write(90);
  connectToWiFi();
  initFirebase();
}

/************ LOOP ************/
void loop() {
  ServoLogic();
  handleMotion();
  if (Firebase.ready() && millis() - lastSend >= interval) {
    //sensors logic
    lastSend = millis();
    float temperature = readTemperature();
    float humidity = readHumidity();
    int gasValue = readGas();

    if (!isnan(temperature) && !isnan(humidity)) {
      sendDataToFirebase(temperature, humidity, gasValue);
    } else {
      Serial.println("Failed to read from sensor");
    }
// OLED display logic

int babyCovered = -1;
int babyPresence = -1;
int babyCrying = -1;

// Read Firebase values
Firebase.RTDB.getInt(&fbdo, "/realtime_video/baby_covered");
babyCovered = fbdo.intData();

Firebase.RTDB.getInt(&fbdo, "/realtime_video/baby_presence");
babyPresence = fbdo.intData();

Firebase.RTDB.getInt(&fbdo, "/baby_room/cry_result/crying");
babyCrying = fbdo.intData();

Serial.print("Covered: ");
Serial.println(babyCovered);
Serial.print("Presence: ");
Serial.println(babyPresence);
Serial.print("Crying: ");
Serial.println(babyCrying);


// Alert priority system
if (babyCovered == 1) {
    showBigText("ALERT: COVERED", 1.9);
}

else if (babyPresence == 0) {
    showBigText("NO BABY", 2);
}

else if (babyCrying == 1) {
    showBigText("BABY CRYING", 1.9);
}

else {
    showALMAAnimation();
}
}
}


/************ FUNCTIONS ************/

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Connecting WiFi...");
  display.display();
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\n WiFi connected");
  Serial.println(WiFi.localIP());
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("WiFi Connected!");
  display.setCursor(0, 35);
  display.println("IP:");
  display.setCursor(20, 35);
  display.println(WiFi.localIP());
  display.display();
  delay(500);
}

void initFirebase() {
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("Connecting to");
  display.setCursor(0, 35);
  display.println("Firebase...");
  display.display();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASS;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase initialized");
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("Firebase");
  display.setCursor(0, 35);
  display.println("Connected!");
  display.display();
  delay(500);
}


float readTemperature() {
  return dht.readTemperature();
}

float readHumidity() {
  return dht.readHumidity();
}

int readGas() {
  return analogRead(MQ135_PIN);
}

void sendDataToFirebase(float temp, float hum, int gas) {
  FirebaseJson json;

  json.set("temperature", temp);
  json.set("humidity", hum);
  json.set("gas", gas);

  if (Firebase.RTDB.setJSON(&fbdo, "/baby_room/sensors", &json)) {
    Serial.println("Data sent to Firebase");
  } else {
    Serial.print("Firebase error: ");
    Serial.println(fbdo.errorReason());
  }
}

void handleMotion() {
  int motion = digitalRead(PIR_PIN);
  unsigned long currentTime = millis();

  if (motion == HIGH) {
    // Motion detected
    if (currentTime - lastPirSend > pirCooldown) {
      lastPirSend = currentTime;
      Serial.println("Motion detected (PIR)");
      
      Firebase.RTDB.setInt(&fbdo, "/baby_room/movement_detection/active", 1);
      Firebase.RTDB.setInt(&fbdo, "/baby_room/movement_detection/timestamp", currentTime);
      Firebase.RTDB.setInt(&fbdo, "/baby_room/camera/capture_request", 0);

      // Reset emergency if baby moves again
      Firebase.RTDB.setInt(&fbdo, "/baby_room/camera/emergency_capture", 0);
    }

    lastMotionTime = currentTime;
    waitingForTimeout = false;
    motionWasHigh = true;
}
  else if (motion == LOW) {
    // No motion detected
    if (motionWasHigh) {
      // Motion just changed from HIGH to LOW
      motionWasHigh = false;
      lastMotionTime = currentTime; // Start counting from when motion stopped
      waitingForTimeout = true;
      Serial.println("Motion stopped, starting 10s timer...");
    }
    
    // Check if we've been waiting and 10 seconds have passed
    if (waitingForTimeout && (currentTime - lastMotionTime >= pirCooldown)) {
      waitingForTimeout = false;
      Firebase.RTDB.setInt(&fbdo, "/baby_room/movement_detection/active", 0);
      Firebase.RTDB.setInt(&fbdo, "/baby_room/camera/capture_request", 1);
      if(Firebase.RTDB.getInt(&fbdo, "/baby_room/movement_detection/awake")==1){
        Firebase.RTDB.setInt(&fbdo, "/baby_room/camera/emergency_capture", 1);
      }
      else{
        Firebase.RTDB.setInt(&fbdo, "/baby_room/camera/emergency_capture", 0);
      }
      Serial.println("No motion for 10s - Capturing image");
    }
  }
}
// Display big text function
void showBigText(const char* text,const int size) {
  display.clearDisplay();
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  
  // Get text dimensions
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  
  // Center the text
  int x = (128 - w) / 2;
  int y = (64 - h) / 2;
  
  display.setCursor(x, y);
  display.println(text);
  display.display();
}
void ServoLogic(){
    // Lire l'angle depuis Firebase
  if (Firebase.RTDB.getInt(&fbdo, "baby_room/servo/angle")) {
    int angle = fbdo.intData();
    
    // Limiter l'angle entre 0 et 180
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    // Bouger le servo à cette position
    servo1.write(angle);
    Serial.print("Servo position: ");
    Serial.println(angle);
  } else {
    Serial.println("Failed to read from Firebase");
  }
}
void showALMAAnimation() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(10, 20);
  display.print("A");
  display.display();
  delay(200);

  display.print("L");
  display.display();
  delay(200);

  display.print("M");
  display.display();
  delay(200);

  display.print("A");
  display.display();
  delay(600);
}