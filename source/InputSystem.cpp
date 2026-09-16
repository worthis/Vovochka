#include "InputSystem.h"
#include <cmath>

namespace vovochka
{
    // Вспомогательные функции
    static bool isAnyKeyDown(const std::vector<int> &keys)
    {
        for (int key : keys)
        {
            if (IsKeyDown(key))
                return true;
        }
        return false;
    }

    static bool isAnyKeyPressed(const std::vector<int> &keys)
    {
        for (int key : keys)
        {
            if (IsKeyPressed(key))
                return true;
        }
        return false;
    }

    static bool isAnyGamepadButtonDown(const std::vector<int> &buttons)
    {
        if (!IsGamepadAvailable(0))
            return false;

        for (int btn : buttons)
        {
            if (IsGamepadButtonDown(0, btn))
                return true;
        }
        return false;
    }

    static bool isAnyGamepadButtonPressed(const std::vector<int> &buttons)
    {
        if (!IsGamepadAvailable(0))
            return false;

        for (int btn : buttons)
        {
            if (IsGamepadButtonPressed(0, btn))
                return true;
        }
        return false;
    }

    InputSystem::InputSystem()
    {
    }

    void InputSystem::update(float dt)
    {
        moveX = moveY = 0;

        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            // Левый стик
            const float stickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            const float stickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

            if (fabsf(stickX) > DEADZONE)
                moveX += (stickX > 0.0f) ? 1 : -1;
            if (fabsf(stickY) > DEADZONE)
                moveY -= (stickY > 0.0f) ? 1 : -1;

            m_prevLStickDir = m_currLStickDir;
            if (std::fabs(stickX) > DEADZONE || std::fabs(stickY) > DEADZONE)
            {
                if (std::fabs(stickX) > std::fabs(stickY))
                    m_currLStickDir = (stickX > 0.0f) ? StickDirection::Right : StickDirection::Left;
                else
                    m_currLStickDir = (stickY > 0.0f) ? StickDirection::Up : StickDirection::Down;
            }
            else
            {
                m_currLStickDir = StickDirection::None;
            }

            // D-pad
            if (isAnyGamepadButtonDown(keys.moveLeft.gamepad))
                moveX -= 1;
            if (isAnyGamepadButtonDown(keys.moveRight.gamepad))
                moveX += 1;
            if (isAnyGamepadButtonDown(keys.moveUp.gamepad))
                moveY -= 1;
            if (isAnyGamepadButtonDown(keys.moveDown.gamepad))
                moveY += 1;
        }
        else
        {
            m_prevLStickDir = m_currLStickDir = StickDirection::None;

            // Клавиатура
            if (isAnyKeyDown(keys.moveLeft.keyboard))
                moveX -= 1;
            if (isAnyKeyDown(keys.moveRight.keyboard))
                moveX += 1;
            if (isAnyKeyDown(keys.moveUp.keyboard))
                moveY -= 1;
            if (isAnyKeyDown(keys.moveDown.keyboard))
                moveY += 1;
        }

        moveX = std::clamp(moveX, -1, 1);
        moveY = std::clamp(moveY, -1, 1);

        // === Курсор ===
        m_mouseLeftPressed = false;
        m_touchPressed = false;

        // 1) Мышь (ПК). Реагируем ТОЛЬКО на фактическое движение или клик:
        //    на Switch GetMousePosition() всегда (0,0)
        const Vector2 rawMouse = GetMousePosition();
        const bool mouseMoved = (rawMouse.x != m_lastRawMouse.x || rawMouse.y != m_lastRawMouse.y);
        if (mouseMoved)
        {
            if (m_lastRawMouse.x >= 0.0f)
            {
                m_cursorPos = screenToMenu(rawMouse);
                m_cursorVisible = true;
                m_cursorActive = true;
                m_cursorIdleTime = 0.0f;
            }
            m_lastRawMouse = rawMouse;
        }
        m_mouseLeftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (m_mouseLeftPressed)
        {
            m_cursorPos = screenToMenu(rawMouse);
            m_cursorVisible = true;
            m_cursorActive = true;
            m_cursorIdleTime = 0.0f;
        }

