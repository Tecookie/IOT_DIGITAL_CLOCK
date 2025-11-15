#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "api_route.h"

// ==== Hardware Objects ====
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 rtc;
DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);

// ==== State ====
bool buzzerState = false;
String weatherTemp = "--";
String weatherDesc = "--";
unsigned long lastWeatherUpdate = 0;
int menuIndex = 0;

// ==== Sensor State ====
bool tempAlert = false;
float currentTemp = 0.0;
float currentHum = 0.0;

// ==== Alarm edit state ====
bool alarmEditMode = false;
bool editingAlarmHours = false;
int editAlarmHour = 0;
int editAlarmMin = 0;

bool alarmRinging = false;            // true while alarm is sounding
unsigned long alarmRingStart = 0;
unsigned long lastBuzzerToggle = 0;
unsigned long buzzerToggleInterval = 500; // not used for continuous mode but kept

// ==== Countdown edit state ====
bool countdownEditMode = false;
bool editingCountdownSeconds = false; // false => editing minutes, true => editing seconds
int editCountdownMin = 0;
int editCountdownSec = 0;

bool countdownRinging = false;        // continuous long beep state for finished countdown

// ==== Buttons debounce helper ====
bool btnPressed(int pin)
{
  static unsigned long lastBtn = 0;
  if (digitalRead(pin) == LOW && millis() - lastBtn > 200)
  {
    lastBtn = millis();
    return true;
  }
  return false;
}

// ==== DHT read helper ====
void readDHT(float &t, float &h)
{
  t = dht.readTemperature();
  h = dht.readHumidity();
  if (isnan(t) || isnan(h))
  {
    t = -1;
    h = -1;
  }
  currentTemp = t;
  currentHum = h;
}

// ==== WiFi connect ====
void connectWiFi()
{
  lcd.clear();
  lcd.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40)
  {
    delay(500);
    lcd.print(".");
    attempts++;
  }
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED)
  {
    lcd.print(WiFi.localIP());
  }
  else
  {
    lcd.print("WiFi FAIL!");
  }
  delay(1500);
  lcd.clear();
}

// ==== Weather fetch ====
void getWeather()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    weatherTemp = "--";
    weatherDesc = "No WiFi";
    return;
  }

  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + CITY +
               "&appid=" + WEATHER_API_KEY + "&units=metric&lang=en";

  http.begin(url);
  int code = http.GET();

  if (code == HTTP_CODE_OK)
  {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, payload);
    if (!err)
    {
      float t = doc["main"]["temp"].as<float>();
      int h = doc["main"]["humidity"].as<int>();
      String desc = doc["weather"][0]["description"].as<String>();
      if (desc.length() > 0) desc[0] = toupper(desc[0]);
      weatherTemp = String(t, 1) + (char)223 + "C " + String(h) + "%";
      weatherDesc = desc.substring(0, min((int)desc.length(), 16));
    }
    else
    {
      weatherTemp = "JSON Err";
      weatherDesc = "Parse failed";
    }
  }
  else
  {
    weatherTemp = "--";
    weatherDesc = "HTTP " + String(code);
  }

  http.end();
}

// ==== Alarm ringing control ====
void startAlarmRinging()
{
  alarmRinging = true;
  alarmRingStart = millis();
  // For alarm we will toggle to create pattern; keep buzzerState = true initially
  buzzerState = true;
  digitalWrite(BUZZER, HIGH);
}

void stopAlarmRinging(bool disableAlarm = false)
{
  alarmRinging = false;
  buzzerState = false;
  digitalWrite(BUZZER, LOW);
  if (disableAlarm)
    alarmOn = false;
}

