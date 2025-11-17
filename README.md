# ⭐ ESP32 IOT DIGITAL CLOCK

A modern ESP32-powered IoT clock featuring real-time monitoring, weather
updates, alarms, countdown timer, LED/Buzzer control, and a beautiful
React + Tailwind web dashboard.

------------------------------------------------------------------------

## 📦 Features

### 🕒 Core Functions

-   Real-time **clock & date** display (RTC DS1307)
-   Automatic **ambient light detection** (LCD backlight auto adjust)
-   **Weather info** from OpenWeather API (temperature, humidity,
    condition)
-   **DHT22 sensor** integration
-   **Alarm system** with buzzer + LED effects
-   **Countdown timer**
-   **LED & Buzzer remote control**
-   Dynamic **LCD screen switching** via web dashboard

### 🌐 Web Dashboard

Built using: - **React + Vite** - **TailwindCSS** - **AT-Design UI
components** - **Framer Motion animations** - Mobile responsive UI\
- Dark / Light Mode

------------------------------------------------------------------------

## ⚙️ Hardware Setup

### 📌 Components

  Component         Description
  ----------------- -------------------------------
  ESP32 Devkit V1   Main microcontroller
  DHT22             Temperature & Humidity Sensor
  LM393             Light Sensor
  DS1307            Real-time Clock
  I2C LCD 1602      Display
  Passive Buzzer    Alarm sound
  LED               Visual indicator
  Buttons           OK, LEFT, RIGHT

### 🪛 Pin Mapping

  Function       Pin
  -------------- --------------
  LED            GPIO 12
  Buzzer         GPIO 25
  Button OK      GPIO 26
  Button Left    GPIO 33
  Button Right   GPIO 32
  DHT22 Data     GPIO 27
  Light Sensor   GPIO 34
  RTC SDA/SCL    GPIO 21 / 22

------------------------------------------------------------------------

## 🖥️ Software Setup

### 1️⃣ Install Dependencies

``` bash
cd web
npm install
```

### 2️⃣ Add `.env` (This should appear on the first boot in LCD)

    VITE_ESP32_API=http://YOUR_ESP32_IP:3000

### 3️⃣ Start React App

``` bash
npm run dev
```

### 4️⃣ ESP32 Firmware

PlatformIO → Upload firmware\
Make sure to update inside `config.h`:

``` cpp
const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASS = "YOUR_PASSWORD";
```

------------------------------------------------------------------------

## 🌩 API Endpoints (ESP32)

### 🔹 GET `/api/status`

Returns JSON containing: - time, date - temperature, humidity -
weather - LED / buzzer state - alarm status - countdown status

### 🔹 POST `/api/alarm`

    hour=7&min=30&on=1

### 🔹 POST `/api/countdown`

    seconds=60

### 🔹 POST `/api/led`

    led=1
    buzzer=0

### 🔹 POST `/api/lcd`

    screen=weather

------------------------------------------------------------------------

## 🖼 Dashboard Screens

### **1. Connection Status**

Shows: - WiFi connection - ESP32 IP - Last heartbeat

### **2. Temperature & Humidity**

-   DHT22 readings
-   Weather icon
-   Animated sensor card

### **3. Weather**

-   OpenWeather information
-   Auto-updates every 10 min

### **4. Time**

-   Live RTC clock
-   Sync button (sync with browser time)

### **5. Alarm Settings**

-   Hour & minute dropdowns
-   On/Off toggle
-   Live buzzer test

### **6. LED & Buzzer Control**

-   Toggle LED
-   Toggle buzzer
-   Blinking animation preview

------------------------------------------------------------------------

## 📱 LCD Screen Switching

From React dashboard, you can choose what the physical LCD should
display:

  Screen        LCD Output
  ------------- ------------------
  Time          HH:MM:SS
  Weather       Temp + Condition
  Temperature   DHT22 readings
  Alarm         ON/OFF + time
  Connection    IP + WiFi
  LED/Buzzer    Current states

The React UI sends:

    POST /api/lcd
    screen=time

ESP32 updates LCD accordingly.

------------------------------------------------------------------------

## 🚀 Deployment

### 🟦 Deploy React App (Vite)

``` bash
npm run build
```

Upload the `/dist` folder to any free hosting: - Netlify\
- Vercel\
- GitHub Pages

### 🟧 ESP32 Hosting

ESP32 handles only the API --- the dashboard stays on your hosting.

------------------------------------------------------------------------

## 🔧 Troubleshooting

### ❌ ESP32 Shows `0.0.0.0`

Fix: - Ensure **2.4GHz WiFi** - Avoid WPA3-only routers\
- Try phone hotspot\
- Restart router sometimes required

### ❌ CORS Issues

Your backend `server.js` must include:

``` js
app.use(cors());
```

### ❌ "Refused to connect"

-   Firewall might block port `3000`
-   Hotspot sometimes blocks local ports

------------------------------------------------------------------------

## 📘 License

MIT License --- free to use and modify.

------------------------------------------------------------------------

## ❤️ Credits

Created by **Tecookie**\
