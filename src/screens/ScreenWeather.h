#pragma once
// ============================================================
//  ScreenWeather.h
//  Layout :
//    [y=1..4]  "METEO" (gauche)  |  temp+° (droite)   TTFont
//    [y=6]     trait
//    [y=7..22] icône météo (gauche 18px) | vent + pression (droite)
//    [y=23]    trait
//    [y=24..31] description                              TTFont
// ============================================================

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "../ScreenManager.h"
#include "../DisplayHelper.h"
#include "../services/WeatherService.h"

class ScreenWeather : public BaseScreen {
public:
    explicit ScreenWeather(MatrixPanel_I2S_DMA* matrix) : _mx(matrix) {}

    void onEnter() override { Theme565::refresh(_mx); _dirty = true; }

    void update() override {
        static uint32_t lastFetch = 0;
        if (WeatherService::instance().data.fetchedAt != lastFetch) {
            lastFetch = WeatherService::instance().data.fetchedAt;
            _dirty = true;
        }
    }

    void draw() override {
        if (!_dirty) return;
        _dirty = false;

        DH::gradientBg(_mx);
        DH::useSmallFont(_mx);

        WeatherData& wd = WeatherService::instance().data;

        // ── ZONE 1 : Titre + Température (au-dessus du trait) ──
        // "METEO" à gauche, température à droite
        DH::print(_mx, 1, 4, "METEO", Theme565::accent);

        if (wd.valid) {
            float temp    = wd.tempCurrent;
            bool  celsius = ConfigManager::instance().cfg.useCelsius;
            char tbuf[8];
            snprintf(tbuf, sizeof(tbuf), "%d", (int)round(temp));
            String tStr = String(tbuf);
            uint16_t cTemp = _tempColor(temp);

            // Temp alignée à droite : largeur réelle + °(4px) + espace(1px) + C/F(5px)
            int tW = ttfWidth(tStr);
            int totalTW = tW + 4 + 5; // chiffres + °(4px) + C/F(5px)
            int xT = max(0, 64 - totalTW);
            DH::print(_mx, xT, 4, tStr, cTemp);
            int xDeg = xT + tW; // collé au chiffre
            _drawDegree(xDeg, 0, cTemp);
            _mx->setTextColor(cTemp);
            _mx->setCursor(xDeg + 4, 4);
            _mx->print(celsius ? 'C' : 'F');
        }

        // ── Trait haut ──────────────────────────────────────────
        DH::hline(_mx, 6);

        if (!wd.valid) {
            DH::printCentered(_mx, 18, "Indisponible", Theme565::dim);
            _mx->flipDMABuffer();
            return;
        }

        // ── ZONE 2 : Icône (gauche) + Vent/Pression (droite) ───
        _drawWeatherIcon(1, 7, WeatherService::instance().getIcon());

        // Vent — aligné à droite
        char wbuf[12];
        snprintf(wbuf, sizeof(wbuf), "%.1fm/s", wd.windSpeed);
        { String ws = String(wbuf);
          DH::print(_mx, ttfRightX(ws), 13, ws, Theme565::secondary); }

        // Pression — alignée à droite
        char pbuf[10];
        snprintf(pbuf, sizeof(pbuf), "%dhPa", wd.pressure);
        { String ps = String(pbuf);
          DH::print(_mx, ttfRightX(ps), 20, ps, Theme565::secondary); }

        // ── Trait bas ──────────────────────────────────────────
        DH::hline(_mx, 24);

        // ── ZONE 3 : Description (y=26..31, 2 lignes si besoin) ───
        String desc = stripAccents(wd.description);
        desc[0] = toupper(desc[0]);
        // Essayer sur 1 ligne (max 10 chars @ 6px = 60px)
        // Si trop long, couper en 2 mots sur 2 lignes
        // 1 seule ligne, tronquée selon la vraie largeur de police
        desc = ttfTrunc(desc, 62);
        DH::printCentered(_mx, 30, desc, Theme565::secondary);

        _mx->flipDMABuffer();
    }

private:
    MatrixPanel_I2S_DMA* _mx;
    bool _dirty = true;

