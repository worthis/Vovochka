// source/ConfigSystem.cpp
#include "ConfigSystem.h"
#include "InputMappings.h"
#include <fstream>
#include "raylib.h"

bool ConfigSystem::loadSettings(const std::string &path)
{
    settingsPath = path;

    std::ifstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_WARNING, "Settings file not found, creating default: %s", path.c_str());
        createDefaultSettings();
        saveSettings(path);
        return false;
    }

    json j;
    file >> j;

    if (j.contains("input"))
    {
        auto &input = j["input"];

        auto loadBinding = [&input](const std::string &name, KeyBinding &binding)
        {
            if (!input.contains(name))
                return;

            auto &obj = input[name];

            if (obj.contains("keyboard") && obj["keyboard"].is_array())
            {
                for (const auto &keyName : obj["keyboard"])
                {
                    int code = stringToKey(keyName.get<std::string>());
                    if (code != KEY_NULL)
                        binding.keyboard.push_back(code);
                }
            }

            if (obj.contains("mouse") && obj["mouse"].is_array())
            {
                for (const auto &btnName : obj["mouse"])
                {
                    int code = stringToMouseButton(btnName.get<std::string>());
                    if (code != -1)
                        binding.mouse.push_back(code);
                }
            }

            if (obj.contains("gamepad") && obj["gamepad"].is_array())
            {
                for (const auto &btnName : obj["gamepad"])
                {
                    int code = stringToGamepadButton(btnName.get<std::string>());
                    if (code != -1)
                        binding.gamepad.push_back(code);
                }
            }
        };

        loadBinding("moveUp", inputConfig.moveUp);
        loadBinding("moveDown", inputConfig.moveDown);
        loadBinding("moveLeft", inputConfig.moveLeft);
        loadBinding("moveRight", inputConfig.moveRight);
        loadBinding("bomb", inputConfig.bomb);
        loadBinding("pause", inputConfig.pause);
        loadBinding("menuUp", inputConfig.menuUp);
        loadBinding("menuDown", inputConfig.menuDown);
        loadBinding("menuLeft", inputConfig.menuLeft);
        loadBinding("menuRight", inputConfig.menuRight);
        loadBinding("menuConfirm", inputConfig.menuConfirm);
        loadBinding("menuCancel", inputConfig.menuCancel);
    }

    TraceLog(LOG_INFO, "Settings loaded from %s", path.c_str());
    return true;
}

void ConfigSystem::saveSettings(const std::string &path)
{
    auto saveBinding = [](const KeyBinding &binding) -> json
    {
        json obj;

        if (binding.hasKeyboard())
        {
            json keys = json::array();
            for (int code : binding.keyboard)
                keys.push_back(keyToString(code));
            obj["keyboard"] = keys;
        }

        if (binding.hasMouse())
        {
            json buttons = json::array();
            for (int code : binding.mouse)
                buttons.push_back(mouseButtonToString(code));
            obj["mouse"] = buttons;
        }

        if (binding.hasGamepad())
        {
            json buttons = json::array();
            for (int code : binding.gamepad)
                buttons.push_back(gamepadButtonToString(code));
            obj["gamepad"] = buttons;
        }

        return obj;
    };

    json j;
    j["input"] = {
        {"moveUp", saveBinding(inputConfig.moveUp)},
        {"moveDown", saveBinding(inputConfig.moveDown)},
        {"moveLeft", saveBinding(inputConfig.moveLeft)},
        {"moveRight", saveBinding(inputConfig.moveRight)},
        {"bomb", saveBinding(inputConfig.bomb)},
        {"pause", saveBinding(inputConfig.pause)},
        {"menuUp", saveBinding(inputConfig.menuUp)},
        {"menuDown", saveBinding(inputConfig.menuDown)},
        {"menuLeft", saveBinding(inputConfig.menuLeft)},
        {"menuRight", saveBinding(inputConfig.menuRight)},
        {"menuConfirm", saveBinding(inputConfig.menuConfirm)},
        {"menuCancel", saveBinding(inputConfig.menuCancel)}};

    std::ofstream file(path);
    if (file.is_open())
    {
        file << j.dump(4);
        TraceLog(LOG_INFO, "Settings saved to %s", path.c_str());
    }
    else
    {
        TraceLog(LOG_WARNING, "Failed to save settings to %s", path.c_str());
    }
}

void ConfigSystem::createDefaultSettings()
{
    // Движение: WASD + стрелки / D-Pad + левый стик
    inputConfig.moveUp.keyboard = {KEY_W, KEY_UP};
    inputConfig.moveUp.gamepad = {GAMEPAD_BUTTON_LEFT_FACE_UP};

    inputConfig.moveDown.keyboard = {KEY_S, KEY_DOWN};
    inputConfig.moveDown.gamepad = {GAMEPAD_BUTTON_LEFT_FACE_DOWN};

    inputConfig.moveLeft.keyboard = {KEY_A, KEY_LEFT};
    inputConfig.moveLeft.gamepad = {GAMEPAD_BUTTON_LEFT_FACE_LEFT};

    inputConfig.moveRight.keyboard = {KEY_D, KEY_RIGHT};
    inputConfig.moveRight.gamepad = {GAMEPAD_BUTTON_LEFT_FACE_RIGHT};

    // Бомба: Пробел / A
    inputConfig.bomb.keyboard = {KEY_SPACE};
    inputConfig.bomb.gamepad = {GAMEPAD_BUTTON_RIGHT_FACE_DOWN}; // A

    // Пауза: Esc / Start
    inputConfig.pause.keyboard = {KEY_ESCAPE};
    inputConfig.pause.gamepad = {GAMEPAD_BUTTON_MIDDLE_RIGHT}; // Start

    // Меню
    inputConfig.menuUp.keyboard = {KEY_UP, KEY_W};
    inputConfig.menuUp.gamepad = {GAMEPAD_BUTTON_LEFT_FACE_UP};

    inputConfig.menuDown.keyboard = {KEY_DOWN, KEY_S};
    inputConfig.menuDown.gamepad = {GAMEPAD_BUTTON_LEFT_FACE_DOWN};

    inputConfig.menuLeft.keyboard = {KEY_LEFT, KEY_A};
    inputConfig.menuLeft.gamepad = {GAMEPAD_BUTTON_LEFT_FACE_LEFT};

    inputConfig.menuRight.keyboard = {KEY_RIGHT, KEY_D};
    inputConfig.menuRight.gamepad = {GAMEPAD_BUTTON_LEFT_FACE_RIGHT};

    inputConfig.menuConfirm.keyboard = {KEY_ENTER, KEY_SPACE};
    inputConfig.menuConfirm.gamepad = {GAMEPAD_BUTTON_RIGHT_FACE_DOWN}; // A

    inputConfig.menuCancel.keyboard = {KEY_BACKSPACE, KEY_ESCAPE};
    inputConfig.menuCancel.gamepad = {GAMEPAD_BUTTON_RIGHT_FACE_RIGHT}; // B
}