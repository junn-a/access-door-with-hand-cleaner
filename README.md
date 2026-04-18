# 🚪🧴 Access Door with Hand Cleaning System

> **An Arduino-based access control system that enforces mandatory hand sanitizer compliance before allowing entry into production areas.**

[![Arduino](https://img.shields.io/badge/Platform-Arduino%20Uno-00979D?style=flat-square&logo=arduino)](https://www.arduino.cc/)
[![Sensor](https://img.shields.io/badge/Sensor-MQ--3%20Alcohol-orange?style=flat-square)]()
[![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)]()
[![Status](https://img.shields.io/badge/Status-Production%20Ready-brightgreen?style=flat-square)]()

---

## 📋 Table of Contents

- [Overview](#-overview)
- [How It Works](#-how-it-works)
- [Hardware Requirements](#-hardware-requirements)
- [Wiring Diagram](#-wiring-diagram)
- [System Flowchart](#-system-flowchart)
- [Software Setup](#-software-setup)
- [Configuration](#-configuration)
- [Full Source Code](#-full-source-code)
- [Threshold Calibration](#-threshold-calibration)
- [Troubleshooting](#-troubleshooting)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🎯 Overview

This system ensures that **every employee must apply hand sanitizer (alcohol-based)** before entering a production room. The MQ-3 alcohol gas sensor detects the presence of alcohol vapor from the sanitizer. Only when sufficient alcohol is detected will the Arduino trigger the relay module to unlock the magnetic door lock.

**Key Benefits:**
- ✅ Enforces hygiene compliance automatically — no human supervision needed
- ✅ Fully standalone — works offline, no internet or server required
- ✅ Low cost — uses widely available, off-the-shelf components
- ✅ Easy to calibrate per environment
- ✅ Serial monitor logging for compliance audit trail

---

## ⚙️ How It Works

1. **Employee approaches** the door and places their hand under the hand sanitizer dispenser
2. **They pump sanitizer** onto their hands — this releases alcohol vapor
3. **The MQ-3 sensor** continuously reads analog voltage — alcohol vapor increases the output value
4. **Arduino evaluates** the sensor reading against a configurable threshold
5. **If threshold is met** → relay activates → magnetic lock releases → door opens for 3 seconds → door re-locks
6. **If threshold is NOT met** → door stays locked → event is logged to Serial monitor
7. **System returns to idle** state and waits for next attempt

---

## 🔧 Hardware Requirements

| Component | Specification | Quantity |
|---|---|---|
| Arduino Uno | ATmega328P, 5V logic | 1 |
| MQ-3 Alcohol Sensor | Analog + Digital output | 1 |
| Relay Module | 5V trigger, 10A/250VAC | 1 |
| Magnetic Door Lock | 12V DC, 500mA–1A | 1 |
| Power Supply | 12V DC regulated | 1 |
| Jumper Wires | Male-to-Male & Male-to-Female | As needed |
| Breadboard (optional) | For prototyping | 1 |

> **Note:** The MQ-3 requires a 24–48 hour burn-in period when used for the first time before readings stabilize.

---

## 🔌 Wiring Diagram

```
MQ-3 Sensor          Arduino Uno           Relay Module
───────────          ───────────           ────────────
VCC    ─────────────► 5V                   
GND    ─────────────► GND     ◄────────────── GND
AOUT   ─────────────► A0                   
                      D7     ─────────────► IN (signal)
                      5V     ─────────────► VCC

12V Power Supply      Relay Module          Magnetic Door Lock
────────────────      ────────────          ──────────────────
+12V   ─────────────► COM
                      NO     ─────────────► + (positive)
GND    ─────────────────────────────────► - (negative)
```

### Pin Summary

| Arduino Pin | Connected To | Wire Color (suggested) |
|---|---|---|
| 5V | MQ-3 VCC | Red |
| 5V | Relay VCC | Red |
| GND | MQ-3 GND | Black |
| GND | Relay GND | Black |
| A0 | MQ-3 AOUT | Yellow/Green |
| D7 | Relay IN | Blue |
| D13 | Status LED (optional) | White |

> ⚠️ **Important:** The magnetic door lock runs on **12V**. Never connect it directly to Arduino's 5V pins. Always use the relay to switch the 12V circuit separately.

---

## 📊 System Flowchart

```
                    ┌─────────────────────┐
                    │   System Power ON    │
                    └──────────┬──────────┘
                               │
                    ┌──────────▼──────────┐
                    │  Initialize Arduino  │
                    │  Warm up MQ-3 sensor │
                    └──────────┬──────────┘
                               │
                    ┌──────────▼──────────┐
                    │  Wait for hand near  │◄──────────────────┐
                    │  sanitizer dispenser │                   │
                    └──────────┬──────────┘                   │
                               │                              │
                    ┌──────────▼──────────┐                   │
                    │  Read MQ-3 sensor    │                   │
                    │  analogRead(A0)      │                   │
                    └──────────┬──────────┘                   │
                               │                              │
                    ┌──────────▼──────────┐                   │
                    │  Value ≥ threshold?  │                   │
                    └────┬──────────┬─────┘                   │
                    YES  │          │ NO                       │
                         │          │                          │
          ┌──────────────▼──┐   ┌───▼──────────────┐         │
          │   UNLOCK DOOR   │   │   DENY ENTRY      │         │
          │   Relay → LOW   │   │   Relay stays HIGH│         │
          └──────────┬──────┘   └───┬───────────────┘         │
                     │             │                           │
          ┌──────────▼──────┐   ┌───▼──────────────┐         │
          │  Wait 3 seconds  │   │  Log denial event │         │
          │  (door stays open│   │  Serial.print()   │         │
          └──────────┬──────┘   └───┬───────────────┘         │
                     │             │                           │
          ┌──────────▼──────┐      │                           │
          │   LOCK DOOR     │      │                           │
          │   Relay → HIGH  │      │                           │
          └──────────┬──────┘      │                           │
                     └─────────────┴──────────────────────────┘
                              Return to idle
```

---

## 💻 Software Setup

### Prerequisites

- [Arduino IDE](https://www.arduino.cc/en/software) v1.8.x or v2.x
- No external libraries required — uses Arduino core only

### Installation

1. **Clone this repository:**
   ```bash
   git clone https://github.com/your-username/access-door-hand-cleaning.git
   cd access-door-hand-cleaning
   ```

2. **Open the sketch:**
   ```
   Open Arduino IDE → File → Open → src/access_door.ino
   ```

3. **Select board:**
   ```
   Tools → Board → Arduino AVR Boards → Arduino Uno
   ```

4. **Select port:**
   ```
   Tools → Port → (your COM port, e.g. COM3 or /dev/ttyUSB0)
   ```

5. **Upload:**
   Press the **Upload** button (→) or `Ctrl+U`

6. **Open Serial Monitor** to view live logs:
   ```
   Tools → Serial Monitor → 9600 baud
   ```

---

## ⚙️ Configuration

All configurable parameters are at the top of `access_door.ino`:

```cpp
// ─── CONFIGURATION ───────────────────────────────────────────────
#define MQ3_PIN         A0    // Analog pin for MQ-3 sensor
#define RELAY_PIN       7     // Digital pin controlling the relay
#define LED_PIN         13    // Built-in LED (status indicator)

#define ALCOHOL_THRESHOLD  300  // Sensor value to trigger door unlock
                                // Range: 0–1023. Increase if false positives occur.
                                // Decrease if sensor is too sensitive.

#define DOOR_OPEN_DELAY  3000   // Milliseconds door stays unlocked (default: 3s)
#define SENSOR_WARMUP    2000   // Milliseconds to wait on boot for sensor warmup
#define POLL_INTERVAL    100    // Milliseconds between sensor readings
// ─────────────────────────────────────────────────────────────────
```

---

## 🎚️ Threshold Calibration

The most critical step for reliable operation is setting the correct `ALCOHOL_THRESHOLD`.

### Step 1 — Enable live sensor output

Uncomment this line in `checkSensor()`:
```cpp
Serial.print("Sensor: "); Serial.println(sensorValue);
```

### Step 2 — Baseline reading (no sanitizer)

Open Serial Monitor, observe the idle value. Typical: **50–150**

### Step 3 — Active reading (with sanitizer)

Pump sanitizer normally and hold hand near sensor. Observe peak value. Typical: **300–700+**

### Step 4 — Set threshold

Set `ALCOHOL_THRESHOLD` to approximately **60–70% of your peak reading**.

```
Example: idle=80, peak=500 → threshold = 300
```

### Step 5 — Re-comment the debug line

```cpp
// Serial.print("Sensor: "); Serial.println(sensorValue);
```

---

## 🛠️ Troubleshooting

| Symptom | Likely Cause | Solution |
|---|---|---|
| Door always unlocked | Threshold too low | Increase `ALCOHOL_THRESHOLD` |
| Door never unlocks | Threshold too high, or sensor cold | Lower threshold, wait for warmup |
| Sensor reads ~0 constantly | Wiring issue | Check AOUT → A0 connection |
| Relay clicks but door doesn't open | 12V not connected to relay COM | Check PSU → Relay COM wire |
| Door opens for wrong duration | Wrong `DOOR_OPEN_DELAY` value | Adjust in milliseconds (3000 = 3s) |
| Serial monitor shows garbage | Wrong baud rate | Set Serial Monitor to 9600 baud |

---

## 📁 Project Structure

```
access-door-hand-cleaning/
├── src/
│   └── access_door.ino       # Main Arduino sketch
├── docs/
│   ├── wiring_diagram.png    # Hardware connection diagram
│   └── flowchart.png         # System logic flowchart
├── README.md
└── LICENSE
```

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome!

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature`
3. Commit your changes: `git commit -m 'Add some feature'`
4. Push to the branch: `git push origin feature/your-feature`
5. Open a Pull Request

---

## 📜 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

---

## 📬 Contact

Built for industrial hygiene compliance automation. Feel free to open an issue for questions or improvements.

---

*Made with ❤️ for safer production environments.*
