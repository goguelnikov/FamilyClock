#pragma once
// ============================================================
//  ScreenNotes.h  —  Affichage de notes personnalisées
//  4 lignes de texte libre configurées depuis l'interface web
//  Layout :
//    y=4   "NOTES"
//    y=6   trait
//    y=12  ligne 1
//    y=18  ligne 2
//    y=24  ligne 3
//    y=30  ligne 4
// ============================================================

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "../ScreenManager.h"
#include "../DisplayHelper.h"
#include "../Config.h"

class ScreenNotes : public BaseScreen {
public:
    explicit ScreenNotes(MatrixPanel_I2S_DMA* matrix) : _mx(matrix) {}

    void onEnter() override {
        Theme565::refresh(_mx);
        _dirty = true;
    }

    void update() override {
        // Redessiner si la config a changé (notes modifiées depuis le web)
        static uint32_t _lastCheck = 0;
        if (millis() - _lastCheck > 2000) { // vérifier toutes les 2s
            _lastCheck = millis();
            _dirty = true; // relire cfg.notes à chaque fois (léger)
        }
    }

    void draw() override {
        if (!_dirty) return;
        _dirty = false;

        DH::gradientBg(_mx);
        DH::useSmallFont(_mx);

        // Titre
        DH::print(_mx, 1, 4, "NOTES", Theme565::accent);
        DH::hline(_mx, 6);

        AppConfig& cfg = ConfigManager::instance().cfg;

        // 4 lignes
        const int yLines[4] = {12, 18, 24, 30};
        for (uint8_t i = 0; i < 4; i++) {
            String line = stripAccents(String(cfg.notes[i]));
            if (line.length() == 0) continue;
            line = ttfTrunc(line, 63);
            DH::print(_mx, 1, yLines[i], line, Theme565::primary);
        }

        _mx->flipDMABuffer();
    }

    // Appelé depuis WebConfig après sauvegarde
    void refresh() { _dirty = true; }

private:
    MatrixPanel_I2S_DMA* _mx;
    bool _dirty = true;
};
