#pragma once

#include "raylib.h"
#include "third_party/json.hpp"
#include <string>
#include <vector>

using json = nlohmann::json;

struct KeyBinding
{
    std::vector<int> keyboard;
    std::vector<int> mouse;
    std::vector<int> gamepad;

    bool hasKeyboard() const { return !keyboard.empty(); }
    bool hasMouse() const { return !mouse.empty(); }
    bool hasGamepad() const { return !gamepad.empty(); }
};

struct InputConfig
{
    // Движение игрока (4 направления)
    KeyBinding moveUp;
    KeyBinding moveDown;
    KeyBinding moveLeft;
    KeyBinding moveRight;

    // Действия
    KeyBinding bomb;  // Пробел / A
    KeyBinding pause; // Esc / Start

    // Меню
    KeyBinding menuUp;
    KeyBinding menuDown;
    KeyBinding menuLeft;
    KeyBinding menuRight;
    KeyBinding menuConfirm;
    KeyBinding menuCancel;
};

class ConfigSystem
{
public:
    static ConfigSystem &instance()
    {
        static ConfigSystem instance;
        return instance;
    }

    bool loadSettings(const std::string &path);
    void saveSettings(const std::string &path);

    const InputConfig &getInputConfig() const { return inputConfig; }

private:
    ConfigSystem() = default;
    ~ConfigSystem() = default;
    ConfigSystem(const ConfigSystem &) = delete;
    ConfigSystem &operator=(const ConfigSystem &) = delete;

    InputConfig inputConfig;
    std::string settingsPath;

    void createDefaultSettings();
};