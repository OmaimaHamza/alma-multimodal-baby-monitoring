<div align="center">
# ALMA Interface — Mobile Application

### The frontend dashboard for the ALMA baby-monitoring system.

[![Flutter](https://img.shields.io/badge/Flutter-02569B?style=for-the-badge&logo=flutter&logoColor=white)](https://flutter.dev/)
[![Firebase](https://img.shields.io/badge/Firebase-FFCA28?style=for-the-badge&logo=firebase&logoColor=black)](https://firebase.google.com/)
</div>

---

## Overview

This directory contains the **mobile application interface** for the ALMA project. Developed by [**Salma Garboune**](https://github.com/salmagne), the interface acts as the central hub for parents to monitor their baby's environment, view live feeds, and interact with the hardware.

The application is deployed and accessible at: **[https://baby-monitoring-d0bfc.web.app](https://baby-monitoring-d0bfc.web.app)**

### Architecture & Data Flow
The interface does not communicate directly with the hardware or the AI models. Instead, it uses **Firebase Realtime Database** as a central message broker. 

1. **Hardware (ESP32)** sends sensor data (temperature, humidity, movement) and video streams to Firebase.
2. **AI Models** (presence detection, covered-face detection, awake/sleep classifier) process the data and write their predictions to Firebase.
3. **The Interface** listens to Firebase for real-time updates and displays them to the user. It also sends commands (like adjusting the camera angle or playing a lullaby) back to Firebase, which the hardware reads.

## Interface Pages Breakdown

Below is a detailed breakdown of the application's pages and their connected components.

### 1. Profile Page
The Profile page displays the parent's account information and the baby's basic details.

<div align="center">
  <img width="449" height="800" alt="WhatsApp Image 2026-10-21 at 6 59 02 PM (2)" src="https://github.com/user-attachments/assets/4ced4be6-4709-41f7-b6e2-3081df2af6d3" />

</div>

**Key Components:**
*   **Parent Profile:** Displays the user's name, email, and account status.
*   **Baby's Details:** Shows the baby's name ("Alaa") and birthday. 
*   **Navigation:** Bottom navigation bar allows switching between the **Home** (monitoring) and **Profile** views.

### 2. Home Page — Live Stream & Controls
The Home page is the primary dashboard. The top section is dedicated to the live video feed and interaction buttons.

<div align="center">
  <img width="451" height="800" alt="WhatsApp Image 2026-10-21 at 6 59 02 PM" src="https://github.com/user-attachments/assets/a7d5bfc2-62d3-4e1c-99a0-d1f9fb86d363" />

</div>

**Key Components:**
*   **Header:** Displays the ALMA logo and a green **LIVE** indicator, confirming the app is successfully connected to the Firebase real-time stream.
*   **Start Live Stream:** A large video player area. When clicked, it initiates the video feed from the ESP32-CAM.
*   **Speak:** A push-to-talk button. *(Future/Hardware integration: Sends an audio signal to the hardware).*
*   **Lullaby:** Plays a pre-set audio track through the hardware to soothe the baby.
*   **Camera Angle:** A slider that sends control commands via Firebase to the hardware to physically adjust the camera's tilt (e.g., 30°).

### 3. Home Page — AI Status & Room Sensors
Scrolling down on the Home page reveals the real-time AI predictions and environmental data.

<div align="center">
  <img width="449" height="800" alt="WhatsApp Image 2026-10-21 at 6 59 02 PM (1)" src="https://github.com/user-attachments/assets/63eaea37-0abb-496c-841b-91d288dfe53b" />
</div>

**Key Components:**
*   **AI Baby Status Card ("Alaa"):** 
    *   Shows the current state: **Is Sleeping 😴** (Powered by the *Awake/Sleep Classifier*).
    *   **Present:** A green indicator confirming the baby is in the crib (Powered by the *Presence Detection* model).
    *   **Not Covered:** A green indicator confirming the baby's face is clear (Powered by the *Covered-Face Detection* model).
*   **Room Sensors:**
    *   **Temperature:** 29.0°C
    *   **Humidity:** 30.2%
    *   **Air Quality:** 4095 ppm
    *   **Movement:** "Stable" (Powered by the PIR sensor, indicating if the baby is moving or immobile).

## Firebase Data Mapping

For developers connecting to this interface, here is how the UI elements map to the Firebase Realtime Database structure:

| UI Element | Firebase Path (Example) | Source |
| :--- | :--- | :--- |
| Live Video / Presence | `realtime_video/baby_presence` | AI: Presence Detection |
| "Not Covered" Status | `realtime_video/face_coverage` | AI: Covered-Face Detection |
| "Is Sleeping/Awake" | `baby_room/movement_detection` | AI: Awake/Sleep Classifier |
| Temperature / Humidity | `baby_room/sensors/temp`, `hum` | Hardware: DHT Sensor |
| Air Quality | `baby_room/sensors/air` | Hardware: Gas Sensor |
| Movement Status | `baby_room/sensors/movement` | Hardware: PIR Sensor |
| Camera Angle | `control/camera_angle` | Interface -> Hardware |
