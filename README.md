# TrainSim‑Controller  
**USB HID Train Simulator Controller for PC**

This project implements a custom **USB game controller** designed specifically for train simulator software on PC. It uses an **Arduino‑compatible microcontroller** configured as a **USB HID device**, allowing physical levers, switches, and buttons to control throttle, brake, horn, lights, and other locomotive functions.

The goal is to provide a realistic, tactile driving experience for simulators such as Train Simulator Classic, Train Sim World, OpenRails, and others.

---

## ✨ Features

- **USB HID Game Controller**  
  Recognized natively by Windows — no drivers required.

- **Multiple Control Inputs**  
  Supports:
  - Throttle lever  
  - Dynamic brake / train brake  
  - Independent brake  
  - Reverser  
  - Horn, bell, lights, wipers, etc.  
  - Additional switches and momentary buttons

- **Configurable Command Mapping**  
  Includes a `Comand Codes.txt` file with all supported command codes.

- **High‑Resolution Analog Inputs**  
  Smooth lever movement using ADC inputs.

- **Modular Design**  
  Easy to expand with more buttons, toggles, or rotary encoders.

- **PlatformIO Project Structure**  
  Clean, maintainable codebase with separate `include`, `src`, and `lib` directories.

---

## 🛠 Hardware Overview

| Component | Description |
|----------|-------------|
| **MCU** | Arduino Pro Micro / Leonardo (ATmega32u4 USB HID) |
| **Inputs** | Potentiometers for levers, switches, buttons |
| **USB Interface** | Native USB HID joystick |
| **Documentation** | `Train Controller.pdf` (wiring & mechanical design) |

---

## 📁 Project Structure

```
TrainSim-Controller/
├── include/            # Header files
├── lib/                # External libraries
├── src/                # Main firmware
├── test/               # Unit tests
├── Calculations.xlsx   # Input scaling & calibration math
├── Comand Codes.txt    # Command mapping reference
├── Train Controller.pdf# Hardware documentation
└── platformio.ini      # PlatformIO configuration
```

---

## ⚙️ Installation & Setup

### 1. Clone the Repository
```bash
git clone https://github.com/DMatern/TrainSim-Controller.git
cd TrainSim-Controller
```

### 2. Open with PlatformIO  
Use VS Code + PlatformIO extension.

### 3. Build & Upload  
Connect your ATmega32u4‑based board and click **Upload**.

---

## 🎮 How It Works

The firmware reads analog and digital inputs and sends them to the PC as a **USB joystick**. Train simulators can then bind these inputs to in‑game controls.

### Input Types Supported
- **Analog axes** (0–1023 ADC resolution)
- **Digital buttons**
- **Toggle switches**
- **Momentary switches**

### Command Mapping  
The file `Comand Codes.txt` contains the full list of supported commands and their internal codes.

---

## 📐 Calibration

The included `Calculations.xlsx` file helps determine:
- Input scaling  
- Dead zones  
- Lever travel mapping  
- Output ranges for HID reports  

You can adjust these values in the firmware to match your physical hardware.

---

## 📸 Documentation

The file **Train Controller.pdf** includes:
- Wiring diagrams  
- Mechanical layout  
- Lever geometry  
- Switch placement  

This makes it easy to build or modify your own controller.

---

## 🚧 Future Enhancements

- Add OLED display for mode/status  
- Add force‑feedback or detent simulation  
- Add support for rotary encoders  
- Add configuration menu via USB serial  
- Add EEPROM‑stored calibration profiles  

---

## 📜 License

Add your preferred license here (MIT, GPL, etc.).

---
