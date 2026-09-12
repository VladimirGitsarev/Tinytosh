# 🖥️ Tinytosh PC Bridge

This is the official desktop companion application for the **[Tinytosh](https://github.com/VladimirGitsarev/Tinytosh)** smart display project. Built with Rust and Tauri, it is incredibly lightweight, fast, and cross-platform (Windows, macOS, Linux).

## ✨ Key Features

* **📊 Native Hardware & Media Monitoring:** Reads system stats (CPU Load, RAM Usage, and Network Speeds) directly from the OS kernel, and streams active media playback (Track, Artist, Album, Status). No need to run heavy third-party software like AIDA64 or HWiNFO.
* **📡 Wireless Telemetry & Auto-Connect:** Automatically discovers Tinytosh devices on your local network via mDNS. Connects on startup to broadcast your PC stats completely wirelessly!
* **🔁 Smart Connection Fallback:** The app constantly monitors your hardware and instantly prioritizes a wired USB connection for maximum stability. If you pull the USB cable, it silently falls back to Wi-Fi to keep the data flowing without dropping a beat.
* **📜 Live USB Device Logs Terminal:** Streams real-time debug logs from the ESP32 when connected via USB, allowing you to easily monitor API fetches, network scanning, and system status right from your desktop.
* **🔐 Hardware Pairing Lock:** Securely pairs your Tinytosh to your active PC, ensuring multiple computers on the same network don't fight over the display.
* **👻 Smart Auto-Hide:** Configure your Tinytosh to automatically hide the PC Monitor, PC Media, or Bambu 3D Printer screens from its rotation when your computer is disconnected, nothing is playing, or your printer is offline.
* **🔄 Unified Configuration Sync:** Features instant, stutter-free 2-way sync between your PC and the device to ensure your settings are always up to date.
* **🎛️ Dynamic UI & Complete Control:** The dashboard physically mirrors your device! Configuration panels automatically reorder themselves in real-time, letting you manage every device setting directly from your desktop. Configure the procedural **OLED Theme Engine**, reassign **Hardware I2C/Touch Pins**, manage up to **5 rotating items** for financial trackers, set **custom per-screen sync intervals** for Weather/AQI/Stocks/Crypto/Currency, and tweak Calendar, Daylight, Moon, Population, Night Mode, animations, and more.
* **🖨️ Bambu 3D Printer Configuration:** Seamlessly input your Bambu Lab printer's Serial Number, Access Code, and optional IP address to instantly sync and enable live printing telemetry on your Tinytosh display.
* **🚀 System Tray & Autostart:** Minimizes quietly to the background and can be configured to launch automatically when you log in.

## 🛠️ Tech Stack

* **Backend:** Rust 🦀 (handles serial communication, system telemetry, media monitoring, mDNS network scanning, and HTTP requests)
* **Frontend:** Vanilla HTML, CSS, and JavaScript
* **Framework:** [Tauri](https://tauri.app/)

## 🚀 Development & Build Guide

### Prerequisites
Make sure you have installed [Node.js](https://nodejs.org/) and [Rust](https://www.rust-lang.org/tools/install).

### Setup
Navigate to this directory and install the frontend dependencies:
```bash
npm install
```

### Running in Development Mode
To run the app in development mode with hot-reloading:
```bash
npm run tauri dev
```

### Building for Release
To compile a standalone executable/installer for your current operating system:
```bash
npm run tauri build
```
The compiled installer will be located in `src-tauri/target/release/bundle/`.

## ⚙️ Recommended IDE Setup

- [VS Code](https://code.visualstudio.com/) + [Tauri](https://marketplace.visualstudio.com/items?itemName=tauri-apps.tauri-vscode) + [rust-analyzer](https://marketplace.visualstudio.com/items?itemName=rust-lang.rust-analyzer)