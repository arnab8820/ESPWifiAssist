# ESPWifiAssist

ESPWifiAssist is a lightweight helper library for ESP8266 that provides an automatic WiFi setup flow using a captive portal.

It allows your device to automatically connect to a saved WiFi network and fall back to Access Point mode when credentials are missing or invalid, letting users configure WiFi from a browser.

---

## Features

- Automatic STA ↔ AP switching
- Built-in captive portal
- WiFi credentials stored in flash (LittleFS)
- Network scanning API
- Callback-based event system
- Lightweight and easy to integrate

---

## Installation (PlatformIO)

Add this to your `platformio.ini`:

```ini
lib_deps =
    arnab8820/ESPWifiAssist
