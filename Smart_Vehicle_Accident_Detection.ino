// Aim: Smart Vehicle Accident Detection, Alert sent on Telegram & Obstacle Alert System on LCD with RGB light

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <math.h>

//==============================
// WiFi Credentials
//==============================

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

//==============================
// Telegram
//==============================

const char* BOT_TOKEN = "YOUR_BOT_TOKEN";
const char* CHAT_ID = "6895161222";

//==============================
// LCD
//==============================

LiquidCrystal_I2C lcd(0x27, 16, 2);

//==============================
// MPU6050
//==============================

Adafruit_MPU6050 mpu;

//==============================
// HC-SR04
//==============================

#define TRIG_PIN 5
#define ECHO_PIN 18

//==============================
// RGB LED
//==============================

#define RED_LED 27
#define GREEN_LED 25
#define BLUE_LED 26

//==============================
// Variables
//==============================

bool accidentDetected = false;
bool telegramSent = false;

float distanceCM = 0;

//Calibration Offsets
float offsetX = 0;
float offsetY = 0;
float offsetZ = 0;

//Acceleration
float ax, ay, az;

//Gyroscope
float gx, gy, gz;

//Temperature
float temp;

//Angles
float pitch = 0;
float roll = 0;

//Impact
float totalAcc = 0;

//Timing
unsigned long accidentStart = 0;
unsigned long lastLCDUpdate = 0;

//Thresholds

const float IMPACT_THRESHOLD = 12.0;
const float TILT_THRESHOLD = 15.0;

//==============================
// RGB Function
//==============================

void setColor(bool r, bool g, bool b) {
  digitalWrite(RED_LED, r);
  digitalWrite(GREEN_LED, g);
  digitalWrite(BLUE_LED, b);
}

//==============================
// WiFi Connection
//==============================

void connectWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");
  lcd.setCursor(0, 1);
  lcd.print("WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");

  delay(1500);
}

//==============================
// Auto Reconnect
//==============================

void reconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED)
    return;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reconnecting");
  lcd.setCursor(0, 1);
  lcd.print("WiFi...");

  WiFi.disconnect();
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected");
  delay(1000);
}

//==============================
// MPU Calibration
//==============================

void calibrateMPU() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Initializing");

  lcd.setCursor(0, 1);
  lcd.print("Keep Still");

  float sumX = 0;
  float sumY = 0;
  float sumZ = 0;

  sensors_event_t a, g, t;

  for (int i = 0; i < 200; i++) {
    mpu.getEvent(&a, &g, &t);

    sumX += a.acceleration.x;
    sumY += a.acceleration.y;
    sumZ += a.acceleration.z - 9.81;

    delay(10);
  }

  offsetX = sumX / 200.0;
  offsetY = sumY / 200.0;
  offsetZ = sumZ / 200.0;

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Calibration");

  lcd.setCursor(0, 1);
  lcd.print("Complete");

  delay(2000);
}

//==============================
// Setup
//==============================

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  setColor(LOW, LOW, LOW);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  if (!mpu.begin()) {
    lcd.clear();
    lcd.print("MPU Error");

    while (1)
      ;
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  calibrateMPU();

  connectWiFi();

  lcd.clear();
}
//======================================================
// PART 2
// HC-SR04 + LCD + RGB + Telegram Functions
//======================================================

// Read Distance from HC-SR04
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0)
    return 400;

  float distance = duration * 0.0343 / 2.0;

  return distance;
}

//------------------------------------------------------
// LCD Obstacle Display
//------------------------------------------------------

void displayObstacle(float distance) {
  static int lastZone = -1;

  int zone;

  if (distance > 30)
    zone = 0;
  else if (distance >= 10)
    zone = 1;
  else
    zone = 2;

  if (zone != lastZone) {
    lcd.clear();
    lastZone = zone;
  }

  lcd.setCursor(0, 0);
  lcd.print("Dist:");
  lcd.print((int)distance);
  lcd.print("cm   ");

  lcd.setCursor(0, 1);

  switch (zone) {
    case 0:
      lcd.print("NO OBSTACLE    ");
      setColor(LOW, HIGH, LOW);
      break;

    case 1:
      lcd.print("OBSTACLE AHEAD ");
      setColor(HIGH, HIGH, LOW);
      break;

    case 2:
      lcd.print("STOP!          ");
      setColor(HIGH, LOW, LOW);
      break;
  }
}

