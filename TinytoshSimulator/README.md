# 🛰️ Tinytosh Simulator

![Tinytosh Simulator](../img/simulator.png)

This is a [Wokwi](https://wokwi.com/) simulation of **[Tinytosh](https://github.com/VladimirGitsarev/Tinytosh)** — a way to try the device's screens, navigation, and button behavior without soldering a single wire, printing a case, or assembling any hardware. One click and you're looking at a live, ticking Tinytosh in your browser.

## 🧐 Why this exists

Not everyone wants to commit to buying parts, printing a case, and wiring up an OLED before knowing if Tinytosh is for them. This simulator lets you poke around the real screen layouts, cycle through screens with the button, and get a feel for the device first — no hardware required.

It's a simplified, standalone build of the display logic: no WiFi, no API calls, no settings web panel, no persistence. All the weather, air quality, stocks, crypto, flights, and everything else you see is realistic mock data hardcoded into `structs.h`, driven by the same `Config` struct and the same screen-drawing code as the real firmware. Every screen, layout option, and button gesture (single tap, double tap, long press) behaves exactly as it does on real hardware — it just isn't fetching anything live.

## 🚀 Try it now (no install required)

👉 **[Open the Tinytosh Simulator on Wokwi](https://wokwi.com/projects/452257384760576001)**

Just follow the link and hit the green ▶️ Play button — that's it.

## 💻 Run it locally

Prefer to tinker in your own editor? You can run this same simulation locally in VS Code:

1. Install [VS Code](https://code.visualstudio.com/), the [Wokwi for VS Code](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode) extension, and the [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) extension.
2. Open this folder (`TinytoshSimulator/`) in VS Code.
3. Build the firmware:
   ```bash
   pio run
   ```
4. Open the Command Palette and run **`Wokwi: Start Simulator`**.

The first build downloads the ESP32 toolchain and libraries, so it may take a few minutes — after that, `pio run` only takes a few seconds. Whenever you edit `sketch.ino`, `structs.h`, or `images.h`, just re-run `pio run` and restart the simulator to see your changes.

## 📁 What's in here

| File | Purpose |
|---|---|
| `sketch.ino` | The simulation's screen-drawing, navigation, and button-handling logic. |
| `structs.h` | The shared `Config`/data structs, plus all the mock data defaults. |
| `images.h` | Icon bitmaps used across the screens. |
| `diagram.json` | The simulated hardware wiring (ESP32-C3, SSD1306 OLED, pushbutton). |
| `platformio.ini` | Local build configuration (board, framework, libraries). |
| `wokwi.toml` | Tells the Wokwi extension where to find the compiled firmware. |
