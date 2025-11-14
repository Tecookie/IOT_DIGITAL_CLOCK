#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <RTClib.h>
#include <DHT.h>
#include <WiFi.h>
#include "config.h"

// ===== External references from main.cpp =====
extern RTC_DS1307 rtc;
extern DHT dht;
extern bool buzzerState;
extern unsigned long countdownTime;
extern bool countdownActive;
extern int alarmHour;
extern int alarmMin;
extern bool alarmOn;
extern String weatherTemp;
extern String weatherDesc;
extern int menuIndex;

// New globals for temperature status
extern float currentTemp;
extern float currentHum;
extern bool tempAlert;

// ===== Helper: Add CORS headers =====
void addCORSHeaders(AsyncWebServerRequest *request, AsyncWebServerResponse *response)
{
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

// ===== Setup API routes =====
void setupApiRoutes(AsyncWebServer &server)
{

    // ===== STATUS =====
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        DateTime now = rtc.now();
        String menuName;

        switch(menuIndex) {
    case 0: menuName = "Time"; break;
    case 1: menuName = "Temperature"; break;
    case 2: menuName = "Alarm"; break;
    case 3: menuName = "Countdown"; break;
    case 4: menuName = "Weather"; break;
}
        unsigned long remainingSeconds = 0;
        if (countdownActive && millis() < countdownTime) {
            remainingSeconds = (countdownTime - millis()) / 1000;
        }

        String json = "{";
        json += "\"menuIndex\":" + String(menuIndex) + ",";
        json += "\"menuName\":\"" + menuName + "\",";
        json += "\"time\":\"" + String(now.hour()) + ":" + (now.minute()<10?"0":"") + String(now.minute()) + ":" + (now.second()<10?"0":"") + String(now.second()) + "\",";
        json += "\"date\":\"" + String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "\",";
        json += "\"temperature\":" + String(currentTemp,1) + ",";
        json += "\"humidity\":" + String(currentHum,1) + ",";
        json += "\"tempAlert\":" + String(tempAlert ? "true" : "false") + ",";
        json += "\"tempHigh\":" + String(TEMP_HIGH,1) + ",";
        json += "\"tempLow\":" + String(TEMP_LOW,1) + ",";
        json += "\"alarmHour\":" + String(alarmHour) + ",";
        json += "\"alarmMin\":" + String(alarmMin) + ",";
        json += "\"alarmOn\":" + String(alarmOn ? "true" : "false") + ",";
        json += "\"countdownActive\":" + String(countdownActive ? "true" : "false") + ",";
        json += "\"countdownRemaining\":" + String(remainingSeconds) + ",";
        json += "\"buzzer\":" + String(buzzerState ? "true" : "false") + ",";
        json += "\"weatherTemp\":\"" + weatherTemp + "\",";
        json += "\"weatherDesc\":\"" + weatherDesc + "\",";
        json += "\"wifiConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
        json += "}";

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        addCORSHeaders(request, response);
        request->send(response); });

    // ===== LCD MODE =====
    server.on("/api/lcd", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("mode", true)) {
            String mode = request->getParam("mode", true)->value();
            mode.toLowerCase();
            if (mode == "time") menuIndex = 0;
            else if (mode == "temperature") menuIndex = 1;
            else if (mode == "alarm") menuIndex = 2;
            else if (mode == "countdown") menuIndex = 3;
            else if (mode == "weather") menuIndex = 4;
        }
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
        addCORSHeaders(request, response);
        request->send(response); });

    // ===== ALARM =====
    server.on("/api/alarm", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("hour", true)) alarmHour = request->getParam("hour", true)->value().toInt();
        if (request->hasParam("min", true)) alarmMin = request->getParam("min", true)->value().toInt();
        if (request->hasParam("on", true)) alarmOn = request->getParam("on", true)->value().toInt() != 0;
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
        addCORSHeaders(request, response);
        request->send(response); });

    // ===== BUZZER =====
    server.on("/api/buzzer", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("buzzer", true)) buzzerState = request->getParam("buzzer", true)->value().toInt() != 0;
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
        addCORSHeaders(request, response);
        request->send(response); });

    // ===== COUNTDOWN =====
    server.on("/api/countdown", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("seconds", true)) {
            countdownTime = millis() + (request->getParam("seconds", true)->value().toInt() * 1000);
            countdownActive = true;
        }
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
        addCORSHeaders(request, response);
        request->send(response); });
}
