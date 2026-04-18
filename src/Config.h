#pragma once
// ============================================================
//  Config.h  —  Types partagés et configuration globale
// ============================================================

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// ---- Identifiants d'écrans ----
enum ScreenID : uint8_t {
    SCREEN_CLOCK    = 0,
    SCREEN_WEATHER  = 1,
    SCREEN_ACTIVITY = 2,
    SCREEN_AGENDA   = 3,
    SCREEN_MOON     = 4,
    SCREEN_NOTES      = 5,
    SCREEN_BIRTHDAYS  = 6,
    SCREEN_COUNT      = 7
};

// ---- Jours de la semaine (bitmask) ----
//  bit 0 = Dimanche, bit 1 = Lundi ... bit 6 = Samedi
#define DAY_SUN (1<<0)
#define DAY_MON (1<<1)
#define DAY_TUE (1<<2)
#define DAY_WED (1<<3)
#define DAY_THU (1<<4)
#define DAY_FRI (1<<5)
#define DAY_SAT (1<<6)
#define DAY_WEEKDAYS (DAY_MON|DAY_TUE|DAY_WED|DAY_THU|DAY_FRI)
#define DAY_ALL      (0x7F)

// ---- Activité calendrier ----
struct Activity {
    char     label[24];   // ex: "École"
    char     icon[16];    // ex: "bag"
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  daysMask;    // bitmask jours actifs
    uint8_t  alertMinutes; // délai d'alerte anticipé (0 = pas d'alerte)
};

#define MAX_ACTIVITIES 20

// ---- Anniversaire ----
struct Birthday {
    char    name[24];  // prénom
    uint8_t day;       // jour (1-31)
    uint8_t month;     // mois (1-12)
};
#define MAX_BIRTHDAYS 20

// ---- Configuration persistante ----
struct AppConfig {
    // Identité du dispositif
    char deviceName[32]; // ex: "Horloge Chambre"
    char mdnsName[24];  // ex: "horloge-chambre" (sans espaces)

    // WiFi
    char wifiSSID[33];
    char wifiPass[65];

    // NTP / fuseau
    char ntpServer[64];
    int  gmtOffsetSec;      // ex: 3600 pour UTC+1
    int  daylightOffsetSec; // ex: 3600 en été

    // Météo
    char owmApiKey[48];     // OpenWeatherMap API key
    char owmCity[48];       // ex: "Paris,FR"
    bool useCelsius;
    uint32_t weatherRefreshMs; // ex: 600000 = 10 min

    // Affichage
    uint8_t  brightness;       // 0–100 (pourcentage, converti en PWM gamma dans le code)
    uint32_t screenDurationMs; // durée par écran en rotation (ms)
    bool     autoRotate;
    uint8_t  themeId;          // index dans THEMES[]
    bool     screenEnabled[SCREEN_COUNT]; // activer/désactiver chaque écran (5 écrans)

    // Calendrier
    uint8_t  activityCount;
    Activity activities[MAX_ACTIVITIES];

    // Anniversaires
    uint8_t  birthdayCount;
    Birthday birthdays[MAX_BIRTHDAYS];

    // Notes (4 lignes max 32 chars)
    char     notes[4][32];
};

// ---- Thèmes de couleurs ----
struct Theme {
    const char* name;
    uint8_t  r1, g1, b1;   // couleur primaire   (textes principaux)
    uint8_t  r2, g2, b2;   // couleur secondaire (labels, dates)
    uint8_t  ra, ga, ba;   // couleur accent      (titres, heures)
    uint8_t  rd, gd, bd;   // couleur dim         (éléments passés)
    bool     blackBg;       // true = fond noir pur (LEDs éteintes)
};

