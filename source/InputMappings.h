#pragma once

#include "raylib.h"
#include <string>
#include <map>

// Преобразование кода клавиши в строку
inline std::string keyToString(int key)
{
    static const std::map<int, std::string> keyNames = {
        {KEY_SPACE, "SPACE"},
        {KEY_ESCAPE, "ESC"},
        {KEY_ENTER, "ENTER"},
        {KEY_TAB, "TAB"},
        {KEY_BACKSPACE, "BACKSPACE"},
        {KEY_INSERT, "INSERT"},
        {KEY_DELETE, "DELETE"},
        {KEY_RIGHT, "RIGHT"},
        {KEY_LEFT, "LEFT"},
        {KEY_DOWN, "DOWN"},
        {KEY_UP, "UP"},
        {KEY_PAGE_UP, "PAGE_UP"},
        {KEY_PAGE_DOWN, "PAGE_DOWN"},
        {KEY_HOME, "HOME"},
        {KEY_END, "END"},
        {KEY_CAPS_LOCK, "CAPS_LOCK"},
        {KEY_SCROLL_LOCK, "SCROLL_LOCK"},
        {KEY_NUM_LOCK, "NUM_LOCK"},
        {KEY_PRINT_SCREEN, "PRINT_SCREEN"},
        {KEY_PAUSE, "PAUSE"},
        {KEY_F1, "F1"},
        {KEY_F2, "F2"},
        {KEY_F3, "F3"},
        {KEY_F4, "F4"},
        {KEY_F5, "F5"},
        {KEY_F6, "F6"},
        {KEY_F7, "F7"},
        {KEY_F8, "F8"},
        {KEY_F9, "F9"},
        {KEY_F10, "F10"},
        {KEY_F11, "F11"},
        {KEY_F12, "F12"},
        {KEY_LEFT_SHIFT, "LEFT_SHIFT"},
        {KEY_LEFT_CONTROL, "LEFT_CONTROL"},
        {KEY_LEFT_ALT, "LEFT_ALT"},
        {KEY_LEFT_SUPER, "LEFT_SUPER"},
        {KEY_RIGHT_SHIFT, "RIGHT_SHIFT"},
        {KEY_RIGHT_CONTROL, "RIGHT_CONTROL"},
        {KEY_RIGHT_ALT, "RIGHT_ALT"},
        {KEY_RIGHT_SUPER, "RIGHT_SUPER"},
        {KEY_KB_MENU, "MENU"},
        {KEY_APOSTROPHE, "'"},
        {KEY_COMMA, ","},
        {KEY_MINUS, "-"},
        {KEY_PERIOD, "."},
        {KEY_SLASH, "/"},
        {KEY_ZERO, "0"},
        {KEY_ONE, "1"},
        {KEY_TWO, "2"},
        {KEY_THREE, "3"},
        {KEY_FOUR, "4"},
        {KEY_FIVE, "5"},
        {KEY_SIX, "6"},
        {KEY_SEVEN, "7"},
        {KEY_EIGHT, "8"},
        {KEY_NINE, "9"},
        {KEY_SEMICOLON, ";"},
        {KEY_EQUAL, "="},
        {KEY_A, "A"},
        {KEY_B, "B"},
        {KEY_C, "C"},
        {KEY_D, "D"},
        {KEY_E, "E"},
        {KEY_F, "F"},
        {KEY_G, "G"},
        {KEY_H, "H"},
        {KEY_I, "I"},
        {KEY_J, "J"},
        {KEY_K, "K"},
        {KEY_L, "L"},
        {KEY_M, "M"},
        {KEY_N, "N"},
        {KEY_O, "O"},
        {KEY_P, "P"},
        {KEY_Q, "Q"},
        {KEY_R, "R"},
        {KEY_S, "S"},
        {KEY_T, "T"},
        {KEY_U, "U"},
        {KEY_V, "V"},
        {KEY_W, "W"},
        {KEY_X, "X"},
        {KEY_Y, "Y"},
        {KEY_Z, "Z"},
        {KEY_LEFT_BRACKET, "["},
        {KEY_BACKSLASH, "\\"},
        {KEY_RIGHT_BRACKET, "]"},
        {KEY_GRAVE, "`"},
    };

    auto it = keyNames.find(key);
    if (it != keyNames.end())
        return it->second;

    return "KEY_" + std::to_string(key);
}

