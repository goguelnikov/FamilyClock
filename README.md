# 🕐 Family Clock — ESP32 + HUB75 LED Matrix

A connected family clock displayed on a 64×32 RGB LED matrix panel, powered by an ESP32. Fully configurable through a web interface accessible from any browser on the local network.

![ESP32](https://img.shields.io/badge/ESP32-Arduino-blue) ![PlatformIO](https://img.shields.io/badge/PlatformIO-6.x-orange) ![License](https://img.shields.io/badge/license-MIT-green)

---

## ✨ Features

### Display Screens (auto-rotation)

| Screen | Description |
|--------|-------------|
| ⏰ **Clock** | Large HH:MM display, date, day name, current temperature |
| 🌤️ **Weather** | Conditions, temperature, wind, pressure (OpenWeatherMap API) |
| 📅 **Next Event** | Upcoming calendar event with countdown |
| 📋 **Schedule** | Up to 4 upcoming events of the day |
| 🌙 **Moon** | Current lunar phase with graphical representation |
| 🗒️ **Notes** | 4 lines of free text |
| 🎂 **Birthdays** | Next 4 upcoming birthdays with date |

### Web Interface
- Full configuration from any browser (`http://horloge.local` or direct IP)
- 7 independent tabs: WiFi, Time, Weather, Display, Notes, Calendar, Birthdays
- Real-time brightness adjustment
- 9 color themes including a pure black (LEDs off) Midnight theme
- Per-screen enable/disable toggle
- Device name and mDNS hostname configurable (run multiple clocks on the same network)

### Calendar System
- Up to 20 activities with time, day-of-week bitmask, icon and label
- Configurable departure alert per activity
- Visual states: FUTURE → ALERT (orange) → IMMINENT (red flashing) → PAST (hidden)

---

## 🔧 Hardware

| Component | Details |
|-----------|---------|
| ESP32 | 38-pin devkit (e.g. ESP32-DEVKIT-V1) |
| HUB75 panel | 64×32 RGB LEDs (ICN2038S or compatible driver) |
| Power supply | 5V 2–4A depending on brightness |

> ⚠️ Some HUB75 panels have swapped G↔B channels. If colors look wrong, colors are stored as (R, B, G) in `Config.h` to compensate — adjust if needed for your panel.

---

## 🚀 Getting Started

### Prerequisites
- [PlatformIO](https://platformio.org/) (VSCode extension recommended)
- OpenWeatherMap free API key → [openweathermap.org](https://openweathermap.org/api)

### First Flash

1. **Clone the repository**
   ```bash
   git clone https://github.com/YOUR_USERNAME/horloge-familiale
   cd horloge-familiale
   ```

2. **Flash the firmware**
   ```
   PlatformIO → Upload
   ```

3. **Flash the web interface** (separate step — required!)
   ```
   PlatformIO → Upload Filesystem Image
   ```

4. **Connect to WiFi**  
   On first boot, the clock opens a captive portal:
   - Connect to hotspot **`Horloge-Config`** (password: `horloge123`)
   - Enter your WiFi credentials
   - The clock reboots and shows its IP address for 2 seconds

5. **Open the web interface**
   ```
   http://horloge.local
   ```
   or use the IP shown on the display.

---

## ⚙️ Configuration

### Tab overview

| Tab | Settings |
|-----|----------|
| **WiFi** | Device name, mDNS hostname, WiFi password |
| **Time** | NTP server, UTC offset, DST offset |
| **Weather** | OpenWeatherMap API key, city, units, refresh rate |
| **Display** | Brightness (real-time), rotation speed, theme, screen toggles |
| **Notes** | 4 lines of free text shown on the Notes screen |
| **Calendar** | Activities with time, days, icon and departure alert |
| **Birthdays** | Name, day and month for each birthday |

### Multiple clocks on the same network
Each clock can have a unique name and mDNS hostname set from the WiFi tab:
- Clock 1: `horloge-salon.local`
- Clock 2: `horloge-chambre.local`

---

## 🎨 Color Themes

| Theme | Description |
|-------|-------------|
| Nuit bleue | Dark blue background, white titles, blue text |
| Forêt | Dark green background, green/brown tones |
| Coucher | Warm orange/yellow sunset tones |
| Lavande | Purple/violet tones |
| Glace | Ice blue/white cold tones |
| Monochrome | Pure white, grey tones |
| Vert LED | Classic green LED terminal |
| Rouge LED | Red LED display |
| Minuit | **Pure black background** (LEDs off), white text, accent color |

---

## 📁 Project Structure

```
horloge-familiale/
├── platformio.ini            # PlatformIO build config
├── partitions_custom.csv     # Flash partition table
├── data/
│   └── index.html            # Web interface (uploaded to SPIFFS)
└── src/
    ├── main.cpp              # Boot, WiFi, main loop
    ├── Config.h              # Persistent config (NVS), themes, structs
    ├── DisplayHelper.h       # TTFont helpers, background, color utils
    ├── ScreenManager.h       # Screen rotation manager
    ├── WebConfig.h           # REST API server (ESPAsyncWebServer)
    ├── bigClockFont.h        # Large proportional font (HH:MM)
    ├── smallClockFont.h      # Small proportional font (labels)
    ├── services/
    │   ├── NTPService.h      # Non-blocking NTP time cache
    │   ├── WeatherService.h  # OpenWeatherMap (FreeRTOS task)
    │   └── CalendarService.h # Activity states and scheduling
    └── screens/
        ├── ScreenBoot.h
        ├── ScreenClock.h
        ├── ScreenWeather.h
        ├── ScreenNext.h
        ├── ScreenAgenda.h
        ├── ScreenMoon.h
        ├── ScreenNotes.h
        └── ScreenBirthdays.h
```

---

## 🌐 REST API

All endpoints used by the web interface:

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/config/wifi` | WiFi & identity config |
| GET | `/api/config/time` | NTP config |
| GET | `/api/config/weather` | Weather config |
| GET | `/api/config/display` | Display config + theme list |
| GET | `/api/config/notes` | Notes content |
| GET | `/api/config/calendar` | Activities list |
| GET | `/api/config/birthdays` | Birthdays list |
| POST | `/api/config/*` | Save corresponding section |
| GET | `/api/status` | Runtime status (IP, uptime, NTP, weather) |
| POST | `/api/brightness` | Set brightness in real-time |
| POST | `/api/weather/refresh` | Force weather refresh |
| POST | `/api/wifi/reset` | Reset WiFi credentials |

---

## 📦 Dependencies

```ini
adafruit/Adafruit GFX Library @ ^1.11.9
adafruit/Adafruit BusIO @ ^1.16.1
mrfaptastic/ESP32 HUB75 LED MATRIX PANEL DMA Display @ ^3.0.10
mathieucarbou/ESPAsyncWebServer @ ^3.3.10
mathieucarbou/AsyncTCP @ ^3.3.2
bblanchon/ArduinoJson @ ^7.2.0
arduino-libraries/NTPClient @ ^3.2.1
tzapu/WiFiManager @ ^2.0.17
```

---

## 🔑 Known Issues / Notes

- **SPIFFS must be flashed separately** from the firmware — use *Upload Filesystem Image* in PlatformIO after every `index.html` change.
- **HUB75 color channels** — if your panel shows wrong colors, check the G↔B swap in `Config.h` themes.
- **mDNS** (`horloge.local`) may not work on Android. Use the IP address shown at boot instead.
- `mxconfig.clkphase = false` is set to avoid display artifacts on some panels — revert if needed.

---

## 📄 License

MIT License — free to use, modify and distribute.
