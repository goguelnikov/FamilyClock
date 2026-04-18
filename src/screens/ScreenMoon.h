#pragma once
// ============================================================
//  ScreenMoon.h  —  Phases de la lune
//  Calcul astronomique simplifié (précision ~1 jour)
//  Layout :
//    [y=1..4]  "LUNE" (gauche) | date prochaine phase (droite)
//    [y=5]     trait
//    [y=6..31] dessin lune centré + nom phase + jours restants
// ============================================================

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "../ScreenManager.h"
#include "../DisplayHelper.h"
#include "../services/NTPService.h"
#include <math.h>

// Noms des 8 phases
// Format "ligne1|ligne2" — chaque partie max 28px avec TTFont
static const char* PHASE_NAMES[] = {
    "Nouv.|lune",       // Nouvelle lune
    "1er|crois.",       // Premier croissant
    "1er|quart.",       // Premier quartier
    "Gibb.|crois.",     // Gibbeuse croissante
    "Pleine|lune",      // Pleine lune
    "Gibb.|decr.",      // Gibbeuse décroissante
    "Dern.|quart.",     // Dernier quartier
    "Dern.|crois."      // Dernier croissant
};

class ScreenMoon : public BaseScreen {
public:
    explicit ScreenMoon(MatrixPanel_I2S_DMA* matrix) : _mx(matrix) {}

    void onEnter() override { Theme565::refresh(_mx); _dirty = true; }
    void update()  override {
        // Recalculer une fois par heure
        if (millis() - _lastCalc > 3600000UL) { _dirty = true; _lastCalc = millis(); }
    }

    void draw() override {
        if (!_dirty) return;
        _dirty = false;
        _lastCalc = millis();

        DH::gradientBg(_mx);
        DH::useSmallFont(_mx);

        NTPService& ntp = NTPService::instance();
        if (!ntp.isSynced()) {
            DH::printCentered(_mx, 16, "Sync NTP...", Theme565::dim);
            _mx->flipDMABuffer();
            return;
        }

        // ── Calcul de la phase ───────────────────────────────
        // Âge de la lune en jours depuis nouvelle lune connue
        // Référence : nouvelle lune le 6 janvier 2000 à 18h14 UTC
        // Période synodique : 29.53059 jours
        struct tm t; ntp.now(t);
        // Jours juliens depuis J2000.0
        int y = t.tm_year + 1900;
        int mo = t.tm_mon + 1;
        int d  = t.tm_mday;
        // Formule simplifiée pour le jour julien
        long jd = 367L*y - (7*(y+(mo+9)/12))/4 + (275*mo)/9 + d + 1721013L;
        float jdf = (float)jd + t.tm_hour/24.0f - 0.5f;
        float daysSinceRef = jdf - 2451550.1f; // J2000 + offset nouvelle lune ref
        float age = fmod(daysSinceRef, 29.53059f);
        if (age < 0) age += 29.53059f;

        // Phase 0..7
        float phaseF = age / 29.53059f; // 0..1
        uint8_t phaseIdx = (uint8_t)(phaseF * 8.0f + 0.5f) % 8;

        // Jours jusqu'à prochaine pleine lune (phase=0.5 → jour 14.76)
        float daysToFull = fmod(14.765f - age + 29.53059f, 29.53059f);
        float daysToNew  = fmod(29.53059f - age, 29.53059f);

        // Illumination (0..100%)
        uint8_t illum = (uint8_t)(50.0f * (1.0f - cos(phaseF * 2.0f * 3.14159f)) + 0.5f);

        // ── En-tête ─────────────────────────────────────────
        DH::print(_mx, 1, 4, "LUNE", Theme565::accent);
        // Jours vers prochaine pleine ou nouvelle
        char nbuf[12];
        if (phaseF < 0.5f)
            snprintf(nbuf, sizeof(nbuf), "PL: j-%d", (int)daysToFull);
        else
            snprintf(nbuf, sizeof(nbuf), "NL: j-%d", (int)daysToNew);
        DH::print(_mx, ttfRightX(String(nbuf)) + 1, 4, String(nbuf), Theme565::secondary);
        DH::hline(_mx, 6);

        // ── Dessin de la lune (cercle 14px de diamètre, centré) ──
        // Centre : x=16, y=19 (à gauche pour laisser place au texte)
        int cx = 14, cy = 19, r = 11;
        uint16_t cMoon  = _mx->color565(220, 210, 160); // jaune pâle
        uint16_t cShadow = Theme565::bg;

        // Dessiner le cercle plein
        // Rendu lune : LED allumée = surface éclairée, LED éteinte = ombre
        // k = -cos(phaseF*2π) : k=-1 (nouvelle=sombre), k=+1 (pleine=éclairée)
        // termX = cx - k*halfW
        //   phaseF=0   (nouvelle) k=-1 → termX=xRight → rien à droite → tout sombre ✓
        //   phaseF=0.25 (1er qrt) k=0  → termX=cx     → moitié droite éclairée ✓
        //   phaseF=0.5  (pleine)  k=+1 → termX=xLeft  → tout éclairé ✓
        float k = -cos(phaseF * 2.0f * 3.14159f);

        for (int py = cy - r; py <= cy + r; py++) {
            int halfW = (int)sqrt((float)(r*r - (py-cy)*(py-cy)));
            if (halfW <= 0) continue;
            int xLeft  = cx - halfW;
            int xRight = cx + halfW;
            int litL, litR;
            if (phaseF <= 0.5f) {
                // Croissant : termX = cx - k*halfW, éclairé à droite
                int tX = (int)(cx - k * halfW);
                litL = max(xLeft, tX);
                litR = xRight;
            } else {
                // Décroissant : termX = cx + k*halfW, éclairé à gauche
                int tX = (int)(cx + k * halfW);
                litL = xLeft;
                litR = min(xRight, tX);
            }
            if (litL <= litR)
                _mx->drawFastHLine(litL, py, litR - litL + 1, cMoon);
        }
        _mx->drawCircle(cx, cy, r, _mx->color565(40, 40, 30));

        // ── Texte à droite de la lune ─────────────────────────
        // Nom de phase : split sur '|' → 2 lignes garanties < 28px
        String pname = String(PHASE_NAMES[phaseIdx]);
        int sepIdx = pname.indexOf('|');
        String line1 = sepIdx > 0 ? pname.substring(0, sepIdx) : pname;
        String line2 = sepIdx > 0 ? pname.substring(sepIdx + 1) : "";

        DH::print(_mx, 30, 11, line1, Theme565::primary);
        if (line2.length() > 0)
            DH::print(_mx, 30, 17, line2, Theme565::primary);

        // Illumination
        char ibuf[8];
        snprintf(ibuf, sizeof(ibuf), "%d%%", illum);
        DH::print(_mx, 30, 24, String(ibuf), Theme565::accent);

        // Age en jours
        char abuf[10];
        snprintf(abuf, sizeof(abuf), "j%d/%d", (int)age, 29);
        DH::print(_mx, 30, 30, String(abuf), Theme565::dim);

        _mx->flipDMABuffer();
    }

private:
    MatrixPanel_I2S_DMA* _mx;
    bool     _dirty    = true;
    uint32_t _lastCalc = 0;
};
