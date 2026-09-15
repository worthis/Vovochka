#pragma once

#include "raylib.h"
#include "ConfigSystem.h"

namespace vovochka
{
    class InputSystem
    {
    public:
        InputSystem();

        void update(float dt);

        // Движение игрока
        int getMoveX() const { return moveX; }
        int getMoveY() const { return moveY; }
        bool isMovePressed() const;

        // Действия
        bool isBombPressed() const;
        bool isPausePressed() const;

        // Меню
        bool isMenuUpPressed() const;
        bool isMenuDownPressed() const;
        bool isMenuLeftPressed() const;
        bool isMenuRightPressed() const;
        bool isMenuConfirmPressed() const;
        bool isMenuCancelPressed() const;

        // Курсор
        bool hasCursor() const { return m_cursorActive; }
        bool isCursorVisible() const { return m_cursorVisible; }
        bool isCursorClicked() const { return m_mouseLeftPressed || m_touchPressed; }
        Vector2 getMenuCursorPos() const { return m_cursorPos; }

    private:
        static constexpr float DEADZONE = 0.15f;
        static constexpr float RIGHT_STICK_SPEED = 480.0f;
        static constexpr float RIGHT_STICK_IDLE_HIDE = 3.0f;

        ConfigSystem &config = ConfigSystem::instance();

        int moveX = 0, moveY = 0;

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

        Vector2 m_cursorPos = {400.0f, 300.0f};
        bool m_cursorVisible = false;
        bool m_cursorActive = false;
        float m_cursorIdleTime = 0.0f;

        Vector2 m_lastRawMouse = {-1.0f, -1.0f};
        bool m_mouseLeftPressed = false;

        bool m_touchPressed = false;
        int m_prevTouchCount = 0;

        Vector2 screenToMenu(Vector2 screenPos) const;
    };

} // namespace vovochka