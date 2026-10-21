## Experimental AI components for baby-room monitoring: baby-presence detection and covered-face detection.

## Overview

This directory contains the **AI-side components** of the ALMA project. It groups the computer-vision models, the experimental inference servers, and the supporting scripts used to prototype baby-monitoring behaviors.

The models are **research prototypes**. They are included in the repository to document the approach, the design decisions, and the intermediate results obtained during the academic project. They are **not production-ready** and have **not been clinically or safety validated**.

The purpose of this folder is to make the work reproducible and to expose the current state of the models so that future iterations can build on it.

## Scope of this directory

The AI-side work addresses three complementary monitoring concerns:

| Component | Purpose | Status |
|---|---|---|
| **Baby-Presence Detection** | Detect whether a baby is present in the monitored area (frame-level face detection). | Prototype — functional in controlled conditions. |
| **Covered-Face Detection** | Identify frames in which the baby's face appears to be covered (blanket, fabric, occlusion). | Prototype — experimental, accuracy sensitive to lighting and camera angle. |
| **Awake/Sleep + Movement Classification** | Combine a retrained image classifier with a movement-history signal to estimate whether the baby is awake and immobile for a prolonged period. | Experimental — retrained classifier in place, silence-risk logic still being tuned. |

Each component is independent and can be executed or evaluated separately. None of them has been trained or evaluated on a large, representative, clinically annotated dataset.


## Component summaries

### 1. Baby-presence detection

**Approach.** A YOLOv8-nano model fine-tuned for face detection (`yolov8n-face.pt`) is applied to each frame received from the ESP32-CAM MJPEG stream. A face count greater than zero is interpreted as "baby present."

**Runtime.** Real-time local inference on a desktop or laptop, with the annotated feed displayed via OpenCV and the presence flag published to Firebase Realtime Database under `realtime_video/baby_presence`.

**Inputs.** Live MJPEG stream from an ESP32-CAM on port `81` (`/stream`).

**Outputs.** Annotated local window + Firebase boolean.

**Status.** Functional prototype. Detection quality depends on the ESP32 camera's resolution, distance to the baby, and lighting. Current implementation writes to Firebase on every processed frame — this should be debounced in a future iteration.

**Notes.** The script includes three fallback connection strategies (HTTP `requests`, raw socket, and OpenCV/FFmpeg) because ESP32-CAM firmware behavior can vary between builds.

### 2. Covered-face detection

**Approach.** A two-stage pipeline:

1. **Face localization** — Haar Cascade classifier locates the largest plausible face in the frame.
2. **Coverage classification** — a texture-based classifier estimates whether the face region is covered by computing four normalized metrics:
   - global variance (uniform regions → blanket),
   - 3×3 regional variance (local uniformity),
   - Canny edge density (facial features → high density),
   - HSV skin-color ratio (visible skin vs fabric).

A weighted composite score is thresholded, and a **temporal majority vote** over a rolling buffer reduces flicker.

**Inputs.** A video file or stream.

**Outputs.** Annotated video with per-frame covered/visible labels, plus aggregate coverage statistics.

### 3. Awake/sleep classifier + silence-risk

**Approach.** A TensorFlow image classifier — retrained from a pretrained MobileNet/Inception backbone — predicts a class (`awake` / `sleeping` and potentially others listed in `retrained_labels.txt`). A Flask server receives a JPEG frame, runs inference against the pre-loaded graph, and publishes both the predicted class and the confidence to Firebase.

**Silence-risk logic.** A downstream rule combines:
- **prolonged absence of detected movement** (from the PIR sensor, via the hardware side), and
- **an `awake` prediction** from the classifier,

into an "immobile-but-awake" alert condition. This rule is part of the application logic, not the classifier itself.

**Inputs.** Raw JPEG frames posted to `POST /upload`.

**Outputs.** Firebase update at `baby_room/movement_detection` containing a one-hot prediction, confidence, and timestamp.

**Status.** The classifier runs correctly and produces predictions; the retraining dataset is small, and the silence-risk thresholds are still being calibrated. The current server is a **local research server**, not a deployed backend.

## Requirements

- **Python** 3.10+ recommended.
- **TensorFlow 1.x graph mode** — the awake/sleep server uses `tensorflow.compat.v1` with `disable_v2_behavior()` so that the pre-existing `retrained_graph.pb` can be loaded without conversion.
- **PyTorch 2.6+** — required for the presence-detection model; note that PyTorch 2.6 tightened model deserialization and the script explicitly registers safe globals (`DetectionModel`, `Sequential`, `Conv2d`).
- **OpenCV** with FFmpeg support (for the MJPEG stream path).
- **Firebase service account** — a `baby-monitoring.json` service-account file with write access to the target Realtime Database.


## Configuration

The following values must be provided before running the components:

| Parameter | Where it is used | Notes |
|---|---|---|
| ESP32-CAM IP address | `presence-detection/test_esp32_stream.py` (`ESP32_IP`) | Must be on the same network as the inference machine. |
| Firebase service-account path | All components | `baby-monitoring.json` — not committed to the repository. |
| Firebase database URL | `working_server.py` and stream script | Must match your project. |
| Model file paths | Each component | Models are not distributed in the repository; see each component README. |


## Running the components

Each component is standalone and has its own prerequisites. See the component-specific README for exact commands. At a high level:

```bash
# 1. Presence detection (live ESP32-CAM stream)
python presence-detection/test_esp32_stream.py

# 2. Covered-face detection (local video)
jupyter notebook covered-face-detection/baby-face-coverage-detection.ipynb

# 3. Awake/sleep classifier server
python awake-sleep-classifier/working_server.py
```
