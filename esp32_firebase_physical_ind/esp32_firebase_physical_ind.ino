#include <WiFi.h>
#include <FirebaseESP32.h>
#include <max6675.h>
#include "time.h"  // NTP time library

// ✅ Wi-Fi Credentials
#define WIFI_SSID "Rk's iQOO"
#define WIFI_PASSWORD "Ramakanth"

// ✅ Firebase Credentials
#define FIREBASE_HOST "https://temperature-web-monitoring-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyAc6cGPZf0f2-67TCHB-KCKmX4Rjk2FvZ4"

// ✅ Firebase Objects
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;
FirebaseJson json;

// ✅ MAX6675 Thermocouple Sensor Pins
#define SCK 18
#define CS 5
#define SO 19
MAX6675 thermocouple(SCK, CS, SO);

// ✅ LED & Buzzer Pins
#define LED_PIN 4
#define BUZZER_PIN 2

float thresholdTemp = 45.0;  // Default threshold for buzzer
float indicatorTemp = 40.0;  // Default indicator for LED

// ✅ NTP Config
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;  // UTC +5:30
const int daylightOffset_sec = 0;

void setup() {
  Serial.begin(921600);

  // ✅ Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");

  // ✅ Firebase Setup
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.api_key = FIREBASE_AUTH;
  firebaseConfig.database_url = FIREBASE_HOST;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);

  // ✅ Time Setup
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("✅ Time Sync Started");

  // ✅ GPIO Setup
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

// ✅ Get current formatted time
String getFormattedDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("❌ Failed to get time");
    return "00:00:00 | 01-01-1970";
  }

  char timeStr[10];
  char dateStr[12];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  strftime(dateStr, sizeof(dateStr), "%d-%m-%Y", &timeinfo);
  return String(timeStr) + " | " + String(dateStr);
}

// ✅ Fetch threshold & indicator from Firebase
void fetchFirebaseLimits() {
  if (Firebase.getFloat(firebaseData, "/threshold")) {
    thresholdTemp = firebaseData.floatData();
    Serial.print("✅ Threshold: ");
    Serial.println(thresholdTemp);
  } else {
    Serial.println("❌ Couldn't read /threshold");
  }

  if (Firebase.getFloat(firebaseData, "/indicator")) {
    indicatorTemp = firebaseData.floatData();
    Serial.print("✅ Indicator: ");
    Serial.println(indicatorTemp);
  } else {
    Serial.println("❌ Couldn't read /indicator");
  }
}

void loop() {
  Serial.println("📡 Reading Temperature...");
  float temp = thermocouple.readCelsius();

  if (isnan(temp)) {
    Serial.println("❌ Failed to read from MAX6675");
  } else {
    String datetime = getFormattedDateTime();

    Serial.printf("🌡 Temperature: %.2f °C\n", temp);
    Serial.println("🕒 Timestamp: " + datetime);

    // ✅ Get updated threshold & indicator
    fetchFirebaseLimits();

    // ✅ Handle LED (indicator)
    if (temp >= indicatorTemp) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("🔴 LED ON (High Temp Warning)");
    } else {
      digitalWrite(LED_PIN, LOW);
      Serial.println("🟢 LED OFF");
    }

    // ✅ Handle Buzzer (threshold)
    if (temp >= thresholdTemp) {
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.println("🚨 Buzzer ON (Threshold Breached)");
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("🔕 Buzzer OFF");
    }

    // ✅ Push data to Firebase
    json.clear();
    json.set("temperature", temp);
    json.set("recorded_time", datetime);

    if (Firebase.setJSON(firebaseData, "/temperature_data", json)) {
      Serial.println("✅ Data Uploaded to Firebase");
    } else {
      Serial.print("❌ Firebase Upload Error: ");
      Serial.println(firebaseData.errorReason());
    }
  }

  delay(5000);  // every 5 seconds
}
