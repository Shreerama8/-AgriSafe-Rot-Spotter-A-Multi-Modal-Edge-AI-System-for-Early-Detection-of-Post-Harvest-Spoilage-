# -AgriSafe-Rot-Spotter-A-Multi-Modal-Edge-AI-System-for-Early-Detection-of-Post-Harvest-Spoilage-


# 🌱 AgriSafe – Smart Agriculture Safety Monitoring System

An AI + IoT based real-time monitoring system that detects environmental hazards and unsafe conditions in agricultural fields and indoor farming environments.

The system uses **ESP32-CAM + Sensors + Edge AI** to monitor temperature, humidity, air quality and detect objects such as humans/animals/fire locally without internet.

---

## 🚀 Features

* 🌡 Temperature Monitoring (DHT11)
* 💧 Humidity Monitoring
* 🌫 Air Quality Detection (CO₂ & TVOC / MQ135)
* 🤖 AI Object Detection (Edge Impulse Model)
* 📡 Wireless Communication (ESP-NOW)
* 📊 Real-time Dashboard Display
* 🔔 Safety Alerts
* 🌐 Works without Internet (Edge Computing)
* 🧠 Dual Core Multitasking (ESP32)

---

## 🧠 System Architecture

Sensor Node (ESP32-CAM)
→ Collects environment data
→ Runs AI model locally
→ Sends data via ESP-NOW

Receiver Node (ESP32)
→ Receives data
→ Displays on dashboard / LCD

---

## 🛠 Hardware Used

| Component       | Purpose                |
| --------------- | ---------------------- |
| ESP32-CAM       | AI vision processing   |
| ESP32 Dev Board | Receiver & dashboard   |
| DHT11           | Temperature & humidity |
| SGP30           | Air quality sensing    |
| Relay + Fan     | Automatic ventilation  |
| 16x2 LCD        | Local display          |
| Power Supply    | 5V regulated           |

---

## 🧩 Software & Tools

* Arduino IDE
* Edge Impulse
* ESP32 Arduino Core
* GitHub
* Embedded C++

---

## 📂 Project Structure

```
AgriSafe/
 ├── sender_ai_node/
 │    └── agrisafe_camera.ino
 ├── receiver_dashboard/
 │    └── receiver.ino
 ├── lcd_monitor/
 │    └── air_quality_monitor.ino
 ├── images/
 └── README.md
```

---

## ⚙️ How It Works

1. Sensors read environmental data
2. ESP32-CAM captures image
3. Edge AI model detects hazards
4. Data sent wirelessly via ESP-NOW
5. Dashboard shows real-time status
6. Fan activates automatically if temperature rises

---

## 📊 Example Output

Temperature : 31.4°C
Humidity : 72%
CO₂ : 450 ppm
Air Quality : Good
Detected : bad onion 

---

## 🔧 Installation

### 1️⃣ Install Arduino ESP32 Board

Add board URL in Arduino IDE:

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

### 2️⃣ Install Libraries

Install from Library Manager:

* DHT sensor library
* SparkFun SGP30
* ESP32 Camera
* Edge Impulse SDK

### 3️⃣ Upload Code

Upload:

* Sender code → ESP32-CAM
* Receiver code → ESP32 board

---

## 🧪 Applications

* Smart Farming
* Greenhouse Monitoring
* Storage Safety Monitoring
* Livestock Protection
* Industrial Safety

---

## 🎯 Future Improvements

* Cloud dashboard (IoT)
* Mobile App alerts
* GPS tracking
* Weather prediction integration

---

## 👨‍💻 Author

**Shreerama TD**

Electronics & Communication Engineering
IoT | Embedded Systems | Robotics Enthusiast

---

## 📜 License

This project is open-source and free to use for educational purposes.
