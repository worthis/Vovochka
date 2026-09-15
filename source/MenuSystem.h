#pragma once

#include "SpriteSheet.h"
#include "FontSystem.h"
#include "InputSystem.h"
#include "VideoPlayer.h"
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
        Video,
        VideoWindow,
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

        void enterPause();
        void goGameOver(bool isQuitting);

        bool openLevelVideo(int level);
        bool isResumeGameRequested() const { return m_resumeGameRequested; }

        bool isQuitRequested() const { return m_quitRequested; }
        bool isStartGameRequested() const { return m_startGameRequested; }
        int selectedDifficulty() const { return m_selectedDifficulty; }

        void resetRequests();

    private:
        std::string m_dataRoot;
        MenuScreen m_currentScreen = MenuScreen::GameTitle;

        Texture2D m_pauseBackdrop{};
        Texture2D m_backdrop{};
        int m_backdropW = 0, m_backdropH = 0;

        SpriteSheetManager m_menuGraphics;
        SpriteSheetManager m_videoGraphics;
        std::unordered_map<std::string, BitmapFont> m_fonts;
        std::unordered_map<std::string, Sound> m_menuSounds;

        VideoPlayer m_videoPlayer;

        bool m_videoFromLevel = false;
        bool m_resumeGameRequested = false;
        bool m_quitRequested = false;
        bool m_startGameRequested = false;
        bool m_gameOverQuitting = false;
        int m_selectedItem = 0;
        int m_selectedDifficulty = 1;
        float m_gameMenuTimer = 0.0f;

        Vector2 m_lastCursorPos = {0.0f, 0.0f};
        bool m_cursorActive = false;
        bool m_cursorVisible = false;
        bool m_hoverBackBtn = false;
        bool m_pressBackBtn = false;

        void loadMenuGraphics();
        void loadMenuSounds();
        void loadVideoGraphics();

        const BitmapFont &font(const char *name) const;
        void updateBackdrop(int sw, int sh);

        void resumeGame();
        void releasePauseBackdrop();
        void activatePauseItem(int idx);

        void drawFon();
        void drawBackdrop();
        void drawScreen(MenuScreen screen);
        void drawGameTitle();
        void drawMainMenu();
        void drawDifficultyMenu();
        void drawVideoMenu();
        void drawVideoWindow();
        void drawExitMenu();
        void drawGameOver();
        void drawPauseMenu();
        void drawPauseBackdrop();
        void drawCursor();

        int hitTestMainMenu(float mx, float my) const;
        int hitTestDifficulty(float mx, float my) const;
        int hitTestVideo(float mx, float my) const;
        int hitTestExit(float mx, float my) const;
        bool hitTestBackBtn(float mx, float my) const;
        bool hitTestBackBtnVideo(float mx, float my) const;
        int hitTestPause(float mx, float my) const;

        void playMenuSound(const std::string &name);
    };

} // namespace vovochka