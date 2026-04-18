#pragma once
// ============================================================
//  ScreenBoot.h  —  Écran de démarrage "Horloge Familiale"
//  Affiché pendant le boot et la config WiFi
// ============================================================

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "../DisplayHelper.h"

class ScreenBoot {
public:
    static void show(MatrixPanel_I2S_DMA* mx, const char* msg) {
        if (!mx) return;

        // Zone haute (y=0..7) : noir pur
        for (int y = 0; y < 8; y++)
            mx->drawFastHLine(0, y, 64, 0);

        // Zone colorée (y=8..31) : dégradé du bas (lumineux) vers y=8 (sombre)
        // intensité varie de 0 (y=8) à 23 (y=31)
        for (int y = 8; y < 32; y++) {
            int v = y - 8; // 0..23
            mx->drawFastHLine(0, y, 64, mx->color565(0, v/3, v/2));
        }

        DH::useSmallFont(mx);
        uint16_t cPrim = Theme565::primary
                       ? Theme565::primary
                       : mx->color565(0, 180, 255);
        uint16_t cSec  = Theme565::secondary
                       ? Theme565::secondary
                       : mx->color565(130, 130, 130);

        // "Horloge" centrée
        String l1 = "Horloge";
        mx->setTextColor(cPrim);
        mx->setCursor(ttfCenterX(l1), 8);
        mx->print(l1);

        // "Familiale" centrée
        String l2 = "Familiale";
        mx->setTextColor(cPrim);
        mx->setCursor(ttfCenterX(l2), 17);
        mx->print(l2);

        // Trait à 8px du bas (y=24)
        DH::hline(mx, 24);

        // Message de statut centré dans la zone basse
        String m = stripAccents(String(msg));
        mx->setTextColor(cSec);
        mx->setCursor(ttfCenterX(m), 30);
        mx->print(m);

        mx->flipDMABuffer();
    }
};
