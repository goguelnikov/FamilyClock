#pragma once
// ============================================================
//  DisplayHelper.h  —  Police, thème, utilitaires d'affichage
// ============================================================

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "smallClockFont.h"   // TTFont : police small custom
#include "bigClockFont.h"    // bigClockFont : police grande pour l'heure
#include "Config.h"

// ============================================================
//  Conversion accents → ASCII (pour affichage sur matrice)
// ============================================================
static String stripAccents(const String& in) {
    String out;
    out.reserve(in.length());
    const char* s = in.c_str();
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c < 128) {
            out += (char)c;
            s++;
            continue;
        }
        // Séquences UTF-8 2 octets
        if ((c & 0xE0) == 0xC0 && *(s+1)) {
            uint16_t cp = ((c & 0x1F) << 6) | (*(s+1) & 0x3F);
            s += 2;
            switch(cp) {
                case 0xE0: case 0xE1: case 0xE2: case 0xE3:
                case 0xE4: case 0xE5: out += 'a'; break;
                case 0xC0: case 0xC1: case 0xC2: case 0xC3:
                case 0xC4: case 0xC5: out += 'A'; break;
                case 0xE8: case 0xE9: case 0xEA: case 0xEB: out += 'e'; break;
                case 0xC8: case 0xC9: case 0xCA: case 0xCB: out += 'E'; break;
                case 0xEC: case 0xED: case 0xEE: case 0xEF: out += 'i'; break;
                case 0xCC: case 0xCD: case 0xCE: case 0xCF: out += 'I'; break;
                case 0xF2: case 0xF3: case 0xF4: case 0xF5:
                case 0xF6: out += 'o'; break;
                case 0xD2: case 0xD3: case 0xD4: case 0xD5:
                case 0xD6: out += 'O'; break;
                case 0xF9: case 0xFA: case 0xFB: case 0xFC: out += 'u'; break;
                case 0xD9: case 0xDA: case 0xDB: case 0xDC: out += 'U'; break;
                case 0xE7: out += 'c'; break;
                case 0xC7: out += 'C'; break;
                case 0xF1: out += 'n'; break;
                case 0xD1: out += 'N'; break;
                default:   out += '?'; break;
            }
            continue;
        }
        // Autres séquences multi-octets — skip
        if ((c & 0xF0) == 0xE0) { s += 3; out += '?'; }
        else if ((c & 0xF8) == 0xF0) { s += 4; out += '?'; }
        else { s++; }
    }
    return out;
}


// ============================================================
//  Table xAdvance réelle de TTFont (proportionnelle, 0x20..0x7E)
//  Générée depuis smallClockFont.h — évite les erreurs de troncature
// ============================================================
static const uint8_t TTF_ADV[95] PROGMEM = {
  6,2,4,6,6,6,6,2,3,3,4,4,2,5,2,6,  // 0x20-0x2F
  5,2,5,5,5,5,5,5,5,5,2,2,4,5,4,6,  // 0x30-0x3F
  6,6,6,5,5,5,5,5,5,6,5,5,5,6,6,5,  // 0x40-0x4F
  5,5,5,5,6,5,6,6,6,6,6,3,6,3,4,6,  // 0x50-0x5F
  2,5,5,5,5,5,4,5,5,2,3,5,2,6,5,5,  // 0x60-0x6F
  5,5,5,5,6,5,5,6,5,5,5,4,2,4,6     // 0x70-0x7E
};

// Largeur en pixels d'une string avec TTFont (proportionnel)
static inline int ttfWidth(const String& s) {
    int w = 0;
    for (int i = 0; i < (int)s.length(); i++) {
        uint8_t c = (uint8_t)s[i];
        if (c >= 0x20 && c <= 0x7E)
            w += pgm_read_byte(&TTF_ADV[c - 0x20]);
        else
            w += 5; // fallback
    }
    return w;
}

// Tronquer une string pour tenir dans maxPx pixels
static inline String ttfTrunc(const String& s, int maxPx) {
    int w = 0;
    for (int i = 0; i < (int)s.length(); i++) {
        uint8_t c = (uint8_t)s[i];
        int adv = (c >= 0x20 && c <= 0x7E) ? pgm_read_byte(&TTF_ADV[c - 0x20]) : 5;
        if (w + adv > maxPx) return s.substring(0, i);
        w += adv;
    }
    return s;
}

