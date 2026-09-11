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

        bool isQuitRequested() const { return m_quitRequested; }
        bool isStartGameRequested() const { return m_startGameRequested; }
        int selectedDifficulty() const { return m_selectedDifficulty; }

        void resetRequests();

    private:
        struct MenuItem
        {
            const char *label;
            MenuScreen target; // куда переходим по подтверждению
        };

        std::string m_dataRoot;
        MenuScreen m_currentScreen = MenuScreen::GameTitle;

        RenderTexture2D m_menuRT{};

        SpriteSheetManager m_menuGraphics;

        Texture2D m_blurLeft{};
        Texture2D m_blurRight{};

        std::unordered_map<std::string, BitmapFont> m_fonts;

        static const MenuItem kMainItems[5];
 
        std::unordered_map<std::string, Sound> m_menuSounds;

        int m_selectedItem = 0;
        bool m_quitRequested = false;
        bool m_startGameRequested = false;
        int m_selectedDifficulty = 1;
        float m_gameTitleTimer = 0.0f;

        void loadMenuGraphics();
        void loadMenuSounds();
        void createBlurTextures();

        const BitmapFont &font(const char *name) const;

        void drawScreen(MenuScreen screen);
        void drawGameTitle();
        void drawMainMenu();
        void drawDifficultyMenu();
        void drawGameOver();

        void present(bool useFon);

        void playMenuSound(const std::string &name);
    };

} // namespace vovochka