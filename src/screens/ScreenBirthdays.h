#pragma once
// ============================================================
//  ScreenBirthdays.h  —  4 prochains anniversaires
//  Layout :
//    y=4   "ANNIV"
//    y=6   trait
//    y=12  Prenom              Xj
//    y=18  Prenom              Xj
//    y=24  Prenom              Xj
//    y=30  Prenom              Xj
// ============================================================

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "../ScreenManager.h"
#include "../DisplayHelper.h"
#include "../services/NTPService.h"
#include "../Config.h"

class ScreenBirthdays : public BaseScreen {
public:
    explicit ScreenBirthdays(MatrixPanel_I2S_DMA* matrix) : _mx(matrix) {}

    void onEnter() override {
        Theme565::refresh(_mx);
        _dirty   = true;
        _lastDay = -1;
    }

    void update() override {
        int d = NTPService::instance().mday();
        if (d != _lastDay) { _lastDay = d; _dirty = true; }
    }

    void draw() override {
        if (!_dirty) return;
        _dirty = false;

        DH::gradientBg(_mx);
        DH::useSmallFont(_mx);

        DH::print(_mx, 1, 4, "Anniversaires", Theme565::accent);
        DH::hline(_mx, 6);

        if (!NTPService::instance().isSynced()) {
            DH::printCentered(_mx, 18, "Sync NTP...", Theme565::dim);
            _mx->flipDMABuffer();
            return;
        }

        AppConfig& cfg = ConfigManager::instance().cfg;
        if (cfg.birthdayCount == 0) {
            DH::printCentered(_mx, 18, "Aucun anniv.", Theme565::dim);
            _mx->flipDMABuffer();
            return;
        }

        // Calculer le nombre de jours jusqu'à chaque anniversaire
        int today_d = NTPService::instance().mday();
        int today_m = NTPService::instance().month();
        int today_y = NTPService::instance().year();

        struct Entry { int days; uint8_t idx; };
        Entry entries[MAX_BIRTHDAYS];
        uint8_t n = 0;

        for (uint8_t i = 0; i < cfg.birthdayCount; i++) {
            Birthday& b = cfg.birthdays[i];
            if (b.day == 0 || b.month == 0) continue;
            int days = _daysUntil(today_d, today_m, today_y, b.day, b.month);
            entries[n++] = {days, i};
        }

        // Tri par jours croissants (tri à bulles, liste courte)
        for (uint8_t i = 0; i < n-1; i++)
            for (uint8_t j = 0; j < n-i-1; j++)
                if (entries[j].days > entries[j+1].days) {
                    Entry tmp = entries[j]; entries[j] = entries[j+1]; entries[j+1] = tmp;
                }

        const int yLines[4] = {12, 18, 24, 30};
        uint8_t shown = min(n, (uint8_t)4);

        for (uint8_t s = 0; s < shown; s++) {
            Birthday& b  = cfg.birthdays[entries[s].idx];
            int days     = entries[s].days;
            bool isToday = (days == 0);

            // Vert vif pour l'anniversaire du jour
            uint16_t cBday = _mx->color565(0, 0, 200); // vert avec swap G↔B panneau
            uint16_t cName = isToday ? cBday
                           : s == 0  ? Theme565::accent
                           :           Theme565::secondary;

            // Date JJ/MM
            char dbuf[6];
            snprintf(dbuf, sizeof(dbuf), "%02d/%02d", b.day, b.month);
            DH::print(_mx, 1, yLines[s], String(dbuf), isToday ? cBday : Theme565::dim);

            // Prénom tronqué après la date (x=29)
            String name = stripAccents(String(b.name));
            name = ttfTrunc(name, 35);
            DH::print(_mx, 29, yLines[s], name, cName);
        }

        _mx->flipDMABuffer();
    }

private:
    MatrixPanel_I2S_DMA* _mx;
    bool _dirty   = true;
    int  _lastDay = -1;

    // Nombre de jours jusqu'au prochain anniversaire (0 = aujourd'hui)
    int _daysUntil(int td, int tm, int ty, int bd, int bm) {
        // Date anniversaire cette année
        int days = _dayOfYear(bd, bm) - _dayOfYear(td, tm);
        if (days < 0) {
            // Passé cette année → l'année prochaine
            int daysInYear = _isLeap(ty) ? 366 : 365;
            days += daysInYear;
        }
        return days;
    }

    int _dayOfYear(int d, int m) {
        const int months[] = {0,31,59,90,120,151,181,212,243,273,304,334};
        return months[m-1] + d;
    }

    bool _isLeap(int y) {
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    }
};
