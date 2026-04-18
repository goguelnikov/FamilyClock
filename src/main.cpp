// ============================================================
//  main.cpp  —  Horloge Familiale ESP32 + HUB75 64x32
//  Version stable — structure originale
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "esp_task_wdt.h"

#include "Config.h"
#include "ScreenManager.h"
#include "WebConfig.h"
#include "DisplayHelper.h"
#include "services/NTPService.h"
#include "services/WeatherService.h"
#include "services/CalendarService.h"
#include "screens/ScreenBoot.h"
#include "screens/ScreenClock.h"
#include "screens/ScreenWeather.h"
#include "screens/ScreenNext.h"
#include "screens/ScreenAgenda.h"
#include "screens/ScreenMoon.h"
#include "screens/ScreenNotes.h"
#include "screens/ScreenBirthdays.h"

// ---- Panneau HUB75 ----
MatrixPanel_I2S_DMA* matrix = nullptr;

// ---- Instances écrans ----
ScreenClock*    screenClock    = nullptr;
ScreenWeather*  screenWeather  = nullptr;
ScreenNext*     screenNext     = nullptr;
ScreenAgenda*   screenAgenda   = nullptr;
ScreenMoon*     screenMoon     = nullptr;
ScreenNotes*      screenNotes      = nullptr;
ScreenBirthdays*  screenBirthdays  = nullptr;

// ============================================================
//  Connexion WiFi
// ============================================================
bool connectWiFi() {
    AppConfig& cfg = ConfigManager::instance().cfg;

    // Tentative directe si credentials connus
    if (strlen(cfg.wifiSSID) > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(cfg.wifiSSID, cfg.wifiPass);
        uint8_t tries = 0;
        while (WiFi.status() != WL_CONNECTED && tries < 20) {
            delay(500); tries++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Connexion directe OK : %s\n",
                          WiFi.localIP().toString().c_str());
            return true;
        }
        Serial.println("[WiFi] Connexion directe échouée, portail captif...");
    }

    // Portail captif WiFiManager
    ScreenBoot::show(matrix, "Config WiFi...");
    WiFiManager wm;
    wm.setDebugOutput(false);
    wm.setAPCallback([](WiFiManager* w) {
        ScreenBoot::show(matrix, "Horloge-Config");
    });
    wm.setConfigPortalTimeout(180);
    wm.setConnectTimeout(15);
    wm.setConnectRetries(2);

    if (wm.autoConnect("Horloge-Config", "horloge123")) {
        WiFi.SSID().toCharArray(cfg.wifiSSID, sizeof(cfg.wifiSSID));
        WiFi.psk().toCharArray(cfg.wifiPass, sizeof(cfg.wifiPass));
        ConfigManager::instance().save();
        Serial.printf("[WiFi] OK via portail : %s\n",
                      WiFi.localIP().toString().c_str());
        return true;
    }
    Serial.println("[WiFi] Échec connexion");
    return false;
}

// ============================================================
//  Tâche FreeRTOS pour la météo (non bloquant pour loop())
// ============================================================
TaskHandle_t weatherTaskHandle = nullptr; // extern dans WebConfig.h

void weatherTask(void* param) {
    // Attendre 20s au boot pour laisser WiFi se stabiliser
    vTaskDelay(pdMS_TO_TICKS(20000));
    while (true) {
        if (WiFi.status() == WL_CONNECTED)
            WeatherService::instance().fetch();
        // Attendre notification (refresh forcé) ou timeout (refresh auto)
        AppConfig& cfg = ConfigManager::instance().cfg;
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(cfg.weatherRefreshMs));
    }
}

// ============================================================
//  setup()
// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Horloge Familiale v1.0 ===");

    // Désactiver WDT sur loopTask
    esp_task_wdt_delete(NULL);

    // --- Config ---
    ConfigManager::instance().load();
    AppConfig& cfg = ConfigManager::instance().cfg;

    // --- Panneau HUB75 ---
    HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, CHAIN_LENGTH);
    mxconfig.clkphase = false;
    matrix = new MatrixPanel_I2S_DMA(mxconfig);
    matrix->begin();
    // Courbe quadratique : 0%=éteint, 5%=1/255, 100%=255
    uint8_t bri8 = cfg.brightness == 0 ? 0 : (uint8_t)max(1, (int)cfg.brightness * cfg.brightness * 255 / 10000);
    matrix->setBrightness8(bri8);
    matrix->clearScreen();
    matrix->setFont(nullptr);
    Theme565::refresh(matrix);

    ScreenBoot::show(matrix, "Démarrage...");

    // --- SPIFFS ---
    if (!SPIFFS.begin(true))
        Serial.println("[SPIFFS] Erreur montage");

    // --- WiFi ---
    ScreenBoot::show(matrix, "WiFi...");
    bool wifiOk = connectWiFi();

    // --- NTP ---
    if (wifiOk) {
        ScreenBoot::show(matrix, "NTP sync...");
        NTPService::instance().begin();
    }

    // --- Interface web ---
    if (wifiOk) {
        WebConfig::instance().setMatrix(matrix);
        WebConfig::instance().begin();
    }

    // Démarrer la tâche météo en arrière-plan (Core 0)
    if (wifiOk && strlen(cfg.owmApiKey) > 0) {
        xTaskCreatePinnedToCore(weatherTask, "weather", 8192, nullptr, 1, &weatherTaskHandle, 0);
    }

    // Afficher IP 2s
    if (wifiOk) {
        ScreenBoot::show(matrix, WiFi.localIP().toString().c_str());
        delay(2000);
    }

    // --- Écrans ---
    screenClock   = new ScreenClock(matrix);
    screenWeather = new ScreenWeather(matrix);
    screenNext    = new ScreenNext(matrix);
    screenAgenda  = new ScreenAgenda(matrix);
    screenMoon    = new ScreenMoon(matrix);
    screenNotes      = new ScreenNotes(matrix);
    screenBirthdays  = new ScreenBirthdays(matrix);

    ScreenManager& sm = ScreenManager::instance();
    sm.registerScreen(SCREEN_CLOCK,    screenClock);
    sm.registerScreen(SCREEN_WEATHER,  screenWeather);
    sm.registerScreen(SCREEN_ACTIVITY, screenNext);
    sm.registerScreen(SCREEN_AGENDA,   screenAgenda);
    sm.registerScreen(SCREEN_MOON,     screenMoon);
    sm.registerScreen(SCREEN_NOTES,      screenNotes);
    sm.registerScreen(SCREEN_BIRTHDAYS,  screenBirthdays);
    sm.begin();

    Serial.println("[BOOT] Prêt !");
    if (wifiOk) {
        Serial.printf("[BOOT] Interface web : http://%s/\n",
                      WiFi.localIP().toString().c_str());
        Serial.printf("[BOOT] ou http://%s.local/\n", cfg.mdnsName);
    }
}

// ============================================================
//  loop()
// ============================================================
void loop() {
    uint32_t t0, t1;

    t0 = micros();
    NTPService::instance().loop();
    t1 = micros();
    if (t1-t0 > 5000) Serial.printf("[LOOP] NTP: %lu us\n", t1-t0);

    t0 = micros();
    ScreenManager::instance().loop();
    t1 = micros();
    if (t1-t0 > 5000) Serial.printf("[LOOP] Screen: %lu us\n", t1-t0);

    yield();
}
