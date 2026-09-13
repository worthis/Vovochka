#include "MenuSystem.h"
#include "IniReader.h"
#include "SaveSystem.h"
#include "Utils.h"
#include <cmath>

namespace vovochka
{
    static const char *kFontMenu = "Font2";
    static const char *kFontMenuSel = "Font21";
    static const char *kFontVideoAvail = "Font1";
    static const char *kFontVideoFull = "Font4";
    static constexpr float kFonW = 800.0f;
    static constexpr float kFonH = 600.0f;
    static constexpr float kFonInset = 1.0f;                 // срезаем по 1 px с каждой стороны
    static constexpr float kFonVisW = kFonW - 2 * kFonInset; // 798
    static constexpr float kFonVisH = kFonH - 2 * kFonInset; // 598
    static constexpr float kBackBtnX = 30.0f;
    static constexpr float kBackBtnY = 505.0f;

    static const std::pair<const char *, MenuScreen> kMainItems[5] = {
        {"Играть", MenuScreen::Difficulty},
        {"Видео", MenuScreen::Video},
        {"Рекорды", MenuScreen::Main},
        {"Настройки", MenuScreen::Main},
        {"Выход", MenuScreen::Exit},
    };

    static const char *kDifficultyLabels[3] = {
        "Салага",  // 1 = easy
        "Бывалый", // 2 = normal
        "Бабник",  // 3 = hard
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

        loadVideoGraphics();
        loadVideoDurations();
        loadMenuGraphics();
        loadMenuSounds();

        setScreen(MenuScreen::GameTitle);
    }

    const BitmapFont &MenuSystem::font(const char *name) const
    {
        auto it = m_fonts.find(name);
        return (it != m_fonts.end()) ? it->second : m_fonts.begin()->second;
    }

    void MenuSystem::shutdown()
    {
        if (m_backdrop.id != 0)
        {
            UnloadTexture(m_backdrop);
            m_backdrop = {};
        }

        m_videoPlayer.close();

        m_videoGraphics.unload();
        m_menuGraphics.unload();

        for (auto &[name, f] : m_fonts)
            f.unload();
        m_fonts.clear();

        for (auto &[name, s] : m_menuSounds)
            UnloadSound(s);
        m_menuSounds.clear();
    }

    void MenuSystem::loadVideoGraphics()
    {
        for (const char *dir : {"VIDEO", "VIDEOWINDOW"})
        {
            auto p = resolveDirCI(m_dataRoot, {"MENU", dir});
            if (p.empty())
                continue;
            std::error_code ec;
            for (auto &e : std::filesystem::directory_iterator(p, ec))
            {
                if (ec)
                    break;
                if (e.is_regular_file() && toLower(e.path().extension().string()) == ".dat")
                {
                    m_videoGraphics.loadFromIni(e.path().string(), p.string());
                    break;
                }
            }
        }
    }