// Check the RTC alarm condition (non-blocking)
void checkAlarm()
{
  DateTime now = rtc.now();
  // trigger only when seconds == 0 (start of minute), and alarm is enabled
  if (alarmOn && now.hour() == alarmHour && now.minute() == alarmMin && now.second() == 0)
  {
    if (!alarmRinging)
      startAlarmRinging();
  }

  // Non-blocking toggle pattern while ringing (short pattern)
  if (alarmRinging)
  {
    unsigned long nowt = millis();
    if (nowt - lastBuzzerToggle >= buzzerToggleInterval)
    {
      lastBuzzerToggle = nowt;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER, buzzerState ? HIGH : LOW);
    }
  }
}

// ==== Countdown control ====
void startCountdownRinging()
{
  countdownRinging = true;
  buzzerState = true; // continuous long beep
  digitalWrite(BUZZER, HIGH);
}

void stopCountdownRinging()
{
  countdownRinging = false;
  buzzerState = false;
  digitalWrite(BUZZER, LOW);
}

void checkCountdown()
{
  if (countdownActive && millis() >= countdownTime)
  {
    countdownActive = false;
    // When finished, turn buzzer continuous ON (as requested). LCD will show 00:00.
    startCountdownRinging();
  }

  // countdownRinging is continuous high; nothing to toggle here (stays ON until OK)
}

// ==== Temperature Detector (non-blocking, no long delays) ====
void checkTemperature()
{
  float t = dht.readTemperature();
  if (isnan(t))
    return;
  currentTemp = t;

  static bool isAlarming = false;
  static unsigned long alarmStart = 0;

  if (!isAlarming && (t >= TEMP_HIGH || t <= TEMP_LOW))
  {
    isAlarming = true;
    alarmStart = millis();
    tempAlert = true;
    Serial.println("⚠️ TEMP ALERT!");
    // Make buzzer on while temp alert for a short duration
    buzzerState = true;
    digitalWrite(BUZZER, HIGH);
  }

  if (isAlarming && millis() - alarmStart >= TEMP_ALARM_DURATION)
  {
    isAlarming = false;
    tempAlert = false;
    digitalWrite(BUZZER, LOW);
    buzzerState = false;
  }

  if (!isAlarming && (t < TEMP_HIGH && t > TEMP_LOW))
  {
    tempAlert = false;
  }
}

