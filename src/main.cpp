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

// ==== Helper Functions ====
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

// ==== Weather ====
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
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, payload) == DeserializationError::Ok)
    {
      float t = doc["main"]["temp"].as<float>();
      int h = doc["main"]["humidity"].as<int>();
      String desc = doc["weather"][0]["description"].as<String>();
      desc[0] = toupper(desc[0]);
      weatherTemp = String(t, 1) + (char)223 + "C " + String(h) + "%";
      weatherDesc = desc.substring(0, 16);
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

// ==== Alarm ====
void checkAlarm()
{
  DateTime now = rtc.now();
  if (alarmOn && now.hour() == alarmHour && now.minute() == alarmMin)
  {
    digitalWrite(BUZZER, HIGH);
  }
  else
    digitalWrite(BUZZER, LOW);
}

// ==== Temperature Detector ====
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
    digitalWrite(BUZZER, HIGH);
    delay(1000000);
  }

  if (isAlarming && millis() - alarmStart >= TEMP_ALARM_DURATION)
  {
    isAlarming = false;
    tempAlert = false;
    digitalWrite(BUZZER, LOW);
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
  case 0:
  { // Time & Date
    DateTime now = rtc.now();
    lcd.print(now.year());
    lcd.setCursor(0, 0);
    if (now.year() < 2000)
    {
      lcd.setCursor(0, 0);
      lcd.print("RTC not set ");
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

  case 1:
  { // Temp & Humidity
    float t, h;
    readDHT(t, h);
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(t, 1);
    lcd.print((char)223);
    lcd.print("C   ");
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(h, 1);
    lcd.print("%   ");

    if (tempAlert)
    {
      lcd.setCursor(0, 1);
      lcd.print("TEMP ALERT!     ");
    }
    break;
  }

  case 2:
  { // Alarm
    lcd.setCursor(0, 0);
    lcd.print("Alarm ");
    lcd.print(alarmHour < 10 ? "0" : "");
    lcd.print(alarmHour);
    lcd.print(":");
    lcd.print(alarmMin < 10 ? "0" : "");
    lcd.print(alarmMin);

    lcd.setCursor(0, 1);
    lcd.print(alarmOn ? "ON" : "OFF");
    break;
  }

  case 3:
  { // Countdown
    lcd.setCursor(0, 0);
    lcd.print("Countdown:");

    if (countdownActive)
    {
      long remain = (countdownTime - millis()) / 1000;
      if (remain <= 0)
      {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(500);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(500);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(500);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);

        countdownActive = false;
        remain = 0;
      }

      lcd.setCursor(0, 1);
      char buf[8];
      snprintf(buf, sizeof(buf), "%02ld:%02ld", remain / 60, remain % 60);
      lcd.print(buf);
    }
    else
    {
      lcd.setCursor(0, 1);
      lcd.print("OK: start");
    }
    break;
  }

  case 4:
  { // Weather
    lcd.setCursor(0, 0);
    lcd.print("HCM: ");
    lcd.print(weatherTemp);
    lcd.setCursor(0, 1);
    lcd.print(weatherDesc);
    break;
  }
  }
}

// ==== Button Handler ====
void handleMenu()
{
  if (btnPressed(BTN_LEFT))
    menuIndex = (menuIndex - 1 + TOTAL_MENU) % TOTAL_MENU;
  if (btnPressed(BTN_RIGHT))
    menuIndex = (menuIndex + 1) % TOTAL_MENU;
  if (btnPressed(BTN_OK))
  {
    digitalWrite(BUZZER, LOW);
    buzzerState = false;
    tempAlert = false;
    alarmOn = false;
    if (menuIndex == 3 && !countdownActive)
    {
      countdownTime = millis() + 60000;
      countdownActive = true;
    }
    if (menuIndex == 4)
      getWeather();
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

  // Setup API routes
  setupApiRoutes(server);
  server.begin();
}

// ==== Loop ====
void loop()
{
  handleMenu();
  checkAlarm();
  checkTemperature(); // Temperature alarm check

  if (millis() - lastWeatherUpdate > WEATHER_INTERVAL)
  {
    getWeather();
    lastWeatherUpdate = millis();
  }

  updateLCD();
  digitalWrite(BUZZER, buzzerState ? HIGH : LOW);

  if (countdownActive && millis() >= countdownTime)
  {
    countdownActive = false;
    digitalWrite(BUZZER, HIGH);
    delay(1000);
    digitalWrite(BUZZER, LOW);
  }

  delay(200);
}