    void MenuSystem::loadVideoDurations()
    {
        IniReader ini;
        const std::string path = m_dataRoot + "/COMMON/GAMECONFIGINF/VideoDuration.dat";
        if (!ini.loadFile(path))
            return;

        m_videoFullSec.assign(12, 0);
        for (int i = 1; i <= 12; ++i)
            m_videoFullSec[i - 1] = ini.getInt("1", std::to_string(i), 0);
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

    void MenuSystem::updateBackdrop(int sw, int sh)
    {
        if (m_backdrop.id != 0 &&
            m_backdropW == sw &&
            m_backdropH == sh)
            return;

        if (m_backdrop.id != 0)
        {
            UnloadTexture(m_backdrop);
            m_backdrop = {};
        }

        const SpriteSheetGPU *fon = m_menuGraphics.get("MenuFon");
        if (!fon || !fon->valid())
            return;

        constexpr float k = 0.5f;
        const int hw = std::max(1, sw / 2);
        const int hh = std::max(1, sh / 2);
        const float x0 = (float)std::max(0, (sw - (int)kFonW) / 2);
        const float y0 = (float)std::max(0, (sh - (int)kFonH) / 2);

        RenderTexture2D rt = LoadRenderTexture(hw, hh);

        BeginTextureMode(rt);
        ClearBackground(BLACK);

        // src — ВСЕГДА в пикселях фона (800x600); dst — в экранных, умноженных на k
        auto band = [&fon, k](Rectangle src, float dx, float dy, float dw, float dh)
        {
            if (dw <= 0.0f || dh <= 0.0f)
                return;
            Rectangle dst{dx * k, dy * k, dw * k, dh * k};
            DrawTexturePro(fon->texture, src, dst, {0, 0}, 0.0f, WHITE);
        };

        // Границы ВИДИМОЙ части фона на экране
        const float vx = x0 + kFonInset;         // левый край (колонка 1 фона)
        const float vy = y0 + kFonInset;         // верхний край (строка 1)
        const float rx = x0 + kFonW - kFonInset; // правый край (x колонки 799 фона)
        const float by = y0 + kFonH - kFonInset; // нижний край (y строки 599)

        // Бока: колонки 1 и 798, высота = видимой части фона
        band(Rectangle{1, 1, 1, kFonH - 2}, 0.0f, vy, vx, kFonVisH);                   // лево (включая рамку x0)
        band(Rectangle{kFonW - 2, 1, 1, kFonH - 2}, rx, vy, (float)sw - rx, kFonVisH); // право (с колонки 799)
        // Верх: строка 1 — середина над фоном + углы из пикселя (1,1)/(798,1)
        band(Rectangle{1, 1, kFonW - 2, 1}, vx, 0.0f, kFonVisW, vy);       // верх середина
        band(Rectangle{1, 1, 1, 1}, 0.0f, 0.0f, vx, vy);                   // угол ЛВ
        band(Rectangle{kFonW - 2, 1, 1, 1}, rx, 0.0f, (float)sw - rx, vy); // угол ПВ
        // Низ: строка 598 — середина под фоном + углы из (1,598)/(798,598)
        band(Rectangle{1, kFonH - 2, kFonW - 2, 1}, vx, by, kFonVisW, (float)sh - by);       // низ середина
        band(Rectangle{1, kFonH - 2, 1, 1}, 0.0f, by, vx, (float)sh - by);                   // угол ЛН
        band(Rectangle{kFonW - 2, kFonH - 2, 1, 1}, rx, by, (float)sw - rx, (float)sh - by); // угол ПН
        // Центр: видимая часть фона (область за ним тоже блюрится)
        band(Rectangle{1, 1, kFonW - 2, kFonH - 2}, vx, vy, kFonVisW, kFonVisH);

        EndTextureMode();

        Image comp = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&comp);
        UnloadRenderTexture(rt);

        ImageBlurGaussian(&comp, 2);

        m_backdrop = LoadTextureFromImage(comp);
        SetTextureFilter(m_backdrop, TEXTURE_FILTER_BILINEAR);
        UnloadImage(comp);

        m_backdropW = sw;
        m_backdropH = sh;

        TraceLog(LOG_INFO, "Menu backdrop rebuilt: %dx%d", sw, sh);
    }

