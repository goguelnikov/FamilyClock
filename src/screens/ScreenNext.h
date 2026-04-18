#pragma once
// ============================================================
//  ScreenNext.h  —  Prochain RDV
//  Layout :
//    y=4   "Prochain RDV"
//    y=6   trait
//    y=12  HH:MM  >depart  (sur 1 ligne)
//    y=18  description (tronquée)
//    y=26  countdown centré
// ============================================================

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "../ScreenManager.h"
#include "../DisplayHelper.h"
#include "../services/CalendarService.h"
#include "../services/NTPService.h"

class ScreenNext : public BaseScreen {
public:
    explicit ScreenNext(MatrixPanel_I2S_DMA* matrix) : _mx(matrix) {}

    void onEnter() override {
        Theme565::refresh(_mx);
        _dirty   = true;
        _lastMin = -1;
        _lastSec = -1;
    }

    void update() override {
        int m = NTPService::instance().minute();
        int s = NTPService::instance().second();
        ActivityRef next = CalendarService::instance().getNext();
        bool urgent = next.ptr &&
                      (next.state == ACT_ALERT || next.state == ACT_IMMINENT);
        if (m != _lastMin || (urgent && s != _lastSec)) {
            _lastMin = m; _lastSec = s; _dirty = true;
        }
    }

    void draw() override {
        if (!_dirty) return;
        _dirty = false;

        ActivityRef next = CalendarService::instance().getNext();

        // Fond selon urgence
        if (next.ptr && next.state == ACT_IMMINENT) {
            _flashBg(_mx->color565(60, 5, 5));
        } else if (next.ptr && next.state == ACT_ALERT) {
            _solidBg(_mx->color565(40, 20, 0));
        } else {
            DH::gradientBg(_mx);
        }

        DH::useSmallFont(_mx);

        // ── Titre ────────────────────────────────────────────
        // Titre toujours fixe
        DH::print(_mx, 1, 4, "Prochain RDV", Theme565::accent);
        DH::hline(_mx, 6);

        if (!next.ptr) {
            DH::printCentered(_mx, 18, "Bonne journee", Theme565::dim);
            _mx->flipDMABuffer();
            return;
        }

        // Couleurs identiques à ScreenAgenda
        uint16_t cTime = next.state == ACT_IMMINENT ? _mx->color565(255, 60,  0)
                       : next.state == ACT_ALERT    ? _mx->color565(255, 200, 0)
                       : Theme565::accent;
        uint16_t cDep  = next.state == ACT_IMMINENT ? _mx->color565(255, 60,  0)
                       : next.state == ACT_ALERT    ? _mx->color565(255, 180, 0)
                       : Theme565::dim;
        uint16_t cDesc = next.state == ACT_IMMINENT ? _mx->color565(255, 60,  0)
                       : next.state == ACT_ALERT    ? _mx->color565(255, 200, 0)
                       : Theme565::secondary;

        // ── Heure + départ sur 1 ligne ────────────────────────
        char hbuf[6];
        snprintf(hbuf, sizeof(hbuf), "%02d:%02d",
                 next.ptr->hour, next.ptr->minute);
        DH::print(_mx, 1, 12, String(hbuf), cTime);

        if (next.ptr->alertMinutes > 0) {
            String dep = ">" + CalendarService::formatDepartureTime(next.ptr);
            DH::print(_mx, 24, 12, dep, cDep);
        }

        // ── Description ──────────────────────────────────────
        String label = stripAccents(String(next.ptr->label));
        label = ttfTrunc(label, 62);
        DH::print(_mx, 1, 18, label, cDesc);

        // ── Compte à rebours centré ───────────────────────────
        String cd;
        uint16_t cCd;
        if (next.state == ACT_IMMINENT) {
            cd  = "Maintenant!";
            cCd = _mx->color565(255, 60, 0);
        } else if (next.state == ACT_ALERT) {
            cd  = "Bientot";
            cCd = _mx->color565(255, 200, 0);
        } else {
            cd  = stripAccents(CalendarService::formatCountdown(next.minUntil));
            cCd = Theme565::secondary;
        }
        DH::printCentered(_mx, 26, cd, cCd);

        _mx->flipDMABuffer();
    }

private:
    MatrixPanel_I2S_DMA* _mx;
    bool _dirty   = true;
    int  _lastMin = -1;
    int  _lastSec = -1;

    void _solidBg(uint16_t color) {
        for (int y = 0; y < 32; y++)
            _mx->drawFastHLine(0, y, 64, color);
    }
    void _flashBg(uint16_t color) {
        bool flash = (NTPService::instance().second() % 2 == 0);
        _solidBg(flash ? color : _mx->color565(5, 5, 5));
    }
};
