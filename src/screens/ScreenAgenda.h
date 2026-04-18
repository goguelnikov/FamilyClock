#pragma once
// ============================================================
//  ScreenAgenda.h  —  Programme du jour
//  Layout :
//    y=4   Jour | Date
//    y=6   trait
//    y=12  HH:MM Label (RDV 1)
//    y=18  HH:MM Label (RDV 2)
//    y=24  HH:MM Label (RDV 3)
//    y=30  HH:MM Label (RDV 4)
// ============================================================

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "../ScreenManager.h"
#include "../DisplayHelper.h"
#include "../services/CalendarService.h"
#include "../services/NTPService.h"

class ScreenAgenda : public BaseScreen {
public:
    explicit ScreenAgenda(MatrixPanel_I2S_DMA* matrix) : _mx(matrix) {}

    void onEnter() override {
        Theme565::refresh(_mx);
        _dirty   = true;
        _lastMin = -1;
    }

    void update() override {
        int m = NTPService::instance().minute();
        int s = NTPService::instance().second();
        if (m != _lastMin) { _lastMin = m; _dirty = true; }

        // Redessiner à la seconde si alerte active
        auto sched = CalendarService::instance().getToday();
        for (uint8_t i = 0; i < sched.count; i++) {
            if (sched.states[i] == ACT_ALERT || sched.states[i] == ACT_IMMINENT) {
                static int _lastSec = -1;
                if (s != _lastSec) { _lastSec = s; _dirty = true; }
                break;
            }
        }
    }

    void draw() override {
        if (!_dirty) return;
        _dirty = false;

        DH::gradientBg(_mx);
        DH::useSmallFont(_mx);

        // ── En-tête ──────────────────────────────────────────
        NTPService& ntp = NTPService::instance();
        String dayStr = stripAccents(String(ntp.dayName())).substring(0, 3);
        char dbuf[9];
        snprintf(dbuf, sizeof(dbuf), "%02d/%02d/%02d",
                 ntp.mday(), ntp.month(), ntp.year() % 100);
        String dateStr = String(dbuf);
        DH::print(_mx, 1, 4, dayStr, Theme565::accent);
        DH::print(_mx, ttfRightX(dateStr) + 1, 4, dateStr, Theme565::secondary);
        DH::hline(_mx, 6);

        // ── 4 prochains RDV non passés ───────────────────────
        auto sched = CalendarService::instance().getToday();

        // Collecter les RDV non passés (max 4)
        uint8_t futureIdx[4];
        uint8_t count = 0;
        for (uint8_t i = 0; i < sched.count && count < 4; i++) {
            if (sched.states[i] != ACT_PAST)
                futureIdx[count++] = i;
        }

        if (count == 0) {
            DH::printCentered(_mx, 18, "Rien a venir", Theme565::dim);
            _mx->flipDMABuffer();
            return;
        }

        const int yLines[4] = {12, 18, 24, 30};

        for (uint8_t slot = 0; slot < count; slot++) {
            uint8_t i         = futureIdx[slot];
            const Activity* a = sched.items[i];
            ActivityState   s = sched.states[i];

            // Couleur selon état
            uint16_t cLine = s == ACT_IMMINENT ? _mx->color565(255, 60,  0)
                           : s == ACT_ALERT    ? _mx->color565(255, 200, 0)
                           : slot == 0         ? Theme565::accent
                           :                    Theme565::secondary;

            // HH:MM
            char hbuf[6];
            snprintf(hbuf, sizeof(hbuf), "%02d:%02d", a->hour, a->minute);
            DH::print(_mx, 1, yLines[slot], String(hbuf), cLine);

            // Label tronqué après HH:MM (x=25)
            String label = stripAccents(String(a->label));
            label = ttfTrunc(label, 39);
            DH::print(_mx, 25, yLines[slot], label, cLine);
        }

        _mx->flipDMABuffer();
    }

private:
    MatrixPanel_I2S_DMA* _mx;
    bool _dirty   = true;
    int  _lastMin = -1;
};