// ==== LCD Display ====
void updateLCD()
{
  lcd.clear();
  switch (menuIndex)
  {
  case 0: // Time & Date
  {
    DateTime now = rtc.now();
    if (now.year() < 2000)
    {
      lcd.setCursor(0, 0);
      lcd.print("RTC not set     ");
      lcd.setCursor(0, 1);
      lcd.print("Set time via PC ");
      break;
    }

    char timeBuf[9];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.setCursor(0, 0);
    lcd.print(timeBuf);

    char dateBuf[11];
    snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", now.day(), now.month(), now.year());
    lcd.setCursor(0, 1);
    lcd.print(dateBuf);
    break;
  }

  case 1: // Temp & Humidity
  {
    float t, h;
    readDHT(t, h);
    lcd.setCursor(0, 0);
    if (t < -100)
      lcd.print("Temp: N/A       ");
    else
    {
      lcd.print("Temp: ");
      lcd.print(t, 1);
      lcd.print((char)223);
      lcd.print("C   ");
    }

    lcd.setCursor(0, 1);
    if (h < -100)
      lcd.print("Hum: N/A        ");
    else
    {
      lcd.print("Hum: ");
      lcd.print(h, 1);
      lcd.print("%   ");
    }

    if (tempAlert)
    {
      lcd.setCursor(10, 1);
      lcd.print("ALERT");
    }
    break;
  }

  case 2: // Alarm
  {
    lcd.setCursor(0, 0);
    lcd.print("Alarm ");
    if (alarmEditMode)
    {
      char buf[8];
      snprintf(buf, sizeof(buf), "%02d:%02d", editAlarmHour, editAlarmMin);
      lcd.print(buf);
      lcd.setCursor(0, 1);
      if (editingAlarmHours)
        lcd.print("Edit Hr  OK=Save");
      else
        lcd.print("Edit Min OK->");
    }
    else
    {
      char buf[8];
      snprintf(buf, sizeof(buf), "%02d:%02d", alarmHour, alarmMin);
      lcd.print(buf);
      lcd.setCursor(0, 1);
      lcd.print(alarmOn ? "ON " : "OFF");
      lcd.print(" OK=Edit");
      if (alarmRinging)
      {
        lcd.setCursor(7, 1);
        lcd.print("RING");
      }
    }
    break;
  }

  case 3: // Countdown
  {
    lcd.setCursor(0, 0);
    lcd.print("Countdown:");

    if (countdownEditMode)
    {
      lcd.setCursor(0, 1);
      char buf[16];
      // show which field is being edited
      if (editingCountdownSeconds)
        snprintf(buf, sizeof(buf), "%02d:%02d Set S", editCountdownMin, editCountdownSec);
      else
        snprintf(buf, sizeof(buf), "%02d:%02d Set M", editCountdownMin, editCountdownSec);
      lcd.print(buf);
    }
    else if (countdownActive)
    {
      long remain = (long)((countdownTime - millis()) / 1000);
      if (remain < 0) remain = 0;
      lcd.setCursor(0, 1);
      char buf[8];
      snprintf(buf, sizeof(buf), "%02ld:%02ld   ", remain / 60, remain % 60);
      lcd.print(buf);
    }
    else if (countdownRinging)
    {
      // Option 1: show 00:00 when finished (continuous beep until OK)
      lcd.setCursor(0, 1);
      lcd.print("00:00           ");
    }
    else
    {
      lcd.setCursor(0, 1);
      lcd.print("OK=Start/Edit    ");
    }
    break;
  }

  case 4: // Weather
  {
    lcd.setCursor(0, 0);
    lcd.print("HCM: ");
    lcd.print(weatherTemp);
    lcd.setCursor(0, 1);
    lcd.print(weatherDesc);
    break;
  }

  default:
    lcd.setCursor(0, 0);
    lcd.print("Menu ");
    lcd.print(menuIndex);
    break;
  }
}

