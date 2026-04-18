#pragma once
// ============================================================
//  ScreenClock.h  —  Horloge + date + jour
//  Layout :
//    [y=1..7]   Jour 3 lettres (gauche) | JJ/MM (droite)   TTFont
//    [y=8]      trait séparateur haut
//    [y=9..22]  Heure HH:MM             bigClockFont kerning manuel
//    [y=24]     trait séparateur bas
//    [y=25..31] Température             TTFont + ° pixel art
// ============================================================

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "../ScreenManager.h"
#include "../DisplayHelper.h"
#include "../services/NTPService.h"
#include "../services/WeatherService.h"

class ScreenClock : public BaseScreen {
public:
    explicit ScreenClock(MatrixPanel_I2S_DMA* matrix) : _mx(matrix) {}

    void onEnter() override { Theme565::refresh(_mx); _lastSec = -1; }
    void update()  override {}

    void draw() override {
        int sec = NTPService::instance().second();
        if (sec == _lastSec) return;
        _lastSec = sec;

        NTPService& ntp = NTPService::instance();
        DH::gradientBg(_mx);

        if (!ntp.isSynced()) {
            DH::useSmallFont(_mx);
            DH::printCentered(_mx, 16, "Sync NTP...", Theme565::secondary);
            _mx->flipDMABuffer();
            return;
        }

        // ── ZONE 1 : Jour + Date ───────────────────────────────
        DH::useSmallFont(_mx);
        String day  = stripAccents(String(ntp.dayName())).substring(0, 3);
        char dbuf[6];
        snprintf(dbuf, sizeof(dbuf), "%02d/%02d", ntp.mday(), ntp.month());
        String date = String(dbuf);

        DH::print(_mx, 1, 4, day, Theme565::accent);
        DH::print(_mx, ttfRightX(date) + 1, 4, date, Theme565::secondary);

        // ── Trait séparateur haut ──────────────────────────────
        DH::hline(_mx, 6);

        // ── ZONE 2 : Heure avec kerning manuel ─────────────────
        // xAdvance réduit pour le '1' (5px visible vs 10px standard)
        // et le '7' (9px). Le ':' reçoit 5px.
        DH::useBigFont(_mx);
        _mx->setTextColor(Theme565::primary);

        String hh = ntp.timeStr().substring(0, 2);  // "HH"
        String mm  = ntp.timeStr().substring(3, 5); // "MM"
        int yBig  = 21; // baseline bigClockFont

        // Mesure manuelle de la largeur totale
        int totalW = _charW(hh[0]) + _charW(hh[1])   // HH
                   + 5                                  // ":"
                   + _charW(mm[0])  + _charW(mm[1]);   // MM
        int x = (64 - totalW) / 2;

        // HH
        _mx->setCursor(x, yBig); _mx->print(hh[0]); x += _charW(hh[0]);
        _mx->setCursor(x, yBig); _mx->print(hh[1]); x += _charW(hh[1]);

        // ":"  clignotant
        uint16_t cSep = (sec % 2 == 0) ? Theme565::primary : Theme565::dim;
        _mx->setTextColor(cSep);
        _mx->setCursor(x, yBig); _mx->print(':'); x += 5;
        _mx->setTextColor(Theme565::primary);

        // MM
        _mx->setCursor(x, yBig); _mx->print(mm[0]); x += _charW(mm[0]);
        _mx->setCursor(x, yBig); _mx->print(mm[1]);

        // ── Trait séparateur bas (1px plus bas) ────────────────
        DH::hline(_mx, 24);

        // ── ZONE 3 : Température ───────────────────────────────
        DH::useSmallFont(_mx);
        WeatherData& wd = WeatherService::instance().data;
        if (wd.valid) {
            float temp    = wd.tempCurrent;
            bool  celsius = ConfigManager::instance().cfg.useCelsius;
            char tbuf[8];
            snprintf(tbuf, sizeof(tbuf), "%d", (int)round(temp));
            String tStr = String(tbuf);
            uint16_t cTemp = _tempColor(temp);

            // Largeur totale : chiffres + 4px(°) + 1px + 6px(C/F)
            // totalTW = chiffres + °(4px) + C/F(5px), tout collé
            int tW = ttfWidth(tStr);
            int totalTW = tW + 4 + 5;
            int xT = (64 - totalTW) / 2;

            DH::print(_mx, xT, 31, tStr, cTemp);
            int xDeg = xT + tW; // juste après le dernier chiffre, sans espace
            _drawDegree(xDeg, 25, cTemp);
            _mx->setCursor(xDeg + 4, 31);
            _mx->setTextColor(cTemp);
            _mx->print(celsius ? 'C' : 'F');
        } else {
            DH::printCentered(_mx, 30, "Meteo indispo", Theme565::dim);
        }

        _mx->flipDMABuffer();
    }

private:
    MatrixPanel_I2S_DMA* _mx;
    int _lastSec = -1;

    // Avance horizontale par caractère selon métriques bigClockFont
    // xOffset=0 → pas de marge naturelle → on ajoute 1px
    // xOffset=2 → marge large → on réduit
    int _charW(char c) {
        switch(c) {
            case '1': return 7;   // width=5, xOffset=2 → espace mort important
            case '7': return 9;   // width=9, xOffset=0 → on réduit légèrement
            case '8': return 11;  // width=10, xOffset=0 → colle, on ajoute 1px
            default:  return 10;
        }
    }

    // Symbole ° pixel art (carré creux 4×4px)
    void _drawDegree(int x, int y, uint16_t c) {
        _mx->drawPixel(x+1, y,   c);
        _mx->drawPixel(x+2, y,   c);
        _mx->drawPixel(x,   y+1, c);
        _mx->drawPixel(x+3, y+1, c);
        _mx->drawPixel(x,   y+2, c);
        _mx->drawPixel(x+3, y+2, c);
        _mx->drawPixel(x+1, y+3, c);
        _mx->drawPixel(x+2, y+3, c);
    }

    uint16_t _tempColor(float t) {
        if (t < 0)  return _mx->color565(150, 200, 255);
        if (t < 10) return _mx->color565(100, 180, 255);
        if (t < 18) return Theme565::accent;
        if (t < 25) return Theme565::primary;
        if (t < 30) return _mx->color565(255, 160, 0);
        return              _mx->color565(255, 60,  0);
    }
};