    void _drawDegree(int x, int y, uint16_t c) {
        _mx->drawPixel(x+1, y,   c); _mx->drawPixel(x+2, y,   c);
        _mx->drawPixel(x,   y+1, c); _mx->drawPixel(x+3, y+1, c);
        _mx->drawPixel(x,   y+2, c); _mx->drawPixel(x+3, y+2, c);
        _mx->drawPixel(x+1, y+3, c); _mx->drawPixel(x+2, y+3, c);
    }

    uint16_t _tempColor(float t) {
        if (t < 0)  return _mx->color565(150, 200, 255);
        if (t < 10) return _mx->color565(100, 180, 255);
        if (t < 18) return Theme565::accent;
        if (t < 25) return Theme565::primary;
        if (t < 30) return _mx->color565(255, 160, 0);
        return              _mx->color565(255, 60,  0);
    }

    void _drawWeatherIcon(int x, int y, WeatherIcon icon) {
        uint16_t c1 = Theme565::primary;
        uint16_t c2 = Theme565::accent;
        uint16_t cw = _mx->color565(200, 210, 220);
        switch(icon) {
            case WI_CLEAR_DAY:
                _mx->fillCircle(x+8, y+7, 4, c2);
                for (int a = 0; a < 8; a++) {
                    float r = a * 3.14159f / 4;
                    _mx->drawLine(x+8+cos(r)*5, y+7+sin(r)*5,
                                  x+8+cos(r)*7, y+7+sin(r)*7, c2);
                } break;
            case WI_CLEAR_NIGHT:
                _mx->fillCircle(x+8, y+7, 5, c1);
                _mx->fillCircle(x+10,y+5, 4, _mx->color565(5,5,20)); break;
            case WI_FEW_CLOUDS:
                _mx->fillCircle(x+7, y+5, 3, c2);
                _mx->fillCircle(x+6, y+9, 3, cw);
                _mx->fillCircle(x+10,y+8, 4, cw);
                _mx->fillRect(x+4, y+9, 9, 3, cw); break;
            case WI_CLOUDS:
                _mx->fillCircle(x+5,  y+9, 4, cw);
                _mx->fillCircle(x+10, y+7, 5, cw);
                _mx->fillCircle(x+14, y+9, 3, cw);
                _mx->fillRect(x+3, y+9, 13, 4, cw); break;
            case WI_SHOWER: case WI_RAIN: {
                _mx->fillCircle(x+5,  y+6, 3, cw);
                _mx->fillCircle(x+10, y+4, 4, cw);
                _mx->fillRect(x+3, y+6, 11, 3, cw);
                uint16_t cb = _mx->color565(80, 150, 255);
                for (int i = 0; i < 4; i++)
                    _mx->drawLine(x+4+i*3, y+11, x+3+i*3, y+14, cb);
                break; }
            case WI_SNOW: {
                _mx->fillCircle(x+5,  y+6, 3, cw);
                _mx->fillCircle(x+10, y+4, 4, cw);
                _mx->fillRect(x+3, y+6, 11, 3, cw);
                uint16_t cs = _mx->color565(220, 235, 255);
                for (int i = 0; i < 4; i++)
                    _mx->drawPixel(x+4+i*3, y+12, cs);
                break; }
            case WI_THUNDER: {
                _mx->fillCircle(x+5,  y+5, 3, cw);
                _mx->fillCircle(x+10, y+3, 4, cw);
                _mx->fillRect(x+3, y+5, 11, 3, cw);
                uint16_t cy = _mx->color565(255, 255, 0);
                _mx->drawLine(x+10, y+10, x+7,  y+14, cy);
                _mx->drawLine(x+7,  y+14, x+10, y+14, cy);
                _mx->drawLine(x+10, y+14, x+7,  y+18, cy);
                break; }
            case WI_MIST:
                for (int row = 0; row < 4; row++)
                    _mx->drawFastHLine(x+1, y+4+row*3, 14, Theme565::dim);
                break;
            default:
                _mx->drawRect(x+4, y+4, 8, 8, Theme565::dim); break;
        }
    }
};
