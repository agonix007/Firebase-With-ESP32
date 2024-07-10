#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#include <DHT.h>

// DHT11 sensor pin & type
#define DHTPIN 25 
#define DHTTYPE DHT11

// Insert all required WIFI Credentials 
#define WIFI_SSID "Redmi Note 10S"
#define WIFI_PASSWORD "souhardya@29"

// Change according to USER
#define USER_EMAIL "ultimatewar023@gmail.com"
#define USER_PASSWORD "captain"

// Must change according to Project API
#define API_KEY "AIzaSyA1-5f1i_qTu05oNTyuU_XDTmSTPGfVVSw"

// Must change according to DB URL
#define DATABASE_URL "https://classcraft-14b22-default-rtdb.asia-southeast1.firebasedatabase.app/" 

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Variables to save database paths
String databasePath;
String tempPath;
String humPath;
String presPath;

// DHT11 sensor configuration
DHT dht(DHTPIN, DHTTYPE);

// Timer variables (send new readings every fifteen seconds)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 15000;

// Setting up WIFI
void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
}

// Setting up FIREBASE
void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;

  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);

  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  
  uid = auth.token.uid.c_str();
  Serial.println();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/users/";
  databasePath += uid;

  // Update database path for sensor readings
  humPath = databasePath;
  humPath += "/data/humidity"; // --> users/<user_uid>/data/humidity

  tempPath = databasePath;
  tempPath += "/data/temperature"; // --> users/<user_uid>/data/temperature

  fbdo.setResponseSize(2048);
  Firebase.RTDB.setReadTimeout(&fbdo, 1000 * 30);
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "tiny");
}

void pushData(String path, float value) {
  if (Firebase.RTDB.setInt(&fbdo, path.c_str(), value)) {
    Serial.println("PASSED");
    Serial.print("PATH: ");
    Serial.println(fbdo.dataPath());
    Serial.print("TYPE: ");
    Serial.println(fbdo.dataType());
  } else {
    Serial.println("FAILED");
    Serial.print("REASON: ");
    Serial.println(fbdo.errorReason());
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  setupWiFi();
  setupFirebase();

  pinMode(2, OUTPUT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    setupWiFi();
  }
  else{
    digitalWrite(2, HIGH);
  }

  if (Firebase.isTokenExpired()) {
    Firebase.refreshToken(&config);
    Serial.println("Firebase token refreshed");
  }

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    pushData(tempPath, temperature);
    pushData(humPath, humidity);
  }
}