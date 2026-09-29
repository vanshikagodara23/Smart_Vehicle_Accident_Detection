# Smart Vehicle Accident Detection

Smart vehicle safety system using an ESP32. It warns about obstacles on an LCD with an RGB LED, detects accidents with an MPU6050, and sends an emergency alert on Telegram.

## How it works

**Obstacle alert (HC-SR04 + LCD + RGB LED)**

| Distance | LED colour | LCD message |
|---|---|---|
| More than 30 cm | Green | NO OBSTACLE |
| 10 – 30 cm | Yellow | OBSTACLE AHEAD |
| Less than 10 cm | Red | STOP! |

**Accident detection (MPU6050)**

1. On startup the MPU6050 is calibrated (keep the device still while "Keep Still" is shown).
2. An accident is detected when there is a **high impact** (≥ 12 m/s²) **and** the vehicle is **tilted** (pitch or roll ≥ 15°) for at least 0.5 seconds.
3. The LCD shows a 5-second countdown with a flashing red LED.
4. A Telegram message is sent with the impact, pitch, roll and obstacle distance.
5. The alert resets once the vehicle is back to normal.

WiFi reconnects automatically if the connection drops.

## Components

- ESP32 development board
- MPU6050 accelerometer / gyroscope
- HC-SR04 ultrasonic sensor
- 16x2 LCD with I2C module
- RGB LED + 3 resistors (220 Ω)
- Jumper wires, breadboard

## Wiring

| Part | Pin | ESP32 pin |
|---|---|---|
| HC-SR04 | TRIG | 5 |
| HC-SR04 | ECHO | 18 |
| RGB LED | Red | 27 |
| RGB LED | Green | 25 |
| RGB LED | Blue | 26 |
| MPU6050 + LCD | SDA | 21 |
| MPU6050 + LCD | SCL | 22 |

## Libraries

- LiquidCrystal_I2C
- Adafruit MPU6050
- Adafruit Unified Sensor

## Setup

Fill in these values in the code before uploading:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const char* BOT_TOKEN = "YOUR_BOT_TOKEN";   // from @BotFather
const char* CHAT_ID = "YOUR_CHAT_ID";
```

## How to run

1. Install the libraries above from the Library Manager.
2. Select an ESP32 board in the Arduino IDE.
3. Fill in the WiFi and Telegram details.
4. Upload and keep the device still during calibration.
