# 🖥️ Tinytosh

![Tinytosh Showcase](img/showcase_1.jpg)

> **The open-source, retro-styled desktop companion.** > Part smart display, part hardware monitor, 100% hackable.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--C3-green.svg)
![App](https://img.shields.io/badge/desktop_bridge-Rust_%7C_Tauri-orange.svg)

---

## 🚀 Get Started (The Easy Way)

**You do not need to compile code to use Tinytosh.** This repository hosts a **Web Installer** and a complete **Interactive Setup Guide**. You can flash your device directly from your browser in under 2 minutes.

[👉 **Launch Setup Guide & Web Flasher**](https://vladimirgitsarev.github.io/Tinytosh/)  
*(Click above to view the assembly guide, wiring diagrams, and configure your device)*

---

## 🧐 What is this?

**Tinytosh** is a DIY project that fits a smart dashboard inside a tiny, 3D-printed Macintosh-style case. It connects to your WiFi to display useful information or hooks up to your PC via USB or Wi-Fi to show real-time hardware stats.

### Available Screens & Services
* 🕒 **Internet Clock:** Auto-syncs time and date based on your location.
* 🌤️ **Weather Station:** Live Temperature, Humidity, and Forecasts (via Open-Meteo).
* 🍃 **Air Quality:** Monitor local AQI levels (US & EU Standards).
* 📊 **Stock Tracker:** Track market data for ~100 top global assets, ETFs, and Mega-Cap Tech with daily trend indicators.
* 📈 **Crypto Tracker:** Watch your favorite coin (from top 75 global cryptos) with price and trend indicators.
* 💱 **Currency Tracker:** Track exchange rates for over 150 fiat currency pairs with custom scaling multipliers.
* 🖥️ **PC Hardware Monitor:** Connects via **USB** or **Wirelessly** to your Windows/Mac/Linux computer to show CPU Load, RAM Usage, and Network Speeds in real-time!
* 🎧 **PC Media:** Displays currently playing track, artist, album, and playback status streamed directly from your connected computer.

### ✨ Key Features
* **Modular Dashboard:** Enable/Disable screens on the fly via a Web Panel or PC App. 
* **Drag & Drop Reordering:** Fully customize your display sequence. Grab and drag screens to change their order. The configuration UI dynamically rearranges itself to match your custom layout perfectly.
* **Touch Button Controls:** Supports an optional TTP223 touch sensor. Tap to instantly skip screens (or wake the display), and **Long Press** to lock/unlock auto-rotation to keep your favorite screen visible indefinitely.
* **Smart Auto-Hide:** PC Monitor and PC Media screens can intelligently hide themselves and skip rotation when your PC is off, disconnected, or no media is playing.
* **Night Mode & Power Saving:** Set a quiet schedule to minimize sleep distractions. Choose between *Dim Display* or *Turn Display Off*. Features "Smart Latching" (waits for the primary screen before sleeping), 10x slower background API fetching to save power, and a temporary 30-second wake feature via the physical button.
* **Zero Config APIs:** Uses free public APIs for Stocks, Crypto, Currency, Weather, and Air Quality. No API keys required.
* **Privacy First:** No accounts, no cloud tracking. Everything runs locally on the ESP32.

![Interface Demo](img/web_panel_demo.gif)

---

## 🛠️ The Software Stack

For developers, makers, and the curious, here is how the magic happens. The project consists of two distinct software parts:

### 1. Firmware (ESP32-C3)
*Written in C++ using the Arduino Framework.*

The firmware is designed to be **non-blocking** and **modular**.
* **Universal Config Sync:** The device hosts its own Web Server, but also accepts and instantly applies full configuration payloads over the PC Serial/USB connection.
* **mDNS Support:** Easily access the device's Web Panel without memorizing IPs using its unique local domain (e.g., `http://tinytosh-ab12.local`).
* **Hardware Pairing Lock:** Telemetry streams are protected. Tinytosh securely pairs to the active PC to ensure multiple computers on the same network don't fight over the display.
* **Dynamic Rendering:** The `DisplayService` handles the OLED. It supports "partial screen buffering," allowing for complex transition effects (like dissolving pixels or sliding curtains) without needing a massive frame buffer.
* **Smart Wifi Manager:** Uses a Captive Portal for initial setup. If WiFi drops, it auto-reconnects without freezing the UI.
* **Preference Storage:** Configuration is saved to the ESP32's Non-Volatile Storage (NVS) using a custom bitmask system for efficiency.

#### 🏗️ Build & Compile Guide

**Web Installer: No Coding Required**

This is the fastest way to get started. You do not need to install VS Code, Arduino, or any drivers.
1.  Connect your ESP32-C3 to your computer via USB.
2.  Open the **[Tinytosh Web Installer](https://vladimirgitsarev.github.io/Tinytosh/)** in a Chromium-based browser (Chrome, Edge, Opera, Brave).
3.  Click **"Connect"** and select your device from the list.
4.  Click **"Install Tinytosh"** to flash the latest firmware automatically.

You can build this project using **PlatformIO** (VS Code) or the **Arduino IDE**.

**Option A: PlatformIO** This is the "Gold Standard" as it manages dependencies automatically. Simply open the project in VS Code and copy the following into your `platformio.ini`:

```ini
[env:esp32-c3-supermini]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
build_flags = 
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
lib_deps =
    https://github.com/tzapu/WiFiManager.git
    bblanchon/ArduinoJson @ ^6.21.0
    adafruit/Adafruit SSD1306 @ ^2.5.7
    adafruit/Adafruit GFX Library @ ^1.11.5
    adafruit/Adafruit BusIO @ ^1.14.1
    mathertel/OneButton @ ^2.5.0
```

**Option B: Arduino IDE** If you prefer the Arduino IDE, you must install the external libraries manually via the Library Manager (`Sketch` -> `Include Library` -> `Manage Libraries...`).:

| Library Name | Author | Purpose |
| :--- | :--- | :--- |
| **WiFiManager** | *tzapu* | Captive portal for WiFi setup |
| **ArduinoJson** | *Benoit Blanchon* | Parsing API data and settings |
| **Adafruit SSD1306** | *Adafruit* | Driver for the OLED screen |
| **Adafruit GFX Library** | *Adafruit* | Core graphics and text support |
| **OneButton** | *Matthias Hertel* | Touch button handling and long presses |

> ⚠️ **Important:** When installing `Adafruit SSD1306`, the IDE may ask if you want to install dependencies like **"Adafruit BusIO"**. Click **"Install All"** to ensure the screen works correctly.

**Note on Built-in Libraries:** The following libraries are required but **do not** need to be installed separately. They are included in the ESP32 Board Package:
* `WiFi.h` & `WiFiServer.h`
* `HTTPClient.h`
* `Preferences.h`
* `Wire.h` (I2C)
* `time.h`
* `ESPmDNS.h`

### 2. PC Bridge App (Desktop)
*Written in Rust 🦀 & Tauri.*

To display PC statistics (CPU/RAM/Net) and manage device settings, the ESP32 uses a lightweight helper app running on the computer.
* **Cross-Platform:** Runs on Windows, macOS, and Linux from a single codebase.
* **Dynamic UI Rendering:** The PC app dashboard physically mirrors your device! Configuration panels automatically reorder themselves in real-time to match the exact screen sequence you set on your Tinytosh.
* **Wireless Telemetry (mDNS):** The PC app automatically discovers Tinytosh devices on your local network. You can broadcast your PC's hardware stats completely wirelessly!
* **Smart Connection Fallback:** The app constantly monitors your hardware and instantly prioritizes a wired USB connection for maximum stability. Yank the USB cable? The app instantly and silently falls back to Wi-Fi to keep the data flowing with zero hesitation.
* **Native Telemetry:** Fetches system stats directly from the OS kernel—no third-party bloatware (like AIDA64) required.

**Build it yourself:**
```bash
cd TinytoshPC
npm install
npm run tauri build
```

---

## 🖨️ Hardware & 3D Files

The case is designed to be **screwless**—everything snaps together. 

* **Microcontroller:** ESP32-C3 SuperMini
* **Display:** 0.96" OLED (I2C)
* **Optional:** TTP223 Touch Sensor (for manual screen switching)

You can download the STL/3MF files and view the full bill of materials on MakerWorld:

[**📥 Download 3D Models on MakerWorld**](https://makerworld.com/en/models/2270326-tinytosh-mini-retro-pc-smart-wifi-display-esp32#profileId-2474693)

---

## 🤝 Contributing

Got a cool idea? Did you design a better case? Wrote a module to track your YouTube subs?

**We love pull requests!**
1.  Fork the repo.
2.  Create your feature branch (`git checkout -b feature/AmazingFeature`).
3.  Commit your changes.
4.  Open a Pull Request.

If you encounter bugs or have feature suggestions, please [Open an Issue](https://github.com/VladimirGitsarev/Tinytosh/issues).

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

* Weather and AQI data provided by [Open-Meteo](https://open-meteo.com/).
* Crypto data provided by [CoinLore](https://www.coinlore.com/cryptocurrency-data-api).
* Stock data provided by [Stooq](https://stooq.com/).
* IP Geolocation by [ip-api](https://ip-api.com/).
* Fiat Currency data provided by [fawazahmed0/currency-api](https://github.com/fawazahmed0/exchange-api).

---

## 📅 Changelog

| Version | Date | Key Changes |
| :--- | :--- | :--- |
| **v1.0.5** | *Apr 2026* | 🎧 Added **PC Media** screen (Track, Artist, Album, Status). 👆 Added **Touch Button Controls** (Long press to lock/unlock auto-rotation). 👻 Added **Auto-hide** toggles to completely skip empty PC Monitor and Media screens. 🌐 Added local mDNS domain access (e.g., `tinytosh-XXXX.local`). |
| **v1.0.4** | *Mar 2026* | 🖥️ **PC App Upgrade:** Added **Wireless Telemetry via Wi-Fi (mDNS)**, **Dynamic UI Rendering** that mirrors Web Panel functionality (update device settings and monitor current API data), and **Smart Connection Fallback** (instant USB-to-WiFi switching). <br>⚙️ **Firmware:** Added Universal Config Sync (saving settings via PC app), Smart IP Reporting via Serial, and Hardware Pairing Locks. |
| **v1.0.3** | *Mar 2026* | 🌙 Added **Night Mode** with smart latching, screen dimming/off scheduling, and 10x background API power saving. 🔄 Introduced **Drag & Drop Reordering** for dynamic screen sequencing directly in the Web Panel. |
| **v1.0.2** | *Mar 2026* | 📈 Added **Stock Tracker** module (~100 global assets, ETFs, Mega-Cap Tech, ADRs). Added customizable "Full Name" layout toggles for Crypto, Currency, and Stock screens. Fixed an animation bug for single-screen setups. |
| **v1.0.1** | *Feb 2026* | 💱 Added **Currency Tracker** module (150+ fiat pairs), introduced dynamic multipliers for large conversion gaps, and optimized logging. Expanded Crypto list to top 75. |
| **v1.0.0** | *Initial* | 🚀 Initial release: Time, Weather, AQI, Crypto, and PC Monitor modules. Web Panel, firmware flasher, and 3D printable case released. |

<p align="center">
  <sub>Built with ❤️ and too much coffee.</sub>
</p>