//------------------------------------------------------
// Read MPU6050
//------------------------------------------------------

void readMPU() {
  sensors_event_t a, g, t;

  mpu.getEvent(&a, &g, &t);

  ax = a.acceleration.x - offsetX;
  ay = a.acceleration.y - offsetY;
  az = a.acceleration.z - offsetZ;

  gx = g.gyro.x;
  gy = g.gyro.y;
  gz = g.gyro.z;

  temp = t.temperature;

  totalAcc = sqrt(ax * ax + ay * ay + az * az);

  pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180 / PI;

  roll = atan2(ay, sqrt(ax * ax + az * az)) * 180 / PI;
}

//------------------------------------------------------
// Telegram
//------------------------------------------------------

void sendTelegramAlert() {
  if (telegramSent)
    return;

  if (WiFi.status() != WL_CONNECTED)
    reconnectWiFi();

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  String message =
    "🚨 SMART VEHICLE EMERGENCY 🚨\n\n";

  message += "⚠️ Possible Vehicle Accident Detected\n\n";

  message += "📡 Device : ESP32\n";
  message += "🚗 Status : High Impact + Vehicle Tilt\n\n";

  message += "Impact : ";
  message += String(totalAcc, 1);
  message += " m/s²\n";

  message += "Pitch : ";
  message += String(pitch, 1);
  message += "°\n";

  message += "Roll : ";
  message += String(roll, 1);
  message += "°\n";

  message += "Obstacle : ";
  message += String(distanceCM, 1);
  message += " cm\n\n";

  message += "GPS : Not Available\n\n";

  message += "Please check on the driver immediately.";

  message.replace(" ", "%20");
  message.replace("\n", "%0A");

  String url =
    "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + message;

  if (https.begin(client, url)) {
    int httpCode = https.GET();

    if (httpCode > 0) {
      telegramSent = true;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Alert Sent");
      lcd.setCursor(0, 1);
      lcd.print("Drive Safe");

      delay(3000);

      lcd.clear();
    }

    https.end();
  }
}

//------------------------------------------------------
// Flash Red LED
//------------------------------------------------------

void flashRedLED() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);

  for (int i = 0; i < 10; i++) {
    digitalWrite(RED_LED, HIGH);
    delay(250);

    digitalWrite(RED_LED, LOW);
    delay(250);
  }
}

//------------------------------------------------------
// Countdown
//------------------------------------------------------

void countdown() {
  for (int i = 5; i >= 1; i--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ACCIDENT!");
    lcd.setCursor(0, 1);
    lcd.print("Alert in ");
    lcd.print(i);

    for (int j = 0; j < 4; j++) {
      digitalWrite(RED_LED, HIGH);
      delay(125);

      digitalWrite(RED_LED, LOW);
      delay(125);
    }
  }

  digitalWrite(RED_LED, HIGH);
}
//======================================================
// PART 3
// Accident Detection + Main Loop
//======================================================

// Check if accident conditions are satisfied
bool checkAccident() {
  bool impact = totalAcc >= IMPACT_THRESHOLD;

  bool tilt =
    abs(pitch) >= TILT_THRESHOLD || abs(roll) >= TILT_THRESHOLD;

  if (impact && tilt) {
    if (accidentStart == 0) {
      accidentStart = millis();
    }

    if (millis() - accidentStart >= 500) {
      accidentStart = 0;
      return true;
    }
  } else {
    accidentStart = 0;
  }

  return false;
}

//======================================================

void accidentRoutine() {
  accidentDetected = true;

  // Turn OFF Green & Blue immediately
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);

  countdown();

  sendTelegramAlert();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alert Sent");
  lcd.setCursor(0, 1);
  lcd.print("Drive Safe");

  delay(3000);

  accidentDetected = false;

  lcd.clear();
}

//======================================================

void loop() {
  reconnectWiFi();

  readMPU();

  distanceCM = getDistance();

  // Only update obstacle display when NOT in accident mode
  if (!accidentDetected) {
    displayObstacle(distanceCM);
  }

  if (!telegramSent && !accidentDetected) {
    if (checkAccident()) {
      accidentRoutine();
    }
  }

  // Reset alert after vehicle returns to normal
  if (telegramSent) {
    if (totalAcc < 12.0 && abs(pitch) < 10 && abs(roll) < 10) {
      telegramSent = false;
    }
  }

  delay(100);
}
