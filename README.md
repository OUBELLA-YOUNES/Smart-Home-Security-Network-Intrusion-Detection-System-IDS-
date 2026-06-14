# Smart-Home-Security-Network-Intrusion-Detection-System-IDS-
# PFE: Smart Home Security & Network Intrusion Detection System (IDS)

A comprehensive Smart Home Security platform engineered as a graduation project (Projet de Fin d'Études). This system bridges IoT hardware simulation, data confidentiality, real-time telemetry processing, and dedicated enterprise network threat mitigation.

## 📺 Project Walkthrough & Demo
Below is a full video demonstration going through the network traffic monitoring, hardware triggers, cryptographic handling, and automated incident responses:

https://github.com

## 🚀 System Architecture & Core Stack
* **Microcontroller Simulation:** Configured an **ESP32** framework simulated via Wokwi, managing DHT sensory arrays, PIR motion indicators, and a physical servo deadbolt.
* **Network Security (IDS):** Monitored using **Suricata IDS** to actively trace network flows and flag anomalous traffic or unauthorized payload injection attempts.
* **Data Encryption:** Integrated **Elliptic Curve Cryptography (ECC)** to provide end-to-end data encryption across device transmission lanes.
* **Data Telemetry:** **Node-RED** acts as the core message broker routing data feeds dynamically straight to an **InfluxDB** time-series database.
* **Incident Alerting:** Backed by **Twilio API** integrations to process instant data-driven alert pathways to SMS and WhatsApp.

## 🛠️ Repository Directory File Map
* `/src/main.cpp`: Main firmware execution layer integrating hardware constraints, cryptographic logic, and platform handling via PlatformIO.
* `/documents/PFE_Rapport.pdf`: Full technical PFE graduation report detailing architecture schematics, security analysis matrices, and system benchmarks.
* `/demo.mp4`: Complete end-to-end operational video presentation.

## 💻 Tech Stack Keywords
`PlatformIO` | `ESP32` | `Suricata IDS` | `Node-RED` | `InfluxDB` | `Twilio API` | `ECC Encryption`

---
*Developed as a graduation project (PFE) focusing on modern embedded computing constraints and network defense topologies.*