// ==== Button Handler (Alarm + Countdown edit flows) ====
void handleMenu()
{
  // LEFT button
  if (btnPressed(BTN_LEFT))
  {
    // Alarm edit mode: minutes ±10, hours ±1
    if (menuIndex == 2 && alarmEditMode)
    {
      if (editingAlarmHours)
        editAlarmHour = (editAlarmHour - 1 + 24) % 24;
      else
        editAlarmMin = (editAlarmMin - 10 + 60) % 60;
      return;
    }

    // Countdown edit mode: minutes ±1, seconds ±1
    if (menuIndex == 3 && countdownEditMode)
    {
      if (editingCountdownSeconds)
        editCountdownSec = max(0, editCountdownSec - 1);
      else
        editCountdownMin = max(0, editCountdownMin - 1);
      return;
    }

    // Normal navigation
    menuIndex = (menuIndex - 1 + TOTAL_MENU) % TOTAL_MENU;
  }

  // RIGHT button
  if (btnPressed(BTN_RIGHT))
  {
    // Alarm edit mode
    if (menuIndex == 2 && alarmEditMode)
    {
      if (editingAlarmHours)
        editAlarmHour = (editAlarmHour + 1) % 24;
      else
        editAlarmMin = (editAlarmMin + 10) % 60;
      return;
    }

    // Countdown edit mode
    if (menuIndex == 3 && countdownEditMode)
    {
      if (editingCountdownSeconds)
      {
        editCountdownSec = editCountdownSec + 1;
        if (editCountdownSec > 59) editCountdownSec = 59;
      }
      else
      {
        editCountdownMin = editCountdownMin + 1;
        if (editCountdownMin > 999) editCountdownMin = 999;
      }
      return;
    }

    // Normal navigation
    menuIndex = (menuIndex + 1) % TOTAL_MENU;
  }

  // OK button
  if (btnPressed(BTN_OK))
  {
    // If alarm or countdown is ringing -> stop on OK
    if (alarmRinging)
    {
      stopAlarmRinging(true); // stop and disable the alarm
      return;
    }
    if (countdownRinging)
    {
      stopCountdownRinging(); // stop continuous beep
      return;
    }

    // Clear any short buzzer flags and alerts
    digitalWrite(BUZZER, LOW);
    buzzerState = false;
    tempAlert = false;

    // ALARM edit flow
    if (menuIndex == 2)
    {
      // Enter edit mode
      if (!alarmEditMode)
      {
        alarmEditMode = true;
        editingAlarmHours = false;
        editAlarmHour = alarmHour;
        editAlarmMin = alarmMin;
        return;
      }

      // If editing minutes -> switch to hours
      if (!editingAlarmHours)
      {
        editingAlarmHours = true;
        return;
      }

      // If editing hours -> save & exit
      if (editingAlarmHours)
      {
        alarmHour = editAlarmHour;
        alarmMin = editAlarmMin;
        alarmOn = true; // enable after edit
        alarmEditMode = false;
        editingAlarmHours = false;
        return;
      }
    }

    // COUNTDOWN edit flow (Option A: Min -> Sec -> Save)
    if (menuIndex == 3)
    {
      // If countdown currently running -> pause it and populate edit values
      if (countdownActive)
      {
        countdownActive = false;
        long remain = (long)((countdownTime - millis()) / 1000);
        if (remain < 0) remain = 0;
        editCountdownMin = remain / 60;
        editCountdownSec = remain % 60;
        return;
      }

      // Enter edit mode (minutes)
      if (!countdownEditMode)
      {
        countdownEditMode = true;
        editingCountdownSeconds = false;
        // keep previous edit values if set, else default to 0:30
        if (editCountdownMin == 0 && editCountdownSec == 0)
        {
          editCountdownMin = 0;
          editCountdownSec = 30;
        }
        return;
      }

      // If editing minutes -> switch to seconds
      if (countdownEditMode && !editingCountdownSeconds)
      {
        editingCountdownSeconds = true;
        return;
      }

      // If editing seconds -> save & start countdown
      if (countdownEditMode && editingCountdownSeconds)
      {
        if (editCountdownSec < 0) editCountdownSec = 0;
        if (editCountdownSec > 59) editCountdownSec = 59;
        unsigned long totalMs = (unsigned long)editCountdownMin * 60000UL + (unsigned long)editCountdownSec * 1000UL;
        if (totalMs == 0)
        {
          // ignore zero-length countdown
          countdownEditMode = false;
          editingCountdownSeconds = false;
          return;
        }

        countdownTime = millis() + totalMs;
        countdownActive = true;
        countdownEditMode = false;
        editingCountdownSeconds = false;
        return;
      }
    }

    // WEATHER refresh
    if (menuIndex == 4)
    {
      getWeather();
      return;
    }
  }
}

// ==== Setup ====
void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  pinMode(BUZZER, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  dht.begin();

  if (!rtc.begin())
  {
    lcd.print("RTC Error");
    while (1)
      ;
  }

  // Auto set RTC if invalid
  DateTime now = rtc.now();
  if (now.year() < 2000)
  {
    Serial.println("RTC invalid, setting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  connectWiFi();
  getWeather();
  lastWeatherUpdate = millis();

  // Setup API routes (uses your api_route.h)
  setupApiRoutes(server);
  server.begin();

  // Ensure buzzer off at boot
  digitalWrite(BUZZER, LOW);
  buzzerState = false;
}

// ==== Loop ====
void loop()
{
  handleMenu();
  checkAlarm();
  checkCountdown();
  checkTemperature();

  // Weather periodic
  if (millis() - lastWeatherUpdate > WEATHER_INTERVAL)
  {
    getWeather();
    lastWeatherUpdate = millis();
  }

  updateLCD();

  // Keep buzzer pin consistent
  digitalWrite(BUZZER, buzzerState ? HIGH : LOW);

  delay(120); // short delay for stability and debounce
}
