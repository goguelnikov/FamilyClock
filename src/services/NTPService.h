#pragma once
// ============================================================
//  NTPService.h  —  Synchronisation heure via NTP
//  Cache de l'heure mis à jour 1x/seconde pour éviter
//  les appels bloquants à getLocalTime() dans loop()
// ============================================================

#include <Arduino.h>
#include <time.h>
#include "esp_sntp.h"
#include "Config.h"

class NTPService {
public:
    static NTPService& instance() {
        static NTPService inst;
        return inst;
    }

    void begin() {
        AppConfig& cfg = ConfigManager::instance().cfg;
        configTime(cfg.gmtOffsetSec, cfg.daylightOffsetSec,
                   cfg.ntpServer, "time.google.com", "time.cloudflare.com");
        Serial.println("[NTP] SNTP configure...");
        _lastSyncAttempt = millis();
        // Attente non bloquante max 5s
        uint32_t t = millis();
        while (!_checkSynced() && millis() - t < 5000) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        if (_synced) _updateCache();
    }

    bool isSynced() const { return _synced; }

    // Accès rapide depuis le cache (non bloquant)
    int hour()   const { return _tm.tm_hour; }
    int minute() const { return _tm.tm_min;  }
    int second() const { return _tm.tm_sec;  }
    int wday()   const { return _tm.tm_wday; }
    int mday()   const { return _tm.tm_mday; }
    int month()  const { return _tm.tm_mon + 1; }
    int year()   const { return _tm.tm_year + 1900; }

    // Pour compatibilité avec CalendarService
    bool now(struct tm& timeinfo) {
        timeinfo = _tm;
        return _synced;
    }

    String timeStr() const {
        if (!_synced) return "--:--";
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", _tm.tm_hour, _tm.tm_min);
        return String(buf);
    }

    String dateStr() const {
        if (!_synced) return "--/--/----";
        char buf[11];
        snprintf(buf, sizeof(buf), "%02d/%02d/%04d",
                 _tm.tm_mday, _tm.tm_mon+1, _tm.tm_year+1900);
        return String(buf);
    }

    const char* dayName() const {
        static const char* jours[] = {
            "Dimanche","Lundi","Mardi","Mercredi","Jeudi","Vendredi","Samedi"
        };
        if (!_synced) return "?";
        return jours[_tm.tm_wday];
    }

    // Appeler dans loop() — met à jour le cache 1x/seconde, non bloquant
    void loop() {
        uint32_t now = millis();

        // Mise à jour du cache 1x par seconde
        if (now - _lastCacheUpdate >= 1000) {
            _lastCacheUpdate = now;
            if (_synced) {
                _updateCache();
            } else if (now - _lastSyncAttempt > 60000) { // vérif sync 1x/min max
                _lastSyncAttempt = now;
                _checkSynced();
            }
        }

        // Re-sync : SNTP tourne en arrière-plan automatiquement, pas besoin de forcer
    }

private:
    bool      _synced            = false;
    struct tm _tm                = {};
    uint32_t  _lastSync          = 0;
    uint32_t  _lastSyncAttempt   = 0;
    uint32_t  _lastCacheUpdate   = 0;

    void _updateCache() {
        // time()+localtime_r est vraiment non bloquant
        time_t now = time(nullptr);
        localtime_r(&now, &_tm);
    }

    bool _checkSynced() {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            if (!_synced) {
                _synced   = true;
                _lastSync = millis();
                time_t now = time(nullptr); localtime_r(&now, &_tm); // non bloquant
                Serial.printf("[NTP] Sync OK: %02d:%02d:%02d\n",
                              _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
            }
            return true;
        }
        return false;
    }
};