// Преобразование строки в код клавиши
inline int stringToKey(const std::string &name)
{
    static const std::map<std::string, int> keyCodes = {
        {"SPACE", KEY_SPACE},
        {"ESC", KEY_ESCAPE},
        {"ESCAPE", KEY_ESCAPE},
        {"ENTER", KEY_ENTER},
        {"RETURN", KEY_ENTER},
        {"TAB", KEY_TAB},
        {"BACKSPACE", KEY_BACKSPACE},
        {"INSERT", KEY_INSERT},
        {"DELETE", KEY_DELETE},
        {"RIGHT", KEY_RIGHT},
        {"LEFT", KEY_LEFT},
        {"DOWN", KEY_DOWN},
        {"UP", KEY_UP},
        {"PAGE_UP", KEY_PAGE_UP},
        {"PAGEUP", KEY_PAGE_UP},
        {"PAGE_DOWN", KEY_PAGE_DOWN},
        {"PAGEDOWN", KEY_PAGE_DOWN},
        {"HOME", KEY_HOME},
        {"END", KEY_END},
        {"CAPS_LOCK", KEY_CAPS_LOCK},
        {"CAPSLOCK", KEY_CAPS_LOCK},
        {"SCROLL_LOCK", KEY_SCROLL_LOCK},
        {"SCROLLLOCK", KEY_SCROLL_LOCK},
        {"NUM_LOCK", KEY_NUM_LOCK},
        {"NUMLOCK", KEY_NUM_LOCK},
        {"PRINT_SCREEN", KEY_PRINT_SCREEN},
        {"PRINTSCREEN", KEY_PRINT_SCREEN},
        {"PAUSE", KEY_PAUSE},
        {"F1", KEY_F1},
        {"F2", KEY_F2},
        {"F3", KEY_F3},
        {"F4", KEY_F4},
        {"F5", KEY_F5},
        {"F6", KEY_F6},
        {"F7", KEY_F7},
        {"F8", KEY_F8},
        {"F9", KEY_F9},
        {"F10", KEY_F10},
        {"F11", KEY_F11},
        {"F12", KEY_F12},
        {"LEFT_SHIFT", KEY_LEFT_SHIFT},
        {"LEFTSHIFT", KEY_LEFT_SHIFT},
        {"LSHIFT", KEY_LEFT_SHIFT},
        {"LEFT_CONTROL", KEY_LEFT_CONTROL},
        {"LEFTCONTROL", KEY_LEFT_CONTROL},
        {"LCONTROL", KEY_LEFT_CONTROL},
        {"LCTRL", KEY_LEFT_CONTROL},
        {"LEFT_ALT", KEY_LEFT_ALT},
        {"LEFTALT", KEY_LEFT_ALT},
        {"LALT", KEY_LEFT_ALT},
        {"LEFT_SUPER", KEY_LEFT_SUPER},
        {"LEFTSUPER", KEY_LEFT_SUPER},
        {"RIGHT_SHIFT", KEY_RIGHT_SHIFT},
        {"RIGHTSHIFT", KEY_RIGHT_SHIFT},
        {"RSHIFT", KEY_RIGHT_SHIFT},
        {"RIGHT_CONTROL", KEY_RIGHT_CONTROL},
        {"RIGHTCONTROL", KEY_RIGHT_CONTROL},
        {"RCONTROL", KEY_RIGHT_CONTROL},
        {"RCTRL", KEY_RIGHT_CONTROL},
        {"RIGHT_ALT", KEY_RIGHT_ALT},
        {"RIGHTALT", KEY_RIGHT_ALT},
        {"RALT", KEY_RIGHT_ALT},
        {"RIGHT_SUPER", KEY_RIGHT_SUPER},
        {"RIGHTSUPER", KEY_RIGHT_SUPER},
        {"MENU", KEY_KB_MENU},
        {"APOSTROPHE", KEY_APOSTROPHE},
        {"COMMA", KEY_COMMA},
        {"MINUS", KEY_MINUS},
        {"PERIOD", KEY_PERIOD},
        {"SLASH", KEY_SLASH},
        {"0", KEY_ZERO},
        {"1", KEY_ONE},
        {"2", KEY_TWO},
        {"3", KEY_THREE},
        {"4", KEY_FOUR},
        {"5", KEY_FIVE},
        {"6", KEY_SIX},
        {"7", KEY_SEVEN},
        {"8", KEY_EIGHT},
        {"9", KEY_NINE},
        {"SEMICOLON", KEY_SEMICOLON},
        {"EQUAL", KEY_EQUAL},
        {"A", KEY_A},
        {"B", KEY_B},
        {"C", KEY_C},
        {"D", KEY_D},
        {"E", KEY_E},
        {"F", KEY_F},
        {"G", KEY_G},
        {"H", KEY_H},
        {"I", KEY_I},
        {"J", KEY_J},
        {"K", KEY_K},
        {"L", KEY_L},
        {"M", KEY_M},
        {"N", KEY_N},
        {"O", KEY_O},
        {"P", KEY_P},
        {"Q", KEY_Q},
        {"R", KEY_R},
        {"S", KEY_S},
        {"T", KEY_T},
        {"U", KEY_U},
        {"V", KEY_V},
        {"W", KEY_W},
        {"X", KEY_X},
        {"Y", KEY_Y},
        {"Z", KEY_Z},
        {"LEFT_BRACKET", KEY_LEFT_BRACKET},
        {"BACKSLASH", KEY_BACKSLASH},
        {"RIGHT_BRACKET", KEY_RIGHT_BRACKET},
        {"GRAVE", KEY_GRAVE},
    };

    auto it = keyCodes.find(name);
    return (it != keyCodes.end()) ? it->second : KEY_NULL;
}

