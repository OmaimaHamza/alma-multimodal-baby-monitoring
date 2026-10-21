
<div align="center">

# ALMA — Multimodal AI Baby Monitoring

### An AI-powered IoT prototype for intelligent baby-room monitoring and parental assistance.

[![Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white)](https://www.python.org/)
[![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![ESP32](https://img.shields.io/badge/ESP32-CAM-E7352C?style=for-the-badge&logo=espressif&logoColor=white)](https://www.espressif.com/)
[![Computer Vision](https://img.shields.io/badge/Computer_Vision-6C5CE7?style=for-the-badge)](https://opencv.org/)
[![Flutter](https://img.shields.io/badge/Flutter-02569B?style=for-the-badge&logo=flutter&logoColor=white)](https://flutter.dev/)

[AI Models](ai-models/README.md) ·
[Hardware](hardware/README.md) ·
[Interface Demo](interface/README.md)  

<img width="588" height="448" alt="3d model" src="https://github.com/user-attachments/assets/b7775456-f175-4fb3-bd12-03843f03f84b" />

</div>

---

## Overview

ALMA is a multimodal AI and IoT prototype designed to support parents through intelligent baby-room monitoring.

The system combines embedded hardware, environmental sensing, computer vision, and artificial intelligence to interpret selected situations and provide relevant information through an interactive interface.

Rather than relying exclusively on conventional monitoring, ALMA explores how multiple data sources can be combined to provide contextual assistance.

**ALMA is intended to assist parents, not replace their judgment.** It does not make decisions on behalf of the mother.

## Project Objectives

- Explore intelligent baby-room monitoring using embedded systems and AI.
- Detect situations involving prolonged inactivity and an apparently awake baby.
- Identify the presence of a baby within the monitored area.
- Detect when the baby's face appears to be covered.
- Integrate hardware events and AI predictions into a unified system.
- Provide an interactive interface for presenting monitoring information.

## System Architecture

ALMA is organized around four components:

1. **Environmental sensing** — collects selected room and motion measurements.
2. **Camera-based monitoring** — captures images and provides visual input.
3. **AI processing** — analyzes visual information to identify predefined situations.
4. **Interactive interface** — presents system information to the user.

## AI Models

| Model | Purpose |
|---|---|
| Silence-Risk Detection | Combines prolonged absence of detected movement with an image-based awake/sleep classification. |
| Baby-Presence Detection | Detects whether a baby is present in the monitored area. |
| Covered-Face Detection | Identifies visual situations in which the baby's face appears to be covered. |

The AI entry point is `ai-models/main.py`, which coordinates the model execution workflow.

See [`ai-models/README.md`](ai-models/README.md) for model-specific documentation, dependencies, configuration, and execution instructions.

## Hardware

The prototype separates hardware responsibilities between two microcontrollers:

- **ESP32-CAM** — camera access and image capture.
- **Arduino Uno** — sensor acquisition and hardware control.
- **DHT22**
- **PIR**
- **OLED display screen**
-  

The repository provides separate firmware directories for each board.

### 3D Hardware Design

<div align="center"><img width="521" height="448" alt="image" src="https://github.com/user-attachments/assets/20c2ecc0-3c4e-476e-ae03-934dc28c532b" /></div>


*3D representation of the ALMA hardware prototype.*  
[prototype in 3D](https://sketchfab.com/3d-models/prototype-du-projet-1-7858f0d72e90454abaf318e3129ad686)  
See [`hardware/README.md`](hardware/README.md) for the components, wiring documentation, firmware, and setup instructions.

## Interface Simulation

The project includes a Flutter-based interface simulation developed by [Salma Garboune](https://github.com/salmagne).

**[Open the ALMA Interface Simulation](https://baby-monitoring-d0bfc.web.app/)**

The simulation allows visitors to explore the project's interface without requiring access to the Flutter source code.

Access instructions and any demo-account requirements are provided in [`interface/README.md`](interface/README.md).

## Repository Structure

```text
hardware/     Embedded firmware
ai-models/    AI models and orchestration
interface/    Flutter simulation access
docs/         Architecture, hardware visuals, and documentation
```

Each component directory contains its own README with the corresponding technical details.

## Getting Started

1. Review the system architecture in `docs/README.md`.
2. Check the required components and firmware instructions in `hardware/README.md`.
3. Install the AI dependencies listed in `ai-models/requirements.txt`.
4. Configure the required model paths, camera connection, and backend settings.
5. Follow the execution instructions in `ai-models/README.md`.
6. Open the interface simulation using the link in `interface/README.md`.

The complete system may require physical hardware, model weights, camera access, and external service configuration. Consult each component's documentation before attempting an end-to-end run.

## Current Limitations

- The prototype's detection accuracy depends on camera conditions, model performance, and the tested environment.
- A single captured image cannot establish whether a baby is breathing normally.
- Visual face-cover detection does not confirm airway obstruction or diagnose suffocation.
- Environmental and motion sensors provide limited information about the baby's physiological state.
- The prototype has not been established as a medically validated or safety-certified monitoring device.

ALMA must not be used as a substitute for direct supervision, safe-sleep practices, or professional medical advice.

## Future Improvements

- Improve detection robustness under different lighting and camera conditions.
- Evaluate false positives and false negatives on representative test data.
- Explore temporal analysis for situations that cannot be assessed reliably from a single image.
- Improve hardware efficiency and monitoring reliability.
- Expand documentation, automated testing, and reproducible deployment.

## Project Status

**Academic prototype.**

The repository documents the implemented components and distinguishes them from planned improvements.

## Contributors

- **Project development:** [Oumaima Hamza]([YOUR_GITHUB_PROFILE_URL](https://github.com/OmaimaHamza))
- **Flutter interface:** [Salma Garboune](https://github.com/salmagne)
- **Academic supervision:** [Mohammed Ameksa]([SUPERVISOR_PROFILE_URL](https://github.com/mameksa-fssm))

## License

This project is licensed under the [MIT License](LICENSE).

Copyright (c) 2026 ALMA Project Contributors

You are free to use, copy, modify, merge, publish, distribute, sublicense, subject to the terms and conditions of the MIT License.

See the [LICENSE](LICENSE) file for the full license text.

<div align="center">

**ALMA — Intelligent Monitoring, Designed to Support Parents**

</div>
