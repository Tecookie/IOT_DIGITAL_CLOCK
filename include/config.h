#pragma once
#include <Arduino.h>

// ===== Pins =====
#define LED 12
#define BUZZER 25
#define BTN_OK 26
#define BTN_LEFT 33
#define BTN_RIGHT 32    
#define DHTPIN 27
#define DHTTYPE DHT22
#define LIGHT_SENSOR 34

// ===== WiFi & Weather =====
const char* WIFI_SSID = "Tec";
const char* WIFI_PASS = "12345678";
const String WEATHER_API_KEY = "76184842ab4678df905a4ce6fa399db2";
const String CITY = "Ho%20Chi%20Minh";
const unsigned long WEATHER_INTERVAL = 10 * 60 * 1000; // 10 minutes

// ===== Menu =====
const int TOTAL_MENU = 5;

// ===== Alarm =====
int alarmHour = 7;
int alarmMin = 0;
bool alarmOn = false;

// ===== Countdown =====
unsigned long countdownTime = 0;
bool countdownActive = false;

// ===== Temp ======
const float TEMP_HIGH = 33.0;   // Too hot (°C)
const float TEMP_LOW  = 15.0;   // Too cold (°C)
const unsigned long TEMP_ALARM_DURATION = 5000; // 5 seconds