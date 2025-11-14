import axios from "axios";

const API_URL = import.meta.env.VITE_ESP32_API;

// Axios instance
const api = axios.create({
  baseURL: API_URL,
  timeout: 3000,
  headers: { "Content-Type": "application/x-www-form-urlencoded" },
});

// ==========================
// ESP32 API FUNCTIONS
// ==========================

// Get full status
export const getStatus = async () => {
  const res = await api.get("/api/status");
  const data = res.data;

  return {
    ...data,
    alarmOn: data.alarmOn === "true" || data.alarmOn === true,
    countdownActive: data.countdownActive === "true" || data.countdownActive === true,
    buzzer: data.buzzer === "true" || data.buzzer === true,
    wifiConnected: data.wifiConnected === "true" || data.wifiConnected === true,
  };
};

// Update alarm
export const updateAlarm = (hour, min, on) =>
  api.post(
    "/api/alarm",
    new URLSearchParams({
      hour,
      min,
      on: on ? 1 : 0,
    })
  );

// Start countdown
export const startCountdown = (seconds) =>
  api.post("/api/countdown", new URLSearchParams({ seconds }));

// Change LCD display mode
export const changeLCDMode = (mode) =>
  api.post("/api/lcd", new URLSearchParams({ mode }));

// Toggle buzzer
export const toggleBuzzer = (buzzer) =>
  api.post("/api/buzzer", new URLSearchParams({ buzzer: buzzer ? 1 : 0 }));