// Thèmes prédéfinis
// Rôles des couleurs :
//   primary   = textes principaux (noms, descriptions)
//   secondary = textes secondaires (dates, labels)
//   accent    = titres + heures (le plus visible)
//   dim       = discret, éléments passés
//   bg        = calculé automatiquement depuis primary (primary/12)
// Note : les composantes sont stockées en ordre R,B,G (swap G↔B)
// car le panneau HUB75 utilisé interprète color565(r,g,b) comme R,B,G
static const Theme THEMES[] = {
    //  name            primary(R,B,G)       secondary(R,B,G)     accent(R,B,G)        dim(R,B,G)
    { "Nuit bleue",  100, 255, 180,      60, 180, 120,       255, 255, 255,       30,  80,  50, false },
    { "Foret",        80,  60, 180,     120,  40,  80,       160,  80, 220,       30,  20,  60, false },
    { "Coucher",     255,  60, 160,     200,  40, 120,       255, 180, 240,       80,  10,  40, false },
    { "Lavande",     200, 255, 160,     160, 200, 120,       255, 255, 220,       60,  80,  30, false },
    { "Glace",       180, 240, 220,     120, 200, 170,       220, 255, 245,       40,  90,  70, false },
    { "Monochrome",  200, 200, 200,     140, 140, 140,       255, 255, 255,       60,  60,  60, false },
    { "Vert LED",      0,   0, 200,       0,   0, 140,         0,   0, 255,        0,   0,  60, false },
    { "Rouge LED",   220,   0,   0,     160,   0,   0,       255,  80,  80,       80,   0,   0, false },
    { "Minuit",      200, 200, 200,     160, 160, 160,       255, 255, 255,       80,  80,  80, true  },
};
static const uint8_t THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

// ---- Singleton config ----
class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager inst;
        return inst;
    }

    AppConfig cfg;

    void load() {
        Preferences prefs;
        prefs.begin("horloge", true);

        // Identité
        strlcpy(cfg.deviceName,   prefs.getString("devName", "Horloge Familiale").c_str(), sizeof(cfg.deviceName));
        strlcpy(cfg.mdnsName,     prefs.getString("mdns",    "horloge").c_str(),           sizeof(cfg.mdnsName));

        // WiFi
        strlcpy(cfg.wifiSSID, prefs.getString("ssid",   "").c_str(), sizeof(cfg.wifiSSID));
        strlcpy(cfg.wifiPass, prefs.getString("pass",   "").c_str(), sizeof(cfg.wifiPass));

        // NTP
        strlcpy(cfg.ntpServer, prefs.getString("ntp", "pool.ntp.org").c_str(), sizeof(cfg.ntpServer));
        cfg.gmtOffsetSec      = prefs.getInt("gmt",  3600);
        cfg.daylightOffsetSec = prefs.getInt("dst",  3600);

        // Météo
        strlcpy(cfg.owmApiKey, prefs.getString("owmKey",  "").c_str(), sizeof(cfg.owmApiKey));
        strlcpy(cfg.owmCity,   prefs.getString("owmCity", "Paris,FR").c_str(), sizeof(cfg.owmCity));
        cfg.useCelsius        = prefs.getBool("celsius", true);
        cfg.weatherRefreshMs  = prefs.getUInt("wRefresh", 600000);

        // Affichage
        cfg.brightness       = prefs.getUChar("bright", 30); // 30% par défaut
        cfg.screenDurationMs = prefs.getUInt("screenDur", 8000);
        cfg.autoRotate       = prefs.getBool("autoRot", true);
        cfg.themeId          = prefs.getUChar("themeId", 0);
        // Écrans actifs (tous par défaut)
        cfg.screenEnabled[SCREEN_CLOCK]    = prefs.getBool("scr0", true);
        cfg.screenEnabled[SCREEN_WEATHER]  = prefs.getBool("scr1", true);
        cfg.screenEnabled[SCREEN_ACTIVITY] = prefs.getBool("scr2", true);
        cfg.screenEnabled[SCREEN_AGENDA]   = prefs.getBool("scr3", true);
        cfg.screenEnabled[SCREEN_MOON]     = prefs.getBool("scr4", true);
        cfg.screenEnabled[SCREEN_NOTES]    = prefs.getBool("scr5", true);
        cfg.screenEnabled[SCREEN_BIRTHDAYS]= prefs.getBool("scr6", true);
        // Notes
        // Anniversaires
        cfg.birthdayCount = prefs.getUChar("bdCount", 0);
        for (uint8_t i = 0; i < cfg.birthdayCount && i < MAX_BIRTHDAYS; i++) {
            String key = "bd" + String(i);
            String val = prefs.getString(key.c_str(), "");
            // Format stocké : "nom,jour,mois"
            int c1 = val.indexOf(',');
            int c2 = val.lastIndexOf(',');
            if (c1 > 0 && c2 > c1) {
                strlcpy(cfg.birthdays[i].name, val.substring(0,c1).c_str(), 24);
                cfg.birthdays[i].day   = val.substring(c1+1,c2).toInt();
                cfg.birthdays[i].month = val.substring(c2+1).toInt();
            }
        }
        strlcpy(cfg.notes[0], prefs.getString("note0", "").c_str(), 32);
        strlcpy(cfg.notes[1], prefs.getString("note1", "").c_str(), 32);
        strlcpy(cfg.notes[2], prefs.getString("note2", "").c_str(), 32);
        strlcpy(cfg.notes[3], prefs.getString("note3", "").c_str(), 32);

        // Calendrier (stocké en JSON dans une seule clé)
        String calJson = prefs.getString("calendar", "[]");
        _loadCalendar(calJson);

        prefs.end();
    }

    void save() {
        Preferences prefs;
        prefs.begin("horloge", false);

        prefs.putString("devName",  cfg.deviceName);
        prefs.putString("mdns",     cfg.mdnsName);
        prefs.putString("ssid",     cfg.wifiSSID);
        prefs.putString("pass",     cfg.wifiPass);
        prefs.putString("ntp",      cfg.ntpServer);
        prefs.putInt   ("gmt",      cfg.gmtOffsetSec);
        prefs.putInt   ("dst",      cfg.daylightOffsetSec);
        prefs.putString("owmKey",   cfg.owmApiKey);
        prefs.putString("owmCity",  cfg.owmCity);
        prefs.putBool  ("celsius",  cfg.useCelsius);
        prefs.putUInt  ("wRefresh", cfg.weatherRefreshMs);
        prefs.putUChar ("bright",   cfg.brightness);
        prefs.putUInt  ("screenDur",cfg.screenDurationMs);
        prefs.putBool  ("autoRot",  cfg.autoRotate);
        prefs.putUChar ("themeId",  cfg.themeId);
        prefs.putBool  ("scr0", cfg.screenEnabled[SCREEN_CLOCK]);
        prefs.putBool  ("scr1", cfg.screenEnabled[SCREEN_WEATHER]);
        prefs.putBool  ("scr2", cfg.screenEnabled[SCREEN_ACTIVITY]);
        prefs.putBool  ("scr3", cfg.screenEnabled[SCREEN_AGENDA]);
        prefs.putBool  ("scr4", cfg.screenEnabled[SCREEN_MOON]);
        prefs.putBool  ("scr5", cfg.screenEnabled[SCREEN_NOTES]);
        prefs.putBool  ("scr6", cfg.screenEnabled[SCREEN_BIRTHDAYS]);
        // Anniversaires
        prefs.putUChar("bdCount", cfg.birthdayCount);
        for (uint8_t i = 0; i < cfg.birthdayCount && i < MAX_BIRTHDAYS; i++) {
            String key = "bd" + String(i);
            String val = String(cfg.birthdays[i].name) + "," +
                         String(cfg.birthdays[i].day)  + "," +
                         String(cfg.birthdays[i].month);
            prefs.putString(key.c_str(), val);
        }
        prefs.putString("note0", cfg.notes[0]);
        prefs.putString("note1", cfg.notes[1]);
        prefs.putString("note2", cfg.notes[2]);
        prefs.putString("note3", cfg.notes[3]);

        prefs.putString("calendar", _saveCalendar());
        prefs.end();
    }

