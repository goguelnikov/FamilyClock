#pragma once
// ============================================================
//  WeatherService.h  —  Météo via OpenWeatherMap (version simple)
// ============================================================

#include <Arduino.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"

struct WeatherData {
    char     description[32];
    char     iconCode[4];
    float    tempCurrent  = 0;
    float    windSpeed    = 0;
    uint16_t pressure     = 0;
    uint8_t  humidity     = 0;
    bool     valid        = false;
    uint32_t fetchedAt    = 0;
};

enum WeatherIcon : uint8_t {
    WI_CLEAR_DAY=0, WI_CLEAR_NIGHT=1, WI_FEW_CLOUDS=2, WI_CLOUDS=3,
    WI_SHOWER=4,    WI_RAIN=5,        WI_THUNDER=6,    WI_SNOW=7,
    WI_MIST=8,      WI_UNKNOWN=9
};

class WeatherService {
public:
    static WeatherService& instance() {
        static WeatherService inst;
        return inst;
    }

    WeatherData data;

    void begin() {
        // Forcer un premier fetch après 20s (laisser WiFi se stabiliser)
        data.fetchedAt = millis() - _refreshMs() + 20000;
    }

    uint32_t _refreshMs() {
        return ConfigManager::instance().cfg.weatherRefreshMs;
    }

    void loop() {
        AppConfig& cfg = ConfigManager::instance().cfg;
        if (millis() - data.fetchedAt > cfg.weatherRefreshMs)
            fetch();
    }

    void fetch() {
        if (WiFi.status() != WL_CONNECTED) return;
        AppConfig& cfg = ConfigManager::instance().cfg;
        if (strlen(cfg.owmApiKey) == 0) return;

        String url = "http://api.openweathermap.org/data/2.5/weather?q=";
        url += cfg.owmCity;
        url += "&appid="; url += cfg.owmApiKey;
        url += "&units="; url += cfg.useCelsius ? "metric" : "imperial";
        url += "&lang=fr";

        WiFiClient client;
        HTTPClient http;
        http.begin(client, url);
        int code = http.GET();

        if (code == 200) {
            JsonDocument doc;
            if (deserializeJson(doc, http.getStream()) == DeserializationError::Ok) {
                strlcpy(data.description,
                        doc["weather"][0]["description"] | "?",
                        sizeof(data.description));
                strlcpy(data.iconCode,
                        doc["weather"][0]["icon"] | "01d",
                        sizeof(data.iconCode));
                data.tempCurrent = doc["main"]["temp"]     | 0.0f;
                data.humidity    = doc["main"]["humidity"] | 0;
                data.windSpeed   = doc["wind"]["speed"]    | 0.0f;
                data.pressure    = doc["main"]["pressure"] | 0;
                data.valid       = true;
                data.fetchedAt   = millis();
                Serial.printf("[Weather] %s %.1f deg\n",
                              data.description, data.tempCurrent);
            }
        } else {
            Serial.printf("[Weather] HTTP error %d\n", code);
        }
        http.end();
    }

    WeatherIcon getIcon() const {
        if (!data.valid) return WI_UNKNOWN;
        String c = String(data.iconCode);
        if (c == "01d") return WI_CLEAR_DAY;
        if (c == "01n") return WI_CLEAR_NIGHT;
        if (c == "02d" || c == "02n") return WI_FEW_CLOUDS;
        if (c.startsWith("03") || c.startsWith("04")) return WI_CLOUDS;
        if (c.startsWith("09")) return WI_SHOWER;
        if (c.startsWith("10")) return WI_RAIN;
        if (c.startsWith("11")) return WI_THUNDER;
        if (c.startsWith("13")) return WI_SNOW;
        if (c.startsWith("50")) return WI_MIST;
        return WI_UNKNOWN;
    }

    String tempStr() const {
        if (!data.valid) return "--";
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", (int)round(data.tempCurrent));
        return String(buf);
    }

    char tempUnit() const {
        return ConfigManager::instance().cfg.useCelsius ? 'C' : 'F';
    }
};
