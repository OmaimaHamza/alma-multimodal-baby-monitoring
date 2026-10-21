# ALMA Hardware

This directory contains the embedded firmware for the ALMA multimodal baby-monitoring prototype. It connects environmental sensors, motion detection, camera-based monitoring, and AI processing through Firebase Realtime Database.

## Hardware Architecture

ALMA uses two microcontrollers with separate responsibilities:

| Component | Responsibilities |
|---|---|
| **ESP32-CAM** | Image capture, MJPEG video streaming, Wi-Fi connectivity, Firebase communication, and sending captured images to the Python server |
| **Arduino Uno** | Sensor acquisition, motion monitoring, servo control, OLED status display, and Firebase-based monitoring updates |

The two boards contribute to the overall monitoring workflow and interact with the software components through the configured communication services.

## Repository Structure

```text
hardware/
├── README.md
├── esp32-cam/
│   ├── README.md
│   ├── esp32-cam.ino
│   ├── base64.cpp
│   └── base64.h
└── arduino-uno/
    ├── README.md
    └── arduino-uno.ino
```

## Hardware Components

The firmware provided in this repository uses or configures the following components:

| Component | Purpose |
|---|---|
| ESP32-CAM | Camera capture and video streaming |
| Arduino Uno | Sensor monitoring and hardware control |
| DHT22 | Temperature and humidity measurements |
| MQ-135 | Analog gas/air-quality-related readings |
| PIR motion sensor | Motion detection |
| SSD1306 OLED, 128 × 64 | Displays ALMA status and alerts |
| Servo motor | Position control through Firebase to control the camera angle |
| Wi-Fi network | Connectivity to Firebase and the Python server |

## 1. ESP32-CAM

**Directory:** `esp32-cam/`

The ESP32-CAM firmware handles camera operations and communication with the AI processing server.

### Main functionalities

- Initializes the camera using the AI-Thinker ESP32-CAM pin configuration.
- Connects to Wi-Fi.
- Initializes Firebase authentication and Realtime Database access.
- Publishes the camera's local IP address, stream port, stream URL, and status.
- Exposes an MJPEG video stream through the `/stream` endpoint on port `81`.
- Checks Firebase for image-capture requests.
- Captures a JPEG image when a capture request is received.
- Sends captured images to the Python server through an HTTP POST request to `/upload`.
- Resets the capture request after a capture attempt.
- Applies a capture cooldown to limit repeated captures.

### Video streaming

The camera stream is exposed at:

```text
http://<ESP32_CAM_IP>:81/stream
```

The camera's IP address must be reachable from the device accessing the stream.

### Image capture and AI integration

When the Firebase field `baby_room/camera/capture_request` is set to `1`, the ESP32-CAM captures an image and sends it to the configured Python server.

The image is transmitted as JPEG data with the `Content-Type: image/jpeg` header.

The Python server address and port must match the server's actual network configuration.

## 2. Arduino Uno

**Directory:** `arduino-uno/`

The Arduino firmware monitors environmental conditions and motion, controls the servo motor, and displays monitoring information on the OLED screen.

### Sensor monitoring

The firmware reads:

- Temperature from the DHT22 sensor.
- Relative humidity from the DHT22 sensor.
- An analog reading from the MQ-135 sensor.
- Motion state from the PIR sensor.

Temperature, humidity, and gas readings are sent to Firebase approximately every 30 seconds when Firebase is ready and the temperature and humidity readings are valid.

The values are written to:

```text
/baby_room/sensors
```

### Motion detection

The PIR sensor is used to track motion and the transition to an inactive state.

The firmware updates the following Firebase fields:

```text
/baby_room/movement_detection/active
/baby_room/movement_detection/timestamp
/baby_room/camera/capture_request
/baby_room/camera/emergency_capture
```

When motion stops, the firmware starts an inactivity timer. Once the configured threshold is reached, it requests an image capture.

The emergency-capture flag is set according to the value read from:

```text
/baby_room/movement_detection/awake
```

This is a software decision rule, not a direct measurement of the baby's breathing or medical condition.

### OLED display and alerts

The SSD1306 OLED displays the ALMA startup screen, connection status, and monitoring information.

The firmware reads AI-related status values from Firebase and applies the following display priority:

1. `baby_covered == 1` → `ALERT: COVERED`
2. `baby_presence == 0` → `NO BABY`
4. Otherwise → ALMA animation

### Servo control

The firmware reads the requested servo angle from:

```text
/baby_room/servo/angle
```

The requested value is limited to the range from 0° to 180° before being sent to the servo.

## Firebase Integration

Both firmware components use Firebase Realtime Database to exchange monitoring information.

The code references fields for sensor readings, motion state, camera capture requests, camera network information, servo position, baby presence and covered-face detection.

The database structure should match the paths expected by the firmware and the AI/backend implementation.

## Software Requirements

- Arduino IDE
- ESP32 board support package
- The Arduino libraries required by the respective firmware
- A configured Firebase project with Realtime Database
- A Wi-Fi network accessible to the ESP32-CAM
- A reachable Python server for image processing you will find it on the `/ai-modesls` folder  


## Setup Overview

1. Install the Arduino IDE and the required board support packages.
2. Install the libraries required by each firmware.
3. Configure the Wi-Fi and Firebase settings.
4. Configure the Python server IP address and port in the ESP32-CAM firmware.
5. Verify the sensor pins and wiring.
6. Upload each firmware to its corresponding board.
7. Check the serial output for initialization and connection status.
8. Verify Firebase updates, camera streaming, image capture, and OLED operation.


## Limitations

- The camera stream and image capture depend on network availability and camera operation.
- The MQ-135 provides a sensor reading that requires appropriate calibration and interpretation; it is not a direct medical-grade air-quality measurement.
- PIR motion detection does not measure breathing.
- The OLED alerts display software detection results and do not establish that the baby is medically safe.

ALMA is an experimental monitoring prototype, not a certified medical device or a substitute for direct supervision and safe-sleep practices.

---

**Part of the ALMA project — Multimodal AI and IoT for Baby-Room Monitoring.**
