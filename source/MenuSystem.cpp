#include "MenuSystem.h"
#include "Utils.h"
#include <cmath>

namespace vovochka
{
    static const char *kFontMenu = "Font2";
    static const char *kFontMenuSel = "Font21";

    const MenuSystem::MenuItem MenuSystem::kMainItems[5] = {
        {"Играть", MenuScreen::Difficulty},
        {"Видео", MenuScreen::Main},   // заглушка: экран VIDEO позже
        {"Рекорды", MenuScreen::Main}, // заглушка: HIGHSCORETABLE позже
        {"Настройки", MenuScreen::Options},
        {"Выход", MenuScreen::Exit},
    };

    MenuSystem::MenuSystem(const std::string &dataRoot)
        : m_dataRoot(dataRoot)
    {
    }

    MenuSystem::~MenuSystem()
    {
        shutdown();
    }

    void MenuSystem::init()
    {
        for (const char *n : {"Font1", "Font2", "Font21", "Font3", "Font4", "Font5", "Font6"})
        {
            BitmapFont f;
            if (f.load(m_dataRoot + "/COMMON/FONT/" + n))
                m_fonts.emplace(n, std::move(f));
        }

        m_menuRT = LoadRenderTexture(800, 600);
        SetTextureFilter(m_menuRT.texture, TEXTURE_FILTER_BILINEAR);

        loadMenuGraphics();
        loadMenuSounds();
        createBlurTextures();

        setScreen(MenuScreen::GameTitle);
    }

    const BitmapFont &MenuSystem::font(const char *name) const
    {
        auto it = m_fonts.find(name);
        return (it != m_fonts.end()) ? it->second : m_fonts.begin()->second;
    }

    void MenuSystem::shutdown()
    {
        if (m_menuRT.texture.id != 0)
        {
            UnloadRenderTexture(m_menuRT);
            m_menuRT = {};
        }

        if (m_blurLeft.id != 0)
        {
            UnloadTexture(m_blurLeft);
            m_blurLeft = {};
        }

        if (m_blurRight.id != 0)
        {
            UnloadTexture(m_blurRight);
            m_blurRight = {};
        }

        m_menuGraphics.unload();

        for (auto &[name, s] : m_menuSounds)
            UnloadSound(s);
        m_menuSounds.clear();
    }

    void MenuSystem::loadMenuGraphics()
    {
        auto commonMenuDir = resolveDirCI(m_dataRoot, {"COMMON", "MENUGRAPHICS"});

        std::vector<std::string> menuDirs = {"GAMETITLE", "MAIN", "DIFFICULTY", "OPTIONS",
                                             "GAME", "GAMEOVER", "EXIT"};

        for (const auto &dir : menuDirs)
        {
            auto menuPath = resolveDirCI(m_dataRoot, {"MENU", dir.c_str()});
            if (menuPath.empty())
                continue;

            // Ищем .dat файл
            std::error_code ec;
            for (auto &e : std::filesystem::directory_iterator(menuPath, ec))
            {
                if (ec)
                    break;
                if (e.is_regular_file() && toLower(e.path().extension().string()) == ".dat")
                {
                    m_menuGraphics.loadFromIni(e.path().string(), commonMenuDir.string());
                    break;
                }
            }
        }

        // Также загружаем общие меню-графики
        auto commonMenuPath = resolveDirCI(m_dataRoot, {"COMMON", "MENUGRAPHICS"});
        if (!commonMenuPath.empty())
        {
            std::error_code ec;
            for (auto &e : std::filesystem::directory_iterator(commonMenuPath, ec))
            {
                if (ec)
                    break;
                if (e.is_regular_file() && toLower(e.path().extension().string()) == ".dat")
                {
                    m_menuGraphics.loadFromIni(e.path().string(), commonMenuPath.string());
                    break;
                }
            }
        }
    }

    void MenuSystem::loadMenuSounds()
    {
        auto soundDir = resolveDirCI(m_dataRoot, {"COMMON", "MENUSOUNDS"});
        if (soundDir.empty())
            return;

        std::error_code ec;
        for (auto &e : std::filesystem::directory_iterator(soundDir, ec))
        {
            if (ec)
                break;
            if (!e.is_regular_file())
                continue;

            std::string ext = toLower(e.path().extension().string());
            if (ext != ".wav")
                continue;

            std::string name = e.path().stem().string();
            Sound s = LoadSound(e.path().string().c_str());
            m_menuSounds[toLower(name)] = s;
        }
    }