    void MenuSystem::drawBackdrop()
    {
        if (m_backdrop.id == 0)
            return;

        DrawTexturePro(m_backdrop,
                       Rectangle{0, 0, (float)m_backdrop.width, (float)m_backdrop.height},
                       Rectangle{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                       {0, 0}, 0.0f, WHITE);
    }

    void MenuSystem::drawFon()
    {
        const SpriteSheetGPU *s = m_menuGraphics.get("MenuFon");
        if (!s)
            return;

        Rectangle src{kFonInset, kFonInset, kFonVisW, kFonVisH};
        Rectangle dst{kFonInset, kFonInset, kFonVisW, kFonVisH};
        DrawTexturePro(s->texture, src, dst, {0, 0}, 0.0f, WHITE);
    }

    void MenuSystem::setScreen(MenuScreen screen)
    {
        m_currentScreen = screen;
        m_selectedItem = 0;
        m_gameMenuTimer = 0.0f;

        if (screen == MenuScreen::GameTitle)
        {
            playMenuSound("OnTitle");
        }
    }

    void MenuSystem::update(float dt, InputSystem &input)
    {
        if (m_currentScreen == MenuScreen::GameTitle)
        {
            m_gameMenuTimer += dt;

            if (input.isMenuConfirmPressed() ||
                input.isBombPressed() ||
                m_gameMenuTimer > 5.0f)
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
                setScreen(kMainItems[m_selectedItem].second);
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
                playMenuSound("Down1");
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

        if (m_currentScreen == MenuScreen::Video)
        {
            int sel = m_selectedItem;
            if (input.isMenuLeftPressed())
                sel = (sel + 11) % 12;
            else if (input.isMenuRightPressed())
                sel = (sel + 1) % 12;
            else if (input.isMenuUpPressed())
                sel = (sel + 8) % 12;
            else if (input.isMenuDownPressed())
                sel = (sel + 4) % 12;
            if (sel != m_selectedItem)
            {
                m_selectedItem = sel;
                playMenuSound("Move1");
            }

            if (input.isMenuConfirmPressed())
            {
                playMenuSound("Down1");

                const std::string path = m_dataRoot + "/VIDEO/video" +
                                         std::to_string(m_selectedItem + 1) + ".mpg";
                const int avail = videoAvailSec(m_selectedItem);
                if (avail > 0 && m_videoPlayer.open(path, (float)avail))
                    setScreen(MenuScreen::VideoWindow);
                else
                    TraceLog(LOG_WARNING, "Video: unavailable or missing file");
            }
            else if (input.isMenuCancelPressed())
            {
                playMenuSound("Down1");
                setScreen(MenuScreen::Main);
            }

            return;
        }

        if (m_currentScreen == MenuScreen::VideoWindow)
        {
            m_videoPlayer.update(dt);

            if (input.isMenuCancelPressed() || m_videoPlayer.finished())
            {
                m_videoPlayer.close();
                playMenuSound("Down1");
                setScreen(MenuScreen::Video);
            }

            return;
        }

        if (m_currentScreen == MenuScreen::Exit)
        {
            // Горизонтальный выбор из двух пунктов
            if ((input.isMenuLeftPressed() || input.isMenuUpPressed()) && m_selectedItem != 0)
            {
                m_selectedItem = 0;
                playMenuSound("Move1");
            }
            else if ((input.isMenuRightPressed() || input.isMenuDownPressed()) && m_selectedItem != 1)
            {
                m_selectedItem = 1;
                playMenuSound("Move1");
            }
            else if (input.isMenuConfirmPressed())
            {
                if (m_selectedItem == 0)
                {
                    playMenuSound("Down1");
                    setScreen(MenuScreen::Main); // "Нет"
                }
                else
                {
                    playMenuSound("OnEnd");
                    goGameOver(true); // "Да" -> GameOver -> выход
                }
            }
            else if (input.isMenuCancelPressed())
            {
                playMenuSound("Down1");
                setScreen(MenuScreen::Main); // отмена = "Нет"
            }
            return;
        }

        if (m_currentScreen == MenuScreen::GameOver)
        {
            m_gameMenuTimer += dt;

            if (input.isMenuConfirmPressed() ||
                input.isBombPressed() ||
                m_gameMenuTimer > 5.0f)
            {
                if (m_gameOverQuitting)
                    m_quitRequested = true; // выход из игры
                else
                    setScreen(MenuScreen::Main); // обратно в главное меню
            }
            return;
        }
    }

    void MenuSystem::render()
    {
        const int sw = GetScreenWidth();
        const int sh = GetScreenHeight();
        const int x0 = std::max(0, (sw - 800) / 2);
        const int y0 = std::max(0, (sh - 600) / 2);

        bool useFon = (m_currentScreen == MenuScreen::Main ||
                       m_currentScreen == MenuScreen::Difficulty ||
                       m_currentScreen == MenuScreen::Options ||
                       m_currentScreen == MenuScreen::Pause);

        if (useFon)
            updateBackdrop(sw, sh);

        BeginDrawing();
        ClearBackground(BLACK);

        // Заливки вокруг фона — в экранных координатах
        if (useFon)
            drawBackdrop();

        // Контент меню: родные 800x600, без масштаба, со смещением в центр
        Camera2D cam{};
        cam.offset = {(float)x0, (float)y0};
        cam.target = {0, 0};
        cam.rotation = 0.0f;
        cam.zoom = 1.0f;

        BeginMode2D(cam);
        drawScreen(m_currentScreen);
        EndMode2D();

        EndDrawing();
    }

    void MenuSystem::goGameOver(bool isQuitting)
    {
        m_gameOverQuitting = isQuitting;
        setScreen(MenuScreen::GameOver);
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
        case MenuScreen::Video:
            drawVideoMenu();
            break;
        case MenuScreen::VideoWindow:
            drawVideoWindow();
            break;
        case MenuScreen::Exit:
            drawExitMenu();
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
        drawFon();

        constexpr float kCenterX = 400.0f;
        constexpr float kFirstY = 180.0f;
        constexpr float kLineH = 44.0f;

        for (int i = 0; i < (int)std::size(kMainItems); ++i)
        {
            const BitmapFont &f = (i == m_selectedItem) ? font(kFontMenuSel) : font(kFontMenu);
            const float y = kFirstY + i * kLineH;
            const float w = (float)f.textWidth(kMainItems[i].first);
            f.draw(kMainItems[i].first, kCenterX - w * 0.5f, y);
        }
    }

    void MenuSystem::drawDifficultyMenu()
    {
        drawFon();

        for (int i = 0; i < 3; ++i)
        {
            const BitmapFont &f = (i == m_selectedDifficulty - 1) ? font(kFontMenuSel) : font(kFontMenu);
            const float y = 237.0f + i * 50.0f;
            const float w = (float)f.textWidth(kDifficultyLabels[i]);
            f.draw(kDifficultyLabels[i], 400.0f - w * 0.5f, y);
        }

        // Кнопка "назад" в левом нижнем углу (кадр 0 = обычное состояние)
        if (const SpriteSheetGPU *b = m_menuGraphics.get("BackBtn"))
        {
            Rectangle src = b->frame(0);
            DrawTexturePro(b->texture, src,
                           Rectangle{kBackBtnX, kBackBtnY, (float)b->patternW, (float)b->patternH},
                           {0, 0}, 0.0f, WHITE);
        }
    }

    void MenuSystem::drawExitMenu()
    {
        if (const SpriteSheetGPU *s = m_menuGraphics.get("GameQuit"))
        {
            Rectangle src = s->frame(0);
            DrawTexturePro(s->texture, src, Rectangle{150, 50, 500, 500}, {0, 0}, 0.0f, WHITE);
        }

        const char *title = "Уже уходишь?";
        const char *kExitOptLabels[2] = {"Нет", "Да"};
        const float kExitOptCX[2] = {200.0f, 605.0f};
        const BitmapFont &f = font(kFontMenu);
        const float w = (float)f.textWidth(title);
        f.draw(title, 400.0f - w * 0.5f, 90.0f);

        for (int i = 0; i < 2; ++i)
        {
            const BitmapFont &f = (i == m_selectedItem) ? font(kFontMenuSel) : font(kFontMenu);
            const float w = (float)f.textWidth(kExitOptLabels[i]);
            f.draw(kExitOptLabels[i], kExitOptCX[i] - w * 0.5f, 190.0f);
        }
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

    void MenuSystem::drawVideoMenu()
    {
        if (const SpriteSheetGPU *s = m_videoGraphics.get("FilmMenu"))
            DrawTexturePro(s->texture, s->frame(0), Rectangle{0, 0, 800, 600}, {0, 0}, 0.0f, WHITE);

        const float gx = 170.0f, gy = 113.0f, cw = 115.0f, ch = 111.0f;
        float selX = gx, selY = gy;

        for (int i = 0; i < 12; ++i)
        {
            const float x = gx + (i % 4) * cw;
            const float y = gy + (i / 4) * ch;

            // превью уровня (секции "1".."12" в VideoMenu.dat)
            const std::string name = std::to_string(i + 1);
            if (const SpriteSheetGPU *c = m_videoGraphics.get(name.c_str()))
                DrawTexturePro(c->texture, c->frame(0), Rectangle{x, y, cw, ch}, {0, 0}, 0.0f, WHITE);

            // времена: розовое = доступно, голубое = полное
            const int full = (i < (int)m_videoFullSec.size()) ? m_videoFullSec[i] : 0;
            const int avail = videoAvailSec(i);

            font(kFontVideoAvail).draw(formatTime(avail), x + 2.0f, y + 2.0f);
            const std::string fs = formatTime(full);
            const float fw = (float)font(kFontVideoFull).textWidth(fs);
            font(kFontVideoFull).draw(fs, x + cw - 2.0f - fw, y + 2.0f);

            if (i == m_selectedItem)
            {
                selX = x;
                selY = y;
            }
        }

        DrawRectangleLinesEx(Rectangle{selX - 2.0f, selY - 2.0f, cw + 4.0f, ch + 4.0f}, 3.0f, PINK);

        // локальная кнопка "Выход"
        if (const SpriteSheetGPU *b = m_videoGraphics.get("BackBtn"))
            DrawTexturePro(b->texture, b->frame(0),
                           Rectangle{35.0f, 365.0f, (float)b->patternW, (float)b->patternH},
                           {0, 0}, 0.0f, WHITE);
    }

    void MenuSystem::drawVideoWindow()
    {
        if (const SpriteSheetGPU *s = m_videoGraphics.get("FilmFon"))
            DrawTexturePro(s->texture, s->frame(0), Rectangle{0, 0, 800, 600}, {0, 0}, 0.0f, WHITE);

        // Видео 512x384 по центру «экрана кинотеатра»
        m_videoPlayer.draw(Rectangle{144.0f, 108.0f, 512.0f, 384.0f});

        // Прогресс: просмотрено / доступно
        const float cap = m_videoPlayer.capSec();
        if (cap > 0.0f)
        {
            const float frac = std::clamp(m_videoPlayer.timePlayed() / cap, 0.0f, 1.0f);
            DrawRectangle(144, 492, 512, 8, Color{30, 30, 30, 255});
            DrawRectangle(144, 492, (int)(512.0f * frac), 8, Color{255, 64, 160, 255});
        }

        if (const SpriteSheetGPU *b = m_videoGraphics.get("BackBtn"))
            DrawTexturePro(b->texture, b->frame(0),
                           Rectangle{35.0f, 365.0f, (float)b->patternW, (float)b->patternH},
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

    int MenuSystem::videoAvailSec(int idx) const
    {
        if (idx < 0 || idx >= 12 || idx >= (int)m_videoFullSec.size())
            return 0;

        // Доступное время считаем из ЛУЧШИХ очков уровня из save-файла
        const int score = SaveSystem::instance().levelScore(idx + 1);
        return (m_videoFullSec[idx] * score + 99) / 100; // реконструкция: ceil(full*score/100)
    }

    std::string MenuSystem::formatTime(int sec)
    {
        char buf[16];
        const char *kTimeFormat = "%d:%02d";
        std::snprintf(buf, sizeof(buf), kTimeFormat, sec / 60, sec % 60);
        return buf;
    }

} // namespace vovochka