// source/InputSystem.h
#pragma once

#include "raylib.h"
#include "ConfigSystem.h"

namespace vovochka
{
    class InputSystem
    {
    public:
        InputSystem();

        void update();

        // Движение игрока
        int getMoveX() const;
        int getMoveY() const;
        bool isMovePressed() const;

        // Действия
        bool isBombPressed() const;
        bool isPausePressed() const;

        // Меню (для будущего)
        bool isMenuUpPressed() const;
        bool isMenuDownPressed() const;
        bool isMenuLeftPressed() const;
        bool isMenuRightPressed() const;
        bool isMenuConfirmPressed() const;
        bool isMenuCancelPressed() const;

    private:
        static constexpr float DEADZONE = 0.15f;

        ConfigSystem &config = ConfigSystem::instance();

        int moveX = 0;
        int moveY = 0;

        enum class StickDirection
        {
            None,
            Left,
            Right,
            Up,
            Down
        };

        StickDirection m_prevLStickDir = StickDirection::None;
        StickDirection m_currLStickDir = StickDirection::None;
    };

} // namespace vovochka