        // 2) Тач (Switch): палец ведёт курсор, касание = клик
        const int touchCount = GetTouchPointCount();
        if (touchCount > 0)
        {
            m_cursorPos = screenToMenu(GetTouchPosition(0));
            m_cursorVisible = false;
            m_cursorActive = true;
            m_cursorIdleTime = 0.0f;
        }
        m_touchPressed = (touchCount > 0 && m_prevTouchCount == 0);
        m_prevTouchCount = touchCount;

        // 3) Правый стик: интегрирует скорость, показывает курсор;
        //    простой > 3 сек — скрыть (актуально на Switch)
        if (IsGamepadAvailable(0))
        {
            const float rx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
            const float ry = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
            if (std::fabs(rx) > DEADZONE || std::fabs(ry) > DEADZONE)
            {
                m_cursorPos.x = std::clamp(m_cursorPos.x + rx * RIGHT_STICK_SPEED * dt, 0.0f, 800.0f);
                m_cursorPos.y = std::clamp(m_cursorPos.y + ry * RIGHT_STICK_SPEED * dt, 0.0f, 600.0f);
                m_cursorVisible = true;
                m_cursorIdleTime = 0.0f;
            }
            else
            {
                m_cursorIdleTime += dt;
                if (m_cursorIdleTime > RIGHT_STICK_IDLE_HIDE)
                {
                    m_cursorVisible = false;
                    m_cursorActive = false;
                }
            }
        }
    }

    bool InputSystem::isMovePressed() const
    {
        return moveX != 0 || moveY != 0;
    }

    bool InputSystem::isBombPressed() const
    {
        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            return isAnyGamepadButtonPressed(keys.bomb.gamepad);
        }
        else
        {
            return isAnyKeyPressed(keys.bomb.keyboard);
        }
    }

    bool InputSystem::isPausePressed() const
    {
        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            return isAnyGamepadButtonPressed(keys.pause.gamepad);
        }
        else
        {
            return isAnyKeyPressed(keys.pause.keyboard);
        }
    }

    // === Меню ===
    bool InputSystem::isMenuUpPressed() const
    {
        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            const bool btn = isAnyGamepadButtonPressed(keys.menuUp.gamepad);
            const bool stick = (m_currLStickDir == StickDirection::Up && m_prevLStickDir != StickDirection::Up);
            return btn || stick;
        }
        else
        {
            return isAnyKeyPressed(keys.menuUp.keyboard);
        }
    }

    bool InputSystem::isMenuDownPressed() const
    {
        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            const bool btn = isAnyGamepadButtonPressed(keys.menuDown.gamepad);
            const bool stick = (m_currLStickDir == StickDirection::Down && m_prevLStickDir != StickDirection::Down);
            return btn || stick;
        }
        else
        {
            return isAnyKeyPressed(keys.menuDown.keyboard);
        }
    }

    bool InputSystem::isMenuLeftPressed() const
    {
        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            const bool btn = isAnyGamepadButtonPressed(keys.menuLeft.gamepad);
            const bool stick = (m_currLStickDir == StickDirection::Left && m_prevLStickDir != StickDirection::Left);
            return btn || stick;
        }
        else
        {
            return isAnyKeyPressed(keys.menuLeft.keyboard);
        }
    }

    bool InputSystem::isMenuRightPressed() const
    {
        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            const bool btn = isAnyGamepadButtonPressed(keys.menuRight.gamepad);
            const bool stick = (m_currLStickDir == StickDirection::Right && m_prevLStickDir != StickDirection::Right);
            return btn || stick;
        }
        else
        {
            return isAnyKeyPressed(keys.menuRight.keyboard);
        }
    }

    bool InputSystem::isMenuConfirmPressed() const
    {
        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            return isAnyGamepadButtonPressed(keys.menuConfirm.gamepad);
        }
        else
        {
            return isAnyKeyPressed(keys.menuConfirm.keyboard);
        }
    }

    bool InputSystem::isMenuCancelPressed() const
    {
        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            return isAnyGamepadButtonPressed(keys.menuCancel.gamepad);
        }
        else
        {
            return isAnyKeyPressed(keys.menuCancel.keyboard);
        }
    }

    // --- Курсор ---
    Vector2 InputSystem::screenToMenu(Vector2 p) const
    {
        const int sw = GetScreenWidth();
        const int sh = GetScreenHeight();
        const float x0 = (float)std::max(0, (sw - 800) / 2);
        const float y0 = (float)std::max(0, (sh - 600) / 2);
        return {p.x - x0, p.y - y0};
    }

} // namespace vovochka