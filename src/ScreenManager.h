#pragma once
// ============================================================
//  ScreenManager.h  —  Gestion de la rotation des écrans
// ============================================================

#include <Arduino.h>
#include "Config.h"

class BaseScreen {
public:
    virtual void onEnter()  {}                          // appelé quand l'écran devient actif
    virtual void onExit()   {}                          // appelé quand on quitte l'écran
    virtual void update()   = 0;                        // appelé chaque loop() — logique
    virtual void draw()     = 0;                        // appelé chaque loop() — rendu
    virtual bool needsRefresh() { return true; }        // false = ne redessiner que si dirty
    virtual ~BaseScreen() {}
};

class ScreenManager {
public:
    static ScreenManager& instance() {
        static ScreenManager inst;
        return inst;
    }

    // Enregistrer les écrans (dans l'ordre de SCREEN_ID)
    void registerScreen(ScreenID id, BaseScreen* screen) {
        if (id < SCREEN_COUNT) _screens[id] = screen;
    }

    void begin() {
        AppConfig& cfg = ConfigManager::instance().cfg;
        // Démarrer sur le premier écran activé
        _current = SCREEN_CLOCK;
        for (uint8_t i = 0; i < SCREEN_COUNT; i++) {
            if (cfg.screenEnabled[i]) { _current = static_cast<ScreenID>(i); break; }
        }
        _lastSwitch = millis();
        if (_screens[_current]) _screens[_current]->onEnter();
    }

    void loop() {
        AppConfig& cfg = ConfigManager::instance().cfg;
        uint32_t now = millis();

        // Rotation automatique
        if (cfg.autoRotate && (now - _lastSwitch >= cfg.screenDurationMs)) {
            nextScreen();
        }

        if (_screens[_current]) {
            _screens[_current]->update();
            _screens[_current]->draw();
        }
    }

    void nextScreen() {
        // Chercher le prochain écran activé (évite boucle infinie si tous désactivés)
        AppConfig& cfg = ConfigManager::instance().cfg;
        for (uint8_t i = 1; i <= SCREEN_COUNT; i++) {
            ScreenID candidate = static_cast<ScreenID>((_current + i) % SCREEN_COUNT);
            if (cfg.screenEnabled[candidate]) {
                goTo(candidate);
                return;
            }
        }
        // Aucun écran activé → rester sur l'actuel
    }

    void prevScreen() {
        AppConfig& cfg = ConfigManager::instance().cfg;
        for (uint8_t i = 1; i <= SCREEN_COUNT; i++) {
            ScreenID candidate = static_cast<ScreenID>((_current + SCREEN_COUNT - i) % SCREEN_COUNT);
            if (cfg.screenEnabled[candidate]) {
                goTo(candidate);
                return;
            }
        }
    }

    void goTo(ScreenID id) {
        if (id >= SCREEN_COUNT || !_screens[id]) return;
        if (_screens[_current]) _screens[_current]->onExit();
        _current = id;
        _lastSwitch = millis();
        if (_screens[_current]) _screens[_current]->onEnter();
    }

    ScreenID current() const { return _current; }

private:
    BaseScreen* _screens[SCREEN_COUNT] = {};
    ScreenID    _current   = SCREEN_CLOCK;
    uint32_t    _lastSwitch = 0;
};