// Преобразование кода кнопки мыши в строку
inline std::string mouseButtonToString(int button)
{
    switch (button)
    {
    case MOUSE_BUTTON_LEFT:
        return "LEFT_BUTTON";
    case MOUSE_BUTTON_RIGHT:
        return "RIGHT_BUTTON";
    case MOUSE_BUTTON_MIDDLE:
        return "MIDDLE_BUTTON";
    case MOUSE_BUTTON_SIDE:
        return "SIDE_BUTTON";
    case MOUSE_BUTTON_EXTRA:
        return "EXTRA_BUTTON";
    case MOUSE_BUTTON_FORWARD:
        return "FORWARD_BUTTON";
    case MOUSE_BUTTON_BACK:
        return "BACK_BUTTON";
    default:
        return "UNKNOWN";
    }
}

// Преобразование строки в код кнопки мыши
inline int stringToMouseButton(const std::string &name)
{
    if (name == "LEFT_BUTTON" || name == "LEFT")
        return MOUSE_BUTTON_LEFT;
    if (name == "RIGHT_BUTTON" || name == "RIGHT")
        return MOUSE_BUTTON_RIGHT;
    if (name == "MIDDLE_BUTTON" || name == "MIDDLE")
        return MOUSE_BUTTON_MIDDLE;
    if (name == "SIDE_BUTTON" || name == "SIDE")
        return MOUSE_BUTTON_SIDE;
    if (name == "EXTRA_BUTTON" || name == "EXTRA")
        return MOUSE_BUTTON_EXTRA;
    if (name == "FORWARD_BUTTON" || name == "FORWARD")
        return MOUSE_BUTTON_FORWARD;
    if (name == "BACK_BUTTON" || name == "BACK")
        return MOUSE_BUTTON_BACK;
    return -1;
}

// Преобразование кода кнопки геймпада в строку
inline std::string gamepadButtonToString(int button)
{
    switch (button)
    {
    case GAMEPAD_BUTTON_MIDDLE_LEFT:
        return "SELECT";
    case GAMEPAD_BUTTON_MIDDLE_RIGHT:
        return "START";
    case GAMEPAD_BUTTON_RIGHT_FACE_UP:
        return "Y";
    case GAMEPAD_BUTTON_RIGHT_FACE_RIGHT:
        return "B";
    case GAMEPAD_BUTTON_RIGHT_FACE_DOWN:
        return "A";
    case GAMEPAD_BUTTON_RIGHT_FACE_LEFT:
        return "X";
    case GAMEPAD_BUTTON_LEFT_FACE_UP:
        return "DPAD_UP";
    case GAMEPAD_BUTTON_LEFT_FACE_RIGHT:
        return "DPAD_RIGHT";
    case GAMEPAD_BUTTON_LEFT_FACE_DOWN:
        return "DPAD_DOWN";
    case GAMEPAD_BUTTON_LEFT_FACE_LEFT:
        return "DPAD_LEFT";
    case GAMEPAD_BUTTON_LEFT_TRIGGER_1:
        return "L1";
    case GAMEPAD_BUTTON_LEFT_TRIGGER_2:
        return "L2";
    case GAMEPAD_BUTTON_RIGHT_TRIGGER_1:
        return "R1";
    case GAMEPAD_BUTTON_RIGHT_TRIGGER_2:
        return "R2";
    default:
        return "UNKNOWN";
    }
}

// Преобразование строки в код кнопки геймпада
inline int stringToGamepadButton(const std::string &name)
{
    if (name == "SELECT")
        return GAMEPAD_BUTTON_MIDDLE_LEFT;
    if (name == "START")
        return GAMEPAD_BUTTON_MIDDLE_RIGHT;
    if (name == "Y")
        return GAMEPAD_BUTTON_RIGHT_FACE_UP;
    if (name == "B")
        return GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
    if (name == "A")
        return GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
    if (name == "X")
        return GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
    if (name == "DPAD_UP")
        return GAMEPAD_BUTTON_LEFT_FACE_UP;
    if (name == "DPAD_RIGHT")
        return GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
    if (name == "DPAD_DOWN")
        return GAMEPAD_BUTTON_LEFT_FACE_DOWN;
    if (name == "DPAD_LEFT")
        return GAMEPAD_BUTTON_LEFT_FACE_LEFT;
    if (name == "L1" || name == "LB")
        return GAMEPAD_BUTTON_LEFT_TRIGGER_1;
    if (name == "L2" || name == "LT")
        return GAMEPAD_BUTTON_LEFT_TRIGGER_2;
    if (name == "R1" || name == "RB")
        return GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
    if (name == "R2" || name == "RT")
        return GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
    return -1;
}