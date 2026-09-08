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

    void InputSystem::update()
    {
        moveX = 0;
        moveY = 0;

        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            // Левый стик
            float stickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            float stickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

            if (fabsf(stickX) > DEADZONE)
                moveX -= (stickX > 0.0f) ? 1 : -1;
            if (fabsf(stickY) > DEADZONE)
                moveY -= (stickY > 0.0f) ? 1 : -1;

            // D-pad
            if (isAnyGamepadButtonDown(keys.moveLeft.gamepad))
                moveX += 1;
            if (isAnyGamepadButtonDown(keys.moveRight.gamepad))
                moveX -= 1;
            if (isAnyGamepadButtonDown(keys.moveUp.gamepad))
                moveY -= 1;
            if (isAnyGamepadButtonDown(keys.moveDown.gamepad))
                moveY += 1;
        }
        else
        {
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

        if (moveX > 1)
            moveX = 1;
        if (moveX < -1)
            moveX = -1;
        if (moveY > 1)
            moveY = 1;
        if (moveY < -1)
            moveY = -1;
    }

    int InputSystem::getMoveX() const { return moveX; }
    int InputSystem::getMoveY() const { return moveY; }

    bool InputSystem::isMovePressed() const
    {
        return moveX != 0 || moveY != 0;
    }

    bool InputSystem::isBombPressed() const
    {
        const InputConfig &keys = config.getInputConfig();

        if (IsGamepadAvailable(0))
        {
            return isAnyGamepadButtonDown(keys.bomb.gamepad);
        }
        else
        {
            return isAnyKeyDown(keys.bomb.keyboard);
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
            return isAnyGamepadButtonPressed(keys.menuUp.gamepad);
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
            return isAnyGamepadButtonPressed(keys.menuDown.gamepad);
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
            return isAnyGamepadButtonPressed(keys.menuLeft.gamepad);
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
            return isAnyGamepadButtonPressed(keys.menuRight.gamepad);
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

} // namespace vovochka