#pragma once
// ============================================================
//  CalendarService.h  —  Calendrier + alertes anticipées
// ============================================================

#include <Arduino.h>
#include "Config.h"
#include "services/NTPService.h"

// État d'une activité par rapport à maintenant
enum ActivityState : uint8_t {
    ACT_FUTURE,   // dans le futur, pas encore en alerte
    ACT_ALERT,    // dans la fenêtre d'alerte (il faut partir !)
    ACT_IMMINENT, // moins de 5 min
    ACT_PAST      // passée
};

struct ActivityRef {
    const Activity* ptr      = nullptr;
    int             minUntil = -1;   // minutes jusqu'à l'heure de l'activité
    int             minUntilAlert = -1; // minutes jusqu'au départ (heure - alerte)
    ActivityState   state    = ACT_FUTURE;
};

class CalendarService {
public:
    static CalendarService& instance() {
        static CalendarService inst;
        return inst;
    }

    // Prochaine activité non passée (ou en alerte)
    ActivityRef getNext() {
        AppConfig& cfg  = ConfigManager::instance().cfg;
        NTPService& ntp = NTPService::instance();

        if (!ntp.isSynced() || cfg.activityCount == 0) return {};

        uint8_t wdayBit = (1 << ntp.wday());
        int nowMin = ntp.hour() * 60 + ntp.minute();

        ActivityRef best;
        best.minUntil = INT16_MAX;

        for (uint8_t i = 0; i < cfg.activityCount; i++) {
            Activity& a = cfg.activities[i];
            if (!(a.daysMask & wdayBit)) continue;

            int actMin   = a.hour * 60 + a.minute;
            int alertMin = actMin - a.alertMinutes; // moment où il faut partir

            // Minutes jusqu'à l'activité elle-même
            int diffAct = actMin - nowMin;
            if (diffAct < 0) diffAct += 24 * 60;

            // Ignorer les activités passées (> 12h derrière)
            int rawDiff = actMin - nowMin;
            if (rawDiff < 0) continue; // heure passée → ignorer

            if (diffAct < best.minUntil) {
                best.minUntil      = diffAct;
                best.minUntilAlert = alertMin - nowMin; // négatif = départ dépassé
                best.ptr           = &a;
                best.state         = _computeState(nowMin, actMin, a.alertMinutes);
            }
        }

        if (best.ptr == nullptr || best.minUntil == INT16_MAX) return {};
        return best;
    }

    // Toutes les activités du jour, triées, avec leur état
    struct DaySchedule {
        const Activity* items[MAX_ACTIVITIES];
        ActivityState   states[MAX_ACTIVITIES];
        int             minUntil[MAX_ACTIVITIES]; // minutes jusqu'à l'activité
        uint8_t         count = 0;
    };

    DaySchedule getToday() {
        AppConfig& cfg  = ConfigManager::instance().cfg;
        NTPService& ntp = NTPService::instance();
        DaySchedule result;
        if (!ntp.isSynced()) return result;

        uint8_t wdayBit = (1 << ntp.wday());
        int nowMin = ntp.hour() * 60 + ntp.minute();

        for (uint8_t i = 0; i < cfg.activityCount; i++) {
            Activity& a = cfg.activities[i];
            if (!(a.daysMask & wdayBit)) continue;
            int actMin = a.hour * 60 + a.minute;
            int diff   = actMin - nowMin;
            result.items[result.count]    = &a;
            result.states[result.count]   = _computeState(nowMin, actMin, a.alertMinutes);
            result.minUntil[result.count] = diff;
            result.count++;
        }

        // Tri par heure croissante
        for (uint8_t i = 0; i < result.count; i++)
            for (uint8_t j = i+1; j < result.count; j++) {
                int ta = result.items[i]->hour * 60 + result.items[i]->minute;
                int tb = result.items[j]->hour * 60 + result.items[j]->minute;
                if (tb < ta) {
                    std::swap(result.items[i],   result.items[j]);
                    std::swap(result.states[i],  result.states[j]);
                    std::swap(result.minUntil[i],result.minUntil[j]);
                }
            }
        return result;
    }

    // Formate un compte à rebours lisible
    static String formatCountdown(int minutes, bool forAlert = false) {
        String prefix = forAlert ? "Partir " : "";
        if (minutes <= 0)  return forAlert ? "Partir maintenant !" : "Maintenant !";
        if (minutes < 60) {
            char buf[20];
            snprintf(buf, sizeof(buf), "%sdans %dmin", prefix.c_str(), minutes);
            return String(buf);
        }
        char buf[20];
        snprintf(buf, sizeof(buf), "%sdans %dh%02d", prefix.c_str(), minutes/60, minutes%60);
        return String(buf);
    }

    // Formate l'heure de départ conseillée
    static String formatDepartureTime(const Activity* a) {
        if (!a || a->alertMinutes == 0) return "";
        int depH = a->hour;
        int depM = a->minute - a->alertMinutes;
        while (depM < 0) { depM += 60; depH--; }
        if (depH < 0) depH += 24;
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", depH, depM);
        return String(buf);
    }

private:
    ActivityState _computeState(int nowMin, int actMin, uint8_t alertMins) {
        int diff = actMin - nowMin;
        if (diff < -60)     return ACT_PAST;
        if (diff < 0)       return ACT_PAST;   // heure passée → PAST immédiatement
        if (diff < 5)       return ACT_IMMINENT;
        if (alertMins > 0 && diff <= (int)alertMins) return ACT_ALERT;
        return ACT_FUTURE;
    }
};