// x de départ pour centrer une string sur 64px
static inline int ttfCenterX(const String& s) {
    return max(0, (64 - ttfWidth(s)) / 2);
}

// x de départ pour aligner à droite sur 64px
static inline int ttfRightX(const String& s) {
    return max(0, 63 - ttfWidth(s));
}

// ============================================================
//  Accès rapide au thème courant
// ============================================================
class Theme565 {
public:
    // Appeler après chaque changement de thème
    static void refresh(MatrixPanel_I2S_DMA* mx) {
        const Theme& t = THEMES[ConfigManager::instance().cfg.themeId % THEME_COUNT];
        primary   = mx->color565(t.r1, t.g1, t.b1);
        secondary = mx->color565(t.r2, t.g2, t.b2);
        accent    = mx->color565(t.ra, t.ga, t.ba);
        dim       = mx->color565(t.rd, t.gd, t.bd);
        // Couleur de fond très sombre dérivée du primaire
        bg        = mx->color565(t.r1/12, t.g1/12, t.b1/12);
    }

    static uint16_t primary;
    static uint16_t secondary;
    static uint16_t accent;
    static uint16_t dim;
    static uint16_t bg;
};

uint16_t Theme565::primary   = 0;
uint16_t Theme565::secondary = 0;
uint16_t Theme565::accent    = 0;
uint16_t Theme565::dim       = 0;
uint16_t Theme565::bg        = 0;

// ============================================================
//  Helpers d'impression avec police TomThumb
// ============================================================
namespace DH {

// Activer la police TomThumb (3x5, espacement réel ~4x6 avec padding)
inline void useSmallFont(MatrixPanel_I2S_DMA* mx) {
    mx->setFont(&TTFont);
    mx->setTextSize(1);
}

// Police grande pour l'heure (bigClockFont)
inline void useBigFont(MatrixPanel_I2S_DMA* mx) {
    mx->setFont(&bigClockFont);
    mx->setTextSize(1);
}

// Revenir à la police bitmap par défaut (6x8)
inline void useDefaultFont(MatrixPanel_I2S_DMA* mx) {
    mx->setFont(nullptr);
    mx->setTextSize(1);
}

// Écrire du texte sans accents avec TomThumb
inline void print(MatrixPanel_I2S_DMA* mx, int x, int y, const String& txt, uint16_t color) {
    mx->setTextColor(color);
    mx->setCursor(x, y);
    mx->print(stripAccents(txt));
}

// Écrire centré horizontalement (utilise les vrais xAdvance de TTFont)
inline void printCentered(MatrixPanel_I2S_DMA* mx, int y, const String& txt, uint16_t color) {
    String s = stripAccents(txt);
    int x = ttfCenterX(s);
    print(mx, x, y, s, color);
}

// Ligne séparatrice thématique
inline void hline(MatrixPanel_I2S_DMA* mx, int y) {
    mx->drawFastHLine(0, y, 64, Theme565::dim);
}

// Couleur de fond uniforme très sombre pour ce thème
// On utilise r1 uniquement (= composante Rouge réelle avec swap G↔B)
// pour une légère teinte sans dégradé parasite
inline uint16_t bgColor(MatrixPanel_I2S_DMA* mx) {
    const Theme& t = THEMES[ConfigManager::instance().cfg.themeId % THEME_COUNT];
    // Fond très sombre : 1/16 de la composante dominante, max 12
    uint8_t r = min(12, (int)t.r1 / 16);
    uint8_t g = min(12, (int)t.g1 / 16);
    uint8_t b = min(12, (int)t.b1 / 16);
    return mx->color565(r, g, b);
}

// Même couleur de fond pour une ligne y (compatibilité ScreenAgenda)
inline uint16_t gradientBgColor(MatrixPanel_I2S_DMA* mx, int y) {
    const Theme& t = THEMES[ConfigManager::instance().cfg.themeId % THEME_COUNT];
    return t.blackBg ? 0 : bgColor(mx);
}

// Fond uniforme — noir pur si thème blackBg, sinon couleur dérivée du thème
inline void gradientBg(MatrixPanel_I2S_DMA* mx) {
    const Theme& t = THEMES[ConfigManager::instance().cfg.themeId % THEME_COUNT];
    if (t.blackBg) {
        mx->fillScreen(0); // noir pur, LEDs éteintes
    } else {
        mx->fillScreen(bgColor(mx));
    }
}

} // namespace DH
