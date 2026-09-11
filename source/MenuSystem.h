#pragma once

#include "SpriteSheet.h"
#include "FontSystem.h"
#include "InputSystem.h"
#include "raylib.h"
#include <string>
#include <unordered_map>

namespace vovochka
{

    enum class MenuScreen
    {
        GameTitle,
        Password,
        Main,
        Difficulty,
        Options,
        InGame,
        Pause,
        GameOver,
        Exit,
    };

    class MenuSystem
    {
    public:
        MenuSystem(const std::string &dataRoot);
        ~MenuSystem();

        void init();
        void shutdown();

        void setScreen(MenuScreen screen);
        MenuScreen currentScreen() const { return m_currentScreen; }

        void update(float dt, InputSystem &input);
        void render();

        void goGameOver(bool isQuitting);

        bool isQuitRequested() const { return m_quitRequested; }
        bool isStartGameRequested() const { return m_startGameRequested; }
        int selectedDifficulty() const { return m_selectedDifficulty; }

        void resetRequests();

    private:
        std::string m_dataRoot;
        MenuScreen m_currentScreen = MenuScreen::GameTitle;

        Texture2D m_backdrop{};
        int m_backdropW = 0, m_backdropH = 0;

        SpriteSheetManager m_menuGraphics;
        std::unordered_map<std::string, BitmapFont> m_fonts;
        std::unordered_map<std::string, Sound> m_menuSounds;

        bool m_quitRequested = false;
        bool m_startGameRequested = false;
        bool m_gameOverQuitting = false;
        int m_selectedItem = 0;
        int m_selectedDifficulty = 1;
        float m_gameMenuTimer = 0.0f;

        void loadMenuGraphics();
        void loadMenuSounds();
        const BitmapFont &font(const char *name) const;
        void updateBackdrop(int sw, int sh);
        void drawFon();
        void drawBackdrop();
        void drawScreen(MenuScreen screen);
        void drawGameTitle();
        void drawMainMenu();
        void drawDifficultyMenu();
        void drawExitMenu();
        void drawGameOver();
        void playMenuSound(const std::string &name);
    };

} // namespace vovochka