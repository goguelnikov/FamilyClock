#pragma once
// ============================================================
//  WebConfig.h  —  Interface de configuration (ESPAsyncWebServer)
//  Version stable — structure originale qui fonctionnait
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <SPIFFS.h>
#include "Config.h"
#include "DisplayHelper.h"
#include "services/NTPService.h"
#include "services/WeatherService.h"

class WebConfig {
public:
    static WebConfig& instance() {
        static WebConfig inst;
        return inst;
    }

    void setMatrix(MatrixPanel_I2S_DMA* mx) { _matrix = mx; }

    void begin() {
        AppConfig& cfg = ConfigManager::instance().cfg;

        if (!MDNS.begin(cfg.mdnsName)) {
            Serial.println("[mDNS] Erreur init");
        } else {
            Serial.printf("[mDNS] %s.local actif\n", cfg.mdnsName);
        }

        // ---- Servir l'interface HTML ----
        // Servir les fichiers depuis SPIFFS
        _server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

        // ---- API REST ----

        // GET /api/config

        // POST /api/config

        // GET /api/status
        _server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
            JsonDocument doc;
            doc["uptime"]    = millis() / 1000;
            doc["freeHeap"]  = ESP.getFreeHeap();
            doc["wifiRSSI"]  = WiFi.RSSI();
            doc["wifiSSID"]  = WiFi.SSID();
            doc["wifiIP"]    = WiFi.localIP().toString();
            doc["ntpSynced"] = NTPService::instance().isSynced();
            doc["weatherOk"] = WeatherService::instance().data.valid;
            doc["time"]      = NTPService::instance().timeStr();
            doc["date"]      = NTPService::instance().dateStr();
            doc["deviceName"]= ConfigManager::instance().cfg.deviceName;
            String out;
            serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        // POST /api/weather/refresh
        _server.on("/api/weather/refresh", HTTP_POST, [](AsyncWebServerRequest* req) {
            // Notifier la tâche météo pour un fetch immédiat
            extern TaskHandle_t weatherTaskHandle;
            if (weatherTaskHandle) xTaskNotifyGive(weatherTaskHandle);
            else WeatherService::instance().data.fetchedAt = 0;
            req->send(200, "application/json", "{\"ok\":true}");
        });

        
        
        
        
        
        
        
        // ── GET par section (6 routes) ──────────────────────────

        _server.on("/api/config/wifi", HTTP_GET, [](AsyncWebServerRequest* req) {
            uint32_t _t0 = millis();
            Serial.printf("[API] GET /api/config/wifi t=%lu\n", _t0);
            AppConfig& cfg = ConfigManager::instance().cfg;
            JsonDocument doc;
            doc["deviceName"] = cfg.deviceName;
            doc["mdnsName"]   = cfg.mdnsName;
            doc["wifiSSID"]   = cfg.wifiSSID;
            String out; serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        _server.on("/api/config/time", HTTP_GET, [](AsyncWebServerRequest* req) {
            uint32_t _t0 = millis();
            Serial.printf("[API] GET /api/config/time t=%lu\n", _t0);
            AppConfig& cfg = ConfigManager::instance().cfg;
            JsonDocument doc;
            doc["ntpServer"]         = cfg.ntpServer;
            doc["gmtOffsetSec"]      = cfg.gmtOffsetSec;
            doc["daylightOffsetSec"] = cfg.daylightOffsetSec;
            String out; serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        _server.on("/api/config/weather", HTTP_GET, [](AsyncWebServerRequest* req) {
            uint32_t _t0 = millis();
            Serial.printf("[API] GET /api/config/weather t=%lu\n", _t0);
            AppConfig& cfg = ConfigManager::instance().cfg;
            JsonDocument doc;
            doc["owmApiKey"]        = cfg.owmApiKey;
            doc["owmCity"]          = cfg.owmCity;
            doc["useCelsius"]       = cfg.useCelsius;
            doc["weatherRefreshMs"] = cfg.weatherRefreshMs;
            String out; serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        _server.on("/api/config/display", HTTP_GET, [](AsyncWebServerRequest* req) {
            uint32_t _t0 = millis();
            Serial.printf("[API] GET /api/config/display t=%lu\n", _t0);
            AppConfig& cfg = ConfigManager::instance().cfg;
            JsonDocument doc;
            doc["brightness"]       = cfg.brightness;
            doc["screenDurationMs"] = cfg.screenDurationMs;
            doc["autoRotate"]       = cfg.autoRotate;
            doc["themeId"]          = cfg.themeId;
            JsonArray sc = doc["screensEnabled"].to<JsonArray>();
            for (uint8_t i = 0; i < SCREEN_COUNT; i++) sc.add(cfg.screenEnabled[i]);
            JsonArray th = doc["themes"].to<JsonArray>();
            for (uint8_t i = 0; i < THEME_COUNT; i++) th.add(THEMES[i].name);
            String out; serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        _server.on("/api/config/notes", HTTP_GET, [](AsyncWebServerRequest* req) {
            uint32_t _t0 = millis();
            Serial.printf("[API] GET /api/config/notes t=%lu\n", _t0);
            AppConfig& cfg = ConfigManager::instance().cfg;
            JsonDocument doc;
            JsonArray na = doc["notes"].to<JsonArray>();
            for (uint8_t i = 0; i < 4; i++) na.add(cfg.notes[i]);
            String out; serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        _server.on("/api/config/calendar", HTTP_GET, [](AsyncWebServerRequest* req) {
            uint32_t _t0 = millis();
            Serial.printf("[API] GET /api/config/calendar t=%lu\n", _t0);
            AppConfig& cfg = ConfigManager::instance().cfg;
            JsonDocument doc;
            JsonArray acts = doc["activities"].to<JsonArray>();
            for (uint8_t i = 0; i < cfg.activityCount; i++) {
                Activity& a = cfg.activities[i];
                JsonObject o = acts.add<JsonObject>();
                o["label"]=a.label; o["icon"]=a.icon;
                o["hour"]=a.hour;   o["minute"]=a.minute;
                o["days"]=a.daysMask; o["alert"]=a.alertMinutes;
            }
            String out; serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        _server.on("/api/config/birthdays", HTTP_GET, [](AsyncWebServerRequest* req) {
            uint32_t _t0 = millis();
            Serial.printf("[API] GET /api/config/birthdays t=%lu\n", _t0);
            AppConfig& cfg = ConfigManager::instance().cfg;
            JsonDocument doc;
            JsonArray bdArr = doc["birthdays"].to<JsonArray>();
            for (uint8_t i = 0; i < cfg.birthdayCount; i++) {
                Birthday& b = cfg.birthdays[i];
                JsonObject o = bdArr.add<JsonObject>();
                o["name"]=b.name; o["day"]=b.day; o["month"]=b.month;
            }
            String out; serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        // ── POST par section ─────────────────────────────────────

        // POST /api/config/wifi
        _server.on("/api/config/wifi", HTTP_POST,
            [](AsyncWebServerRequest* req) {},
            nullptr,
            [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
                WebConfig& wc = WebConfig::instance();
                if (index == 0) wc._bodyLen = 0;
                size_t copy = min(len, sizeof(wc._bodyBuf) - wc._bodyLen - 1);
                memcpy(wc._bodyBuf + wc._bodyLen, data, copy); wc._bodyLen += copy;
                if (index + len < total) return;
                wc._bodyBuf[wc._bodyLen] = '\0';
                JsonDocument doc;
                if (deserializeJson(doc, wc._bodyBuf, wc._bodyLen) == DeserializationError::Ok) {
                    AppConfig& cfg = ConfigManager::instance().cfg;
                    if (doc["deviceName"].is<const char*>()) strlcpy(cfg.deviceName, doc["deviceName"], sizeof(cfg.deviceName));
                    if (doc["mdnsName"].is<const char*>() && strlen(doc["mdnsName"]) > 0) strlcpy(cfg.mdnsName, doc["mdnsName"], sizeof(cfg.mdnsName));
                    if (doc["wifiPass"].is<const char*>() && strlen(doc["wifiPass"]) > 0) strlcpy(cfg.wifiPass, doc["wifiPass"], sizeof(cfg.wifiPass));
                    ConfigManager::instance().save();
                }
                req->send(200, "application/json", "{\"ok\":true}");
            }
        );

        // POST /api/config/time
        _server.on("/api/config/time", HTTP_POST,
            [](AsyncWebServerRequest* req) {},
            nullptr,
            [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
                WebConfig& wc = WebConfig::instance();
                if (index == 0) wc._bodyLen = 0;
                size_t copy = min(len, sizeof(wc._bodyBuf) - wc._bodyLen - 1);
                memcpy(wc._bodyBuf + wc._bodyLen, data, copy); wc._bodyLen += copy;
                if (index + len < total) return;
                wc._bodyBuf[wc._bodyLen] = '\0';
                JsonDocument doc;
                if (deserializeJson(doc, wc._bodyBuf, wc._bodyLen) == DeserializationError::Ok) {
                    AppConfig& cfg = ConfigManager::instance().cfg;
                    if (doc["ntpServer"].is<const char*>()) strlcpy(cfg.ntpServer, doc["ntpServer"], sizeof(cfg.ntpServer));
                    if (doc["gmtOffsetSec"].is<int>()) cfg.gmtOffsetSec = doc["gmtOffsetSec"];
                    if (doc["daylightOffsetSec"].is<int>()) cfg.daylightOffsetSec = doc["daylightOffsetSec"];
                    ConfigManager::instance().save();
                }
                req->send(200, "application/json", "{\"ok\":true}");
            }
        );

        // POST /api/config/weather
        _server.on("/api/config/weather", HTTP_POST,
            [](AsyncWebServerRequest* req) {},
            nullptr,
            [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
                WebConfig& wc = WebConfig::instance();
                if (index == 0) wc._bodyLen = 0;
                size_t copy = min(len, sizeof(wc._bodyBuf) - wc._bodyLen - 1);
                memcpy(wc._bodyBuf + wc._bodyLen, data, copy); wc._bodyLen += copy;
                if (index + len < total) return;
                wc._bodyBuf[wc._bodyLen] = '\0';
                JsonDocument doc;
                if (deserializeJson(doc, wc._bodyBuf, wc._bodyLen) == DeserializationError::Ok) {
                    AppConfig& cfg = ConfigManager::instance().cfg;
                    if (doc["owmApiKey"].is<const char*>() && strlen(doc["owmApiKey"]) > 0) strlcpy(cfg.owmApiKey, doc["owmApiKey"], sizeof(cfg.owmApiKey));
                    if (doc["owmCity"].is<const char*>()) strlcpy(cfg.owmCity, doc["owmCity"], sizeof(cfg.owmCity));
                    if (doc["useCelsius"].is<bool>()) cfg.useCelsius = doc["useCelsius"];
                    if (doc["weatherRefreshMs"].is<uint32_t>()) cfg.weatherRefreshMs = doc["weatherRefreshMs"];
                    ConfigManager::instance().save();
                    WeatherService::instance().data.fetchedAt = 0;
                }
                req->send(200, "application/json", "{\"ok\":true}");
            }
        );

        // POST /api/config/display
        _server.on("/api/config/display", HTTP_POST,
            [](AsyncWebServerRequest* req) {},
            nullptr,
            [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
                WebConfig& wc = WebConfig::instance();
                if (index == 0) wc._bodyLen = 0;
                size_t copy = min(len, sizeof(wc._bodyBuf) - wc._bodyLen - 1);
                memcpy(wc._bodyBuf + wc._bodyLen, data, copy); wc._bodyLen += copy;
                if (index + len < total) return;
                wc._bodyBuf[wc._bodyLen] = '\0';
                JsonDocument doc;
                if (deserializeJson(doc, wc._bodyBuf, wc._bodyLen) == DeserializationError::Ok) {
                    AppConfig& cfg = ConfigManager::instance().cfg;
                    if (doc["brightness"].is<uint8_t>()) {
                        cfg.brightness = (uint8_t)(doc["brightness"] | 0);
                        uint8_t bri8 = cfg.brightness == 0 ? 0 : (uint8_t)max(1, (int)cfg.brightness * cfg.brightness * 255 / 10000);
                        if (wc._matrix) wc._matrix->setBrightness8(bri8);
                    }
                    if (doc["screenDurationMs"].is<uint32_t>()) cfg.screenDurationMs = doc["screenDurationMs"];
                    if (doc["autoRotate"].is<bool>()) cfg.autoRotate = doc["autoRotate"];
                    if (doc["themeId"].is<uint8_t>()) cfg.themeId = (uint8_t)(doc["themeId"] | 0);
                    if (doc["screensEnabled"].is<JsonArray>()) {
                        JsonArray arr = doc["screensEnabled"].as<JsonArray>();
                        for (uint8_t i = 0; i < SCREEN_COUNT && i < arr.size(); i++)
                            cfg.screenEnabled[i] = arr[i].as<bool>();
                    }
                    ConfigManager::instance().save();
                    // Appliquer le thème immédiatement
                    if (wc._matrix) Theme565::refresh(wc._matrix);
                }
                req->send(200, "application/json", "{\"ok\":true}");
            }
        );

        // POST /api/config/notes
        _server.on("/api/config/notes", HTTP_POST,
            [](AsyncWebServerRequest* req) {},
            nullptr,
            [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
                WebConfig& wc = WebConfig::instance();
                if (index == 0) wc._bodyLen = 0;
                size_t copy = min(len, sizeof(wc._bodyBuf) - wc._bodyLen - 1);
                memcpy(wc._bodyBuf + wc._bodyLen, data, copy); wc._bodyLen += copy;
                if (index + len < total) return;
                wc._bodyBuf[wc._bodyLen] = '\0';
                JsonDocument doc;
                if (deserializeJson(doc, wc._bodyBuf, wc._bodyLen) == DeserializationError::Ok) {
                    AppConfig& cfg = ConfigManager::instance().cfg;
                    if (doc["notes"].is<JsonArray>()) {
                        JsonArray na = doc["notes"].as<JsonArray>();
                        for (uint8_t i = 0; i < 4 && i < na.size(); i++) {
                            const char* s = na[i].as<const char*>();
                            strlcpy(cfg.notes[i], s ? s : "", 32);
                        }
                    }
                    ConfigManager::instance().save();
                }
                req->send(200, "application/json", "{\"ok\":true}");
            }
        );

        // POST /api/config/calendar → activités seules
        _server.on("/api/config/calendar", HTTP_POST,
            [](AsyncWebServerRequest* req) {},
            nullptr,
            [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
                WebConfig& wc = WebConfig::instance();
                if (index == 0) wc._bodyLen = 0;
                size_t copy = min(len, sizeof(wc._bodyBuf) - wc._bodyLen - 1);
                memcpy(wc._bodyBuf + wc._bodyLen, data, copy);
                wc._bodyLen += copy;
                if (index + len < total) return;
                wc._bodyBuf[wc._bodyLen] = '\0';
                JsonDocument doc;
                if (deserializeJson(doc, wc._bodyBuf, wc._bodyLen) == DeserializationError::Ok) {
                    AppConfig& cfg = ConfigManager::instance().cfg;
                    if (doc["activities"].is<JsonArray>()) {
                        cfg.activityCount = 0;
                        for (JsonObject o : doc["activities"].as<JsonArray>()) {
                            if (cfg.activityCount >= MAX_ACTIVITIES) break;
                            Activity& a = cfg.activities[cfg.activityCount++];
                            strlcpy(a.label, o["label"] | "?",    sizeof(a.label));
                            strlcpy(a.icon,  o["icon"]  | "star", sizeof(a.icon));
                            a.hour         = o["hour"]   | 0;
                            a.minute       = o["minute"] | 0;
                            a.daysMask     = (uint8_t)(o["days"]  | DAY_ALL);
                            a.alertMinutes = (uint8_t)(o["alert"] | 0);
                        }
                        ConfigManager::instance().save();
                    }
                }
                req->send(200, "application/json", "{\"ok\":true}");
            }
        );

        // POST /api/config/birthdays → anniversaires seuls
        _server.on("/api/config/birthdays", HTTP_POST,
            [](AsyncWebServerRequest* req) {},
            nullptr,
            [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
                WebConfig& wc = WebConfig::instance();
                if (index == 0) wc._bodyLen = 0;
                size_t copy = min(len, sizeof(wc._bodyBuf) - wc._bodyLen - 1);
                memcpy(wc._bodyBuf + wc._bodyLen, data, copy);
                wc._bodyLen += copy;
                if (index + len < total) return;
                wc._bodyBuf[wc._bodyLen] = '\0';
                JsonDocument doc;
                auto err = deserializeJson(doc, wc._bodyBuf, wc._bodyLen);
                if (err == DeserializationError::Ok) {
                    AppConfig& cfg = ConfigManager::instance().cfg;
                    if (doc["birthdays"].is<JsonArray>()) {
                        cfg.birthdayCount = 0;
                        for (JsonObject o : doc["birthdays"].as<JsonArray>()) {
                            if (cfg.birthdayCount >= MAX_BIRTHDAYS) break;
                            Birthday& b = cfg.birthdays[cfg.birthdayCount++];
                            strlcpy(b.name, o["name"] | "", sizeof(b.name));
                            b.day   = o["day"]   | 1;
                            b.month = o["month"] | 1;
                        }
                        ConfigManager::instance().save();
                        // Vérifier relecture immédiate
                    }
                }
                req->send(200, "application/json", "{\"ok\":true}");
            }
        );

                // favicon.ico → 204 pour éviter le 500
        _server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* req) {
            req->send(204);
        });

        // POST /api/brightness → appliquer immédiatement sans sauvegarder
        _server.on("/api/brightness", HTTP_POST,
            [](AsyncWebServerRequest* req) {},
            nullptr,
            [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                JsonDocument doc;
                if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
                    uint8_t pct = (uint8_t)(doc["brightness"] | 30);
                    ConfigManager::instance().cfg.brightness = pct;
                    uint8_t bri8 = pct == 0 ? 0 : (uint8_t)max(1, (int)pct * pct * 255 / 10000);
                    if (WebConfig::instance()._matrix)
                        WebConfig::instance()._matrix->setBrightness8(bri8);
                }
                req->send(200, "application/json", "{\"ok\":true}");
            }
        );

        // POST /api/wifi/reset
        _server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest* req) {
            req->send(200, "application/json", "{\"ok\":true}");
            xTaskCreate([](void*) {
                vTaskDelay(pdMS_TO_TICKS(500));
                WiFiManager wm;
                wm.resetSettings();
                ESP.restart();
            }, "wifireset", 2048, nullptr, 1, nullptr);
        });

        _server.onNotFound([](AsyncWebServerRequest* req) {
            req->send(404, "text/plain", "Not found");
        });

        _server.begin();
        Serial.println("[WebConfig] Serveur démarré sur port 80");
    }

    MatrixPanel_I2S_DMA* _matrix = nullptr;
    char _bodyBuf[2048];  // buffer statique pour les bodies POST
    size_t _bodyLen = 0;

private:
    AsyncWebServer _server{80};
};