private:
    void _loadCalendar(const String& json) {
        JsonDocument doc;
        if (deserializeJson(doc, json) != DeserializationError::Ok) return;
        JsonArray arr = doc.as<JsonArray>();
        cfg.activityCount = 0;
        for (JsonObject obj : arr) {
            if (cfg.activityCount >= MAX_ACTIVITIES) break;
            Activity& a = cfg.activities[cfg.activityCount++];
            strlcpy(a.label, obj["label"] | "?",  sizeof(a.label));
            strlcpy(a.icon,  obj["icon"]  | "star",sizeof(a.icon));
            a.hour         = obj["hour"]     | 0;
            a.minute       = obj["minute"]   | 0;
            a.daysMask     = obj["days"]     | DAY_ALL;
            a.alertMinutes = obj["alert"]    | 0;
        }
    }

    String _saveCalendar() {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (uint8_t i = 0; i < cfg.activityCount; i++) {
            Activity& a = cfg.activities[i];
            JsonObject obj = arr.add<JsonObject>();
            obj["label"]  = a.label;
            obj["icon"]   = a.icon;
            obj["hour"]   = a.hour;
            obj["minute"] = a.minute;
            obj["days"]   = a.daysMask;
            obj["alert"]  = a.alertMinutes;
        }
        String out;
        serializeJson(doc, out);
        return out;
    }
};