    void MenuSystem::createBlurTextures()
    {
        // Получаем MenuFon из графики
        const SpriteSheetGPU *fon = m_menuGraphics.get("MenuFon");
        if (!fon || !fon->valid())
            return;

        // Создаём блюренные текстуры из крайних колонок
        const int blurWidth = 1;
        Image img = LoadImageFromTexture(fon->texture);

        // Левая колонка
        Image leftCrop = ImageFromImage(img, Rectangle{0, 0, (float)blurWidth, 600});
        ImageResize(&leftCrop, 400, 600);
        ImageBlurGaussian(&leftCrop, 16);
        m_blurLeft = LoadTextureFromImage(leftCrop);
        UnloadImage(leftCrop);

        // Правая колонка
        Image rightCrop = ImageFromImage(img, Rectangle{800 - blurWidth, 0, (float)blurWidth, 600});
        ImageResize(&rightCrop, 400, 600);
        ImageBlurGaussian(&rightCrop, 16);
        m_blurRight = LoadTextureFromImage(rightCrop);
        UnloadImage(rightCrop);

        UnloadImage(img);
    }

    void MenuSystem::setScreen(MenuScreen screen)
    {
        m_currentScreen = screen;
        m_selectedItem = 0;

        if (screen == MenuScreen::GameTitle)
        {
            m_gameTitleTimer = 0.0f;
            playMenuSound("OnTitle");
        }
    }

    void MenuSystem::update(float dt, InputSystem &input)
    {
        if (m_currentScreen == MenuScreen::GameTitle)
        {
            m_gameTitleTimer += dt;

            // Любая клавиша → Main
            if (input.isMenuConfirmPressed() ||
                input.isBombPressed() ||
                m_gameTitleTimer > 10.0f)
            {
                setScreen(MenuScreen::Main);
            }

            return;
        }

        if (m_currentScreen == MenuScreen::Main)
        {
            const int itemCount = 5;

            if (input.isMenuUpPressed())
            {
                m_selectedItem = (m_selectedItem - 1 + itemCount) % itemCount;
                playMenuSound("Move1");
            }
            else if (input.isMenuDownPressed())
            {
                m_selectedItem = (m_selectedItem + 1) % itemCount;
                playMenuSound("Move1");
            }

            if (input.isMenuConfirmPressed())
            {
                playMenuSound("Down1");
                setScreen(kMainItems[m_selectedItem].target);
            }

            return;
        }

        if (m_currentScreen == MenuScreen::Difficulty)
        {
            if (input.isMenuUpPressed())
            {
                m_selectedDifficulty = std::max(1, m_selectedDifficulty - 1);
                playMenuSound("Move1");
            }
            else if (input.isMenuDownPressed())
            {
                m_selectedDifficulty = std::min(3, m_selectedDifficulty + 1);
                playMenuSound("Move1");
            }

            if (input.isMenuConfirmPressed())
            {
                playMenuSound("Down1");
                m_startGameRequested = true;
                setScreen(MenuScreen::InGame);
            }

            if (input.isMenuCancelPressed())
            {
                setScreen(MenuScreen::Main);
            }

            return;
        }

        if (m_currentScreen == MenuScreen::Options)
        {
            // заглушка: назад по отмене/подтверждению
            if (input.isMenuCancelPressed() ||
                input.isMenuConfirmPressed())
            {
                playMenuSound("Down1");
                setScreen(MenuScreen::Main);
            }

            return;
        }

        if (m_currentScreen == MenuScreen::Exit)
        {
            if (input.isMenuConfirmPressed())
            {
                playMenuSound("OnEnd");
                m_quitRequested = true;
            }
            else if (input.isMenuCancelPressed())
            {
                playMenuSound("Down");
                setScreen(MenuScreen::Main);
            }

            return;
        }
    }

    void MenuSystem::render()
    {
        BeginTextureMode(m_menuRT);
        ClearBackground(BLACK);
        drawScreen(m_currentScreen);
        EndTextureMode();

        bool useFon = (m_currentScreen == MenuScreen::Main ||
                       m_currentScreen == MenuScreen::Difficulty ||
                       m_currentScreen == MenuScreen::Options ||
                       m_currentScreen == MenuScreen::Pause);

        BeginDrawing();
        ClearBackground(BLACK);
        present(useFon);
        EndDrawing();
    }

    void MenuSystem::drawScreen(MenuScreen screen)
    {
        switch (screen)
        {
        case MenuScreen::GameTitle:
            drawGameTitle();
            break;
        case MenuScreen::Main:
            drawMainMenu();
            break;
        case MenuScreen::Difficulty:
            drawDifficultyMenu();
            break;
        case MenuScreen::GameOver:
            drawGameOver();
            break;
        default:
            break;
        }
    }

    void MenuSystem::drawGameTitle()
    {
        if (const SpriteSheetGPU *s = m_menuGraphics.get("GameTitle"))
        {
            Rectangle src = s->frame(0);
            Rectangle dst{150.0f, 50.0f, 500.0f, 500.0f};
            DrawTexturePro(s->texture, src, dst, {0, 0}, 0.0f, WHITE);
        }
    }

    void MenuSystem::drawMainMenu()
    {
        if (const SpriteSheetGPU *s = m_menuGraphics.get("MenuFon"))
        {
            Rectangle src = s->frame(0);
            DrawTexturePro(s->texture, src, Rectangle{0, 0, 800, 600}, {0, 0}, 0.0f, WHITE);
        }

        constexpr float kCenterX = 400.0f;
        constexpr float kFirstY = 180.0f;
        constexpr float kLineH = 44.0f;

        for (int i = 0; i < 5; ++i)
        {
            const BitmapFont &f = (i == m_selectedItem) ? font(kFontMenuSel) : font(kFontMenu);
            const float y = kFirstY + i * kLineH;
            const float w = (float)f.textWidth(kMainItems[i].label);
            f.draw(kMainItems[i].label, kCenterX - w * 0.5f, y, WHITE);
        }
    }

    void MenuSystem::drawDifficultyMenu()
    {
        if (const SpriteSheetGPU *s = m_menuGraphics.get("MenuFon"))
        {
            Rectangle src = s->frame(0);
            Rectangle dst{0.0f, 0.0f, 800.0f, 600.0f};
            DrawTexturePro(s->texture, src, dst, {0, 0}, 0.0f, WHITE);
        }

        font(kFontMenu).draw("SELECT DIFFICULTY", 250.0f, 100.0f, WHITE);

        const char *items[] = {"EASY", "NORMAL", "HARD"};
        for (int i = 0; i < 3; ++i)
        {
            float y = 250.0f + i * 60.0f;
            Color tint = (i == m_selectedDifficulty - 1) ? YELLOW : WHITE;
            font(kFontMenu).draw(items[i], 350.0f, y, tint);
        }

        font(kFontMenu).draw("Press ENTER to start", 250.0f, 500.0f, WHITE);
    }

    void MenuSystem::drawGameOver()
    {
        if (const SpriteSheetGPU *s = m_menuGraphics.get("GameOver"))
        {
            Rectangle src = s->frame(0);
            Rectangle dst{150.0f, 50.0f, 500.0f, 500.0f};
            DrawTexturePro(s->texture, src, dst, {0, 0}, 0.0f, WHITE);
        }
    }

    void MenuSystem::present(bool useFon)
    {
        const int sw = GetScreenWidth();
        const int sh = GetScreenHeight();
        const float scale = (float)sh / 600.0f;
        const float w = 800.0f * scale;
        const float x0 = (sw - w) * 0.5f;

        if (useFon && m_blurLeft.id != 0 && m_blurRight.id != 0)
        {
            // Блюренные бока
            DrawTexturePro(m_blurLeft,
                           Rectangle{0, 0, 400, 600},
                           Rectangle{0.0f, 0.0f, x0 + 1, (float)sh},
                           {0, 0}, 0.0f, WHITE);
            DrawTexturePro(m_blurRight,
                           Rectangle{0, 0, 400, 600},
                           Rectangle{x0 + w - 1, 0.0f, (float)(sw - (x0 + w) + 1), (float)sh},
                           {0, 0}, 0.0f, WHITE);

            // Виньетка: темнее к внешним краям экрана
            DrawRectangleGradientH(0, 0, (int)x0, sh, ColorAlpha(BLACK, 0.45f), BLANK);
            DrawRectangleGradientH((int)(x0 + w), 0, (int)(sw - (x0 + w)), sh, BLANK, ColorAlpha(BLACK, 0.45f));
        }

        // Центр: меню 800x600, масштабированное по высоте
        DrawTexturePro(m_menuRT.texture,
                       Rectangle{1, 0, 800 - 2, -600},
                       Rectangle{x0 + 1, 0, w - 2, (float)sh},
                       {0, 0}, 0.0f, WHITE);
    }

    void MenuSystem::playMenuSound(const std::string &name)
    {
        auto it = m_menuSounds.find(toLower(name));
        if (it != m_menuSounds.end())
            PlaySound(it->second);
    }

    void MenuSystem::resetRequests()
    {
        m_quitRequested = false;
        m_startGameRequested = false;
    }

} // namespace vovochka