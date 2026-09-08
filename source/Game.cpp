#include "Game.h"
#include "SpriteLayout.h"
#include "InputSystem.h"
#include "ConfigSystem.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include "raylib.h"

std::string toLower(std::string s)
{
    for (auto &c : s)
        c = static_cast<char>(std::tolower((unsigned char)c));
    return s;
}

// регистронезависимый поиск подпапки по частям пути
std::filesystem::path resolveDirCI(const std::filesystem::path &base,
                                   std::initializer_list<const char *> parts)
{
    namespace fs = std::filesystem;
    fs::path cur = base;
    for (const char *part : parts)
    {
        bool found = false;
        std::error_code ec;
        for (auto &e : fs::directory_iterator(cur, ec))
        {
            if (ec)
                break;
            if (e.is_directory() && toLower(e.path().filename().string()) == toLower(part))
            {
                cur = e.path();
                found = true;
                break;
            }
        }
        if (!found)
            return {};
    }
    return cur;
}

namespace vovochka
{

    Game::Game(std::string dataRoot) : m_dataRoot(std::move(dataRoot)) {}
    Game::~Game() { shutdown(); }

    bool Game::init(int screenW, int screenH, const char *title)
    {
        ConfigSystem::instance().loadSettings("settings.json");

        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
        InitWindow(screenW, screenH, title);
        InitAudioDevice();
        SetTargetFPS(60);

        m_camera.zoom = 1.0f;
        m_camera.rotation = 0.0f;
        m_camera.offset = {screenW * 0.5f, screenH * 0.5f};
        m_camera.target = {0, 0};

        setLevel(m_currentLevel);
        m_running = true;
        return true;
    }

    void Game::shutdown()
    {
        if (m_running)
        {
            m_renderer.unload();
            unloadSounds();
            if (IsAudioDeviceReady())
                CloseAudioDevice();
            CloseWindow();
            m_running = false;
        }
    }

    void Game::setLevel(int n)
    {
        m_stats = Stats{};
        m_stats.healthMax = m_diff.playerLifeMax;
        m_stats.health = m_stats.healthMax;
        m_stats.powerMax = 6;
        m_stats.power = m_stats.powerMax;
        m_stats.score = 0;
        m_intimacy = IntimacySession{};
        m_laughTimer = 0.0f;
        m_exitActive = false;
        m_hurtTimer = 0.0f;
        m_deathTimer = 0.0f;
        m_bossGrabCooldown = 0.0f;
        m_currentLevel = std::clamp(n, 1, 12);

        m_renderer.unload();

        if (!m_loader.loadMap(m_currentLevel, m_map))
        {
            TraceLog(LOG_ERROR, "Failed to load Level%d.map", m_currentLevel);
            return;
        }
        if (!m_renderer.loadLevelAssets(m_loader, m_currentLevel))
        {
            TraceLog(LOG_WARNING, "Some tilesets of Level%d failed to load", m_currentLevel);
        }

        loadSounds();

        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());

        m_player.init(m_map,
                      m_renderer.sheet("PlayerStand0"),
                      m_renderer.sheet("PlayerGo"),
                      m_renderer.sheet("PlayerMakeBomb"),
                      m_renderer.sheet("PlayerUndoAttack"),
                      tw, th, m_diff.playerSpeed * kSpeedScale);

        m_loader.loadDifficulty(m_difficulty, m_diff);

        buildFreePoints();
        spawnCondoms();
        spawnEnemies();
        spawnLevelExit();

        m_bombs.clear();
        m_entities.clear();
        for (const auto &o : m_map.objects)
        {
            if (o.type != MapObjectType::Girl)
                continue;
            PreviewEntity e;
            e.type = o.type;
            e.tileX = o.x;
            e.tileY = o.y;
            e.used = false;
            e.anim.sheet = m_renderer.sheet("Girl1Wait");
            e.anim.setBlock(SpriteLayout::girlWait(true, e.anim.sheet ? e.anim.sheet->frameCount : 2));
            if (e.anim.sheet)
            {
                e.x = o.x * tw + (tw - e.anim.sheet->patternW) * 0.5f;
                e.baseY = (o.y + 1) * th - e.anim.sheet->patternH - th * 0.4f;
                m_entities.push_back(std::move(e));
            }
        }

        updateCamera();
    }

    void Game::updateCamera()
    {
        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());
        const float mapW = m_map.width * tw;
        const float mapH = m_map.height * th;

        const float sw = static_cast<float>(GetScreenWidth());
        const float sh = static_cast<float>(GetScreenHeight());

        // актуальный offset каждый кадр (лечит «улёт» при ресайзе)
        m_camera.offset = {sw * 0.5f, sh * 0.5f};

        Vector2 p = m_player.getPixelPos();
        float tx = p.x + 52.0f; // центр спрайта (~104/2)
        float ty = p.y + 52.0f;

        if (mapW <= sw)
            tx = mapW * 0.5f;
        else
            tx = std::clamp(tx, sw * 0.5f, mapW - sw * 0.5f);

        if (mapH <= sh)
            ty = mapH * 0.5f;
        else
            ty = std::clamp(ty, sh * 0.5f, mapH - sh * 0.5f);

        m_camera.target = {tx, ty};
    }

    void Game::processSystemInput()
    {
        // Пауза/выход
        if (m_input.isPausePressed())
        {
            m_running = false;
            return;
        }

        // Отладочные клавиши (оставляем прямыми, это не основное управление)
        if (IsKeyPressed(KEY_G))
            m_debugGrid = !m_debugGrid;

        if (IsKeyPressed(KEY_E))
            setLevel(m_currentLevel + 1);

        if (IsKeyPressed(KEY_Q))
            setLevel(m_currentLevel - 1);

        if (IsKeyPressed(KEY_ONE))
        {
            m_difficulty = 1;
            setLevel(m_currentLevel);
        }

        if (IsKeyPressed(KEY_TWO))
        {
            m_difficulty = 2;
            setLevel(m_currentLevel);
        }

        if (IsKeyPressed(KEY_THREE))
        {
            m_difficulty = 3;
            setLevel(m_currentLevel);
        }
    }

    void Game::update(float dt)
    {
        // Сбор ввода
        m_input.update();

        // Системный ввод (отладка, смена уровня)
        processSystemInput();

        // Гибель игрока: пауза и рестарт уровня
        if (m_deathTimer > 0.0f)
        {
            m_deathTimer -= dt;
            if (m_deathTimer <= 0.0f)
            {
                setLevel(m_currentLevel);
                return;
            }
        }

        // Проверка условий создания бомбы
        bool wantBomb = checkBombConditions();

        // Обновление игрока с вводом
        m_player.setInputEnabled(!m_intimacy.active && !m_bossIntimacy.active && m_deathTimer <= 0.0f);
        m_player.update(dt, m_map, m_input.getMoveX(), m_input.getMoveY(), wantBomb);

        // Камера
        updateCamera();

        // Геймплей
        updateGameplay(dt);
    }

    void Game::startIntimacy(int idx)
    {
        m_intimacy.active = true;
        m_intimacy.t = 0.0f;
        m_intimacy.girlIdx = idx;
        m_intimacy.powerStart = static_cast<float>(m_stats.power);

        auto &g = m_entities[idx];
        if (const SpriteSheetGPU *act = m_renderer.sheet("Girl1Action1"))
        {
            g.anim.sheet = act;
            g.anim.setBlock(FrameBlock{0, act->frameCount, true});
            g.anim.frameTime = 0.08f;
        }

        m_intimacy.duration = std::max({m_intimacy.duration,
                                        soundDuration("KISS"),
                                        soundDuration("GirlScream")});

        playSound("GirlScream");
        playSound("KISS");
    }

    void Game::endIntimacy()
    {
        auto &g = m_entities[m_intimacy.girlIdx];
        g.used = true;
        if (const SpriteSheetGPU *wait = m_renderer.sheet("Girl1Wait"))
        {
            g.anim.sheet = wait;
            g.anim.setBlock(SpriteLayout::girlWait(false, wait->frameCount));
        }

        m_intimacy.active = false;
    }

    void Game::startBossIntimacy(int idx)
    {
        m_bossIntimacy.active = true;
        m_bossIntimacy.t = 0.0f;
        m_bossIntimacy.girlIdx = idx;
        m_bossIntimacy.powerStart = static_cast<float>(m_stats.power);
        m_bossIntimacy.duration = std::max({3.326f, soundDuration("KISS")});

        auto &b = m_enemies[idx];
        b.placeAt(m_player.getTileX(), m_player.getTileY());
        b.startAttack(m_player.getPixelPos().x >= b.getPixelPos().x, true);

        playSound("attak");
        playSound("KISS");
    }

    void Game::endBossIntimacy()
    {
        m_bossIntimacy.active = false;
        m_bossGrabCooldown = kBossGrabCooldown;
        auto &b = m_enemies[m_bossIntimacy.girlIdx];
        b.stopAttack();

        damagePlayer(1);

        playSound("PlayerCackle");
    }

    bool Game::checkBombConditions()
    {
        if (!m_input.isBombPressed())
            return false;

        if (m_intimacy.active ||
            m_bossIntimacy.active)
            return false;

        if (m_deathTimer > 0.0f)
            return false;

        if (m_player.isMakingBomb() ||
            m_player.isHurt())
            return false;

        if (m_player.isClimbing())
            return false;

        if (m_stats.power < m_diff.bombCost)
            return false;

        m_stats.power -= m_diff.bombCost;
        playSound("Breath");

        return true;
    }

    void Game::updateGameplay(float dt)
    {
        Rectangle pr = m_player.getBounds();

        // --- Близость с девушкой: слив Power, рост Score ---
        if (m_intimacy.active)
        {
            m_intimacy.t += dt;
            const float k = std::min(1.0f, m_intimacy.t / m_intimacy.duration);
            m_stats.power = (int)std::ceil(m_intimacy.powerStart * (1.0f - k));
            m_stats.score = std::min(100.0f, m_stats.score + 2.0f * m_intimacy.powerStart * dt / m_intimacy.duration);
            if (k >= 1.0f)
                endIntimacy();
        }

        // --- Близость с боссом: слив Power, падение Score ---
        if (m_bossIntimacy.active)
        {
            m_bossIntimacy.t += dt;
            const float k = std::min(1.0f, m_bossIntimacy.t / m_bossIntimacy.duration);
            m_stats.power = (int)std::ceil(m_bossIntimacy.powerStart * (1.0f - k));
            m_stats.score = std::max(0.0f, m_stats.score - 2.0f * m_bossIntimacy.powerStart * dt / m_bossIntimacy.duration);
            if (k >= 1.0f)
                endBossIntimacy();
        }

        // --- Подбор презервативов ---
        if (!m_intimacy.active &&
            !m_bossIntimacy.active)
        {
            const SpriteSheetGPU *cs = m_renderer.sheet("Condom");
            const float cw = cs ? (float)cs->patternW : 28.0f;
            const float ch = cs ? (float)cs->patternH : 28.0f;
            for (auto it = m_condoms.begin(); it != m_condoms.end();)
            {
                Rectangle cr{it->x, it->y, cw, ch};
                if (m_stats.power < m_stats.powerMax &&
                    CheckCollisionRecs(pr, cr))
                {
                    ++m_stats.power;
                    playSound("TakeCondom");
                    if (m_stats.power >= m_stats.powerMax)
                        playSound("FullStrength");
                    if (--it->count <= 0)
                        it = m_condoms.erase(it);
                    else
                        ++it;
                    continue;
                }
                ++it;
            }
        }

        // --- Девушки: обновление
        m_laughTimer = std::max(0.0f, m_laughTimer - dt);
        for (auto &e : m_entities)
            e.anim.update(dt);

        // --- Девушки: попытка близости ---
        if (!m_intimacy.active &&
            !m_bossIntimacy.active)
        {
            for (size_t i = 0; i < m_entities.size(); ++i)
            {
                const auto &g = m_entities[i];
                if (g.type != MapObjectType::Girl || g.used)
                    continue;

                Rectangle vb = g.anim.getVisibleBounds();
                Rectangle gr{g.x + vb.x, g.baseY + vb.y, vb.width, vb.height};

                if (!CheckCollisionRecs(pr, gr))
                    continue;

                if (m_stats.power >= m_diff.playerStrengthCan &&
                    !m_player.isHurt())
                {
                    startIntimacy(static_cast<int>(i));
                    break;
                }

                if (m_laughTimer <= 0.0f)
                {
                    m_laughTimer = 4.0f;
                    playSound("GirlLaugh");
                }
            }
        }

        // --- Враги: ИИ и коллизии ---
        m_hurtTimer = std::max(0.0f, m_hurtTimer - dt);
        m_bossGrabCooldown = std::max(0.0f, m_bossGrabCooldown - dt);

        for (auto &e : m_enemies)
        {
            if (e.isBoss &&
                !m_bossIntimacy.active)
            {
                e.bossThink(dt, m_map,
                            m_player.getTileX(), m_player.getTileY(),
                            m_stats.power, m_diff.playerStrengthCan);
            }
            e.update(dt, m_map);
        }

        // --- Контакт враг-игрок ---
        if (m_deathTimer <= 0.0f &&
            m_hurtTimer <= 0.0f &&
            !m_bossIntimacy.active &&
            !m_player.isHurt())
        {
            for (size_t i = 0; i < m_enemies.size(); ++i)
            {
                auto &e = m_enemies[i];
                if (!e.alive)
                    continue;

                if (!CheckCollisionRecs(pr, e.getBounds()))
                    continue;

                m_player.cancelMakeBomb();

                if (m_intimacy.active)
                    endIntimacy();

                if (e.isBoss &&
                    m_bossGrabCooldown <= 0.0f &&
                    m_stats.power > 0 &&
                    !m_player.isClimbing() &&
                    !e.isClimbing())
                {
                    startBossIntimacy(static_cast<int>(i));
                }
                else
                {
                    playSound("attak");

                    damagePlayer(e.isBoss && m_player.isClimbing() ? 2 : 1);

                    if (!m_player.isClimbing())
                        m_player.startHurt();

                    if (!e.isBoss &&
                        !e.isClimbing())
                        e.startAttack(m_player.getPixelPos().x >= e.getPixelPos().x);
                }

                break;
            }
        }

        // --- Бомбы ---
        updateBombs(dt);

        // --- Портал ---
        updateLevelExit();

        // --- Портал: проверка коллизии ---
        if (m_levelExit.isActive() &&
            m_levelExit.checkCollision(pr))
        {
            nextLevel();
            return;
        }
    }

    void Game::render()
    {
        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());
        const float mapW = m_map.width * tw;
        const float mapH = m_map.height * th;
        const float screenW = static_cast<float>(GetScreenWidth());
        const float screenH = static_cast<float>(GetScreenHeight());

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(m_camera);

        // --- Фон с параллаксом (рисуем в мировых координатах) ---
        // Позиция фона = camTopLeft * (1 - k) → на экране движется со скоростью k.
        // Размер подбираем так, чтобы фон покрывал экран во всём диапазоне камеры.
        const SpriteSheetGPU *bgSheet = m_renderer.sheet("bg1");
        if (bgSheet && bgSheet->valid())
        {
            const float k = kParallaxFactor;

            float camX = std::clamp(m_camera.target.x - screenW * 0.5f, 0.0f, std::max(0.0f, mapW - screenW));
            float camY = std::clamp(m_camera.target.y - screenH * 0.5f, 0.0f, std::max(0.0f, mapH - screenH));

            const float bgW = static_cast<float>(bgSheet->texture.width);
            const float bgH = static_cast<float>(bgSheet->texture.height);

            const float needW = screenW + k * std::max(0.0f, mapW - screenW);
            const float needH = screenH + k * std::max(0.0f, mapH - screenH);
            const float s = std::max(needW / bgW, needH / bgH);

            Rectangle src{0, 0, bgW, bgH};
            Rectangle dst{camX * (1.0f - k), camY * (1.0f - k), bgW * s, bgH * s};
            DrawTexturePro(bgSheet->texture, src, dst, {0, 0}, 0.0f, WHITE);
        }

        // --- 2. Карта: все тайлы, задний слой ---
        m_renderer.drawMap(m_map);

        // --- 3. Девушки ---
        for (const auto &e : m_entities)
            e.anim.draw(e.x, e.baseY);

        // --- 4. Презервативы (стеки) ---
        if (const SpriteSheetGPU *cs = m_renderer.sheet("Condom"))
        {
            for (const auto &s : m_condoms)
            {
                Rectangle src = cs->frame(0);
                Rectangle dst{s.x, s.y,
                              static_cast<float>(cs->patternW),
                              static_cast<float>(cs->patternH)};
                DrawTexturePro(cs->texture, src, dst, {0, 0}, 0.0f, WHITE);
            }
        }

        // --- 5. Портал ---
        m_levelExit.draw();

        // --- 6. Игрок (лезет по лестнице) ---
        if (!m_intimacy.active &&
            !m_bossIntimacy.active &&
            m_player.isClimbing())
            m_player.draw();

        // --- 7. Враги ---
        for (const auto &e : m_enemies)
            if (e.isClimbing())
                e.draw();

        // --- 8. Фронт-слой: земля поверх стыков лестница×земля ---
        m_renderer.drawFrontLayer(m_map);

        // --- 9. Игрок (идет мимо лестницы) ---
        if (!m_intimacy.active &&
            !m_bossIntimacy.active &&
            !m_player.isClimbing())
            m_player.draw();

        // --- 10. Враги (идет мимо лестницы) ---
        for (const auto &e : m_enemies)
            if (!e.isClimbing())
                e.draw();

        // --- бомбы ---
        if (const SpriteSheetGPU *bs = m_renderer.sheet("Bomb"))
        {
            for (const auto &b : m_bombs)
            {
                if (b.exploding)
                    continue;

                // один проход за время фуза: последний кадр — прямо перед взрывом
                const int frame = std::min(
                    static_cast<int>((b.t / b.fuse) * bs->frameCount),
                    bs->frameCount - 1);

                Rectangle src = bs->frame(frame);
                Rectangle dst{b.x, b.y,
                              static_cast<float>(bs->patternW),
                              static_cast<float>(bs->patternH)};
                DrawTexturePro(bs->texture, src, dst, {0, 0}, 0.0f, WHITE);
            }
        }

        // --- взрыв ---
        if (const SpriteSheetGPU *es = m_renderer.sheet("Explosion"))
        {
            BeginBlendMode(BLEND_ADD_COLORS);
            for (const auto &b : m_bombs)
            {
                if (!b.exploding)
                    continue;

                const int frame = std::min(static_cast<int>(b.explodeT / kExplosionFrameTime),
                                           es->frameCount - 1);

                Rectangle src = es->frame(frame);
                Rectangle dst{b.tileX * tw + (tw - es->patternW) * 0.5f,
                              b.tileY * th + (th - es->patternH) * 0.5f + kExplosionDy,
                              static_cast<float>(es->patternW),
                              static_cast<float>(es->patternH)};
                DrawTexturePro(es->texture, src, dst, {0, 0}, 0.0f, WHITE);
            }
            EndBlendMode();
        }

        if (m_debugGrid)
        {
            for (int x = 0; x <= m_map.width; ++x)
                DrawLineV({x * tw, 0}, {x * tw, mapH}, ColorAlpha(WHITE, 0.15f));
            for (int y = 0; y <= m_map.height; ++y)
                DrawLineV({0, y * th}, {mapW, y * th}, ColorAlpha(WHITE, 0.15f));
        }

        // Экран победы
        if (m_victory)
        {
            m_victoryTimer += GetFrameTime();

            // Простой текст победы
            const char *msg = "VICTORY!";
            int fontSize = 48;
            int textWidth = MeasureText(msg, fontSize);
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();

            DrawText(msg, (screenW - textWidth) / 2, screenH / 2 - fontSize / 2, fontSize, GOLD);
            DrawText("Press any key to exit", (screenW - MeasureText("Press any key to exit", 20)) / 2,
                     screenH / 2 + 50, 20, WHITE);

            if (m_victoryTimer > 2.0f && IsKeyPressed(KEY_SPACE))
            {
                m_running = false;
            }
        }

        EndMode2D();

        DrawText(TextFormat("HP %d/%d", m_stats.health, m_stats.healthMax), 10, 10, 16, RED);
        DrawText(TextFormat("Power %d/%d", m_stats.power, m_stats.powerMax), 160, 10, 16, YELLOW);
        DrawText(TextFormat("Score %d/%d%s", (int)m_stats.score, m_diff.scoreLevel,
                            m_exitActive ? "  EXIT OPEN" : ""),
                 320, 10, 16, GREEN);

        EndDrawing();
    }

    void Game::run()
    {
        while (m_running && !WindowShouldClose())
        {
            const float dt = GetFrameTime();
            update(dt);
            render();
        }
    }

    void Game::buildFreePoints()
    {
        m_freePoints.clear();

        // тайлы, занятые девушками и игроком, исключаем
        std::vector<std::pair<int, int>> occupied;
        for (const auto &o : m_map.objects)
            occupied.emplace_back(o.x, o.y);

        for (int x = 0; x < m_map.width; ++x)
            for (int y = 0; y < m_map.height; ++y)
            {
                if (m_map.kindAt(x, y) != TileKind::Platform)
                    continue;

                bool busy = false;
                for (const auto &p : occupied)
                    if (p.first == x && p.second == y)
                    {
                        busy = true;
                        break;
                    }
                if (!busy)
                    m_freePoints.emplace_back(x, y);
            }
    }

    void Game::spawnCondoms()
    {
        m_condoms.clear();
        if (m_freePoints.empty())
            return;

        srand(12345 + m_currentLevel * 7919 + m_difficulty * 104729);

        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());
        const SpriteSheetGPU *cs = m_renderer.sheet("Condom");
        const float cw = cs ? static_cast<float>(cs->patternW) : 28.0f;
        const float ch = cs ? static_cast<float>(cs->patternH) : 28.0f;

        // Плотность ремейка: один спавн кладёт стак 1..3,
        // поэтому точек на карте меньше, а суммарное число = CondomCount.
        // Точка принимает не больше kStackCap.
        constexpr int kStackMin = 1;
        constexpr int kStackMax = 3;
        constexpr int kStackCap = 4; // максимум презервативов в одной точке

        int remaining = m_diff.condomCount;
        int guard = 0; // страховка от бесконечного цикла, если точки кончились
        while (remaining > 0 && guard++ < 10000)
        {
            // как в оригинале: индекс = Random(count - 4) + 5
            const size_t n = m_freePoints.size();
            const size_t idx = (n > 4)
                                   ? (static_cast<size_t>(rand()) % (n - 4)) + 5
                                   : static_cast<size_t>(rand()) % n;
            const auto &p = m_freePoints[idx];

            // есть ли уже стак в этой точке
            CondomStack *existing = nullptr;
            for (auto &s : m_condoms)
                if (s.tileX == p.first && s.tileY == p.second)
                {
                    existing = &s;
                    break;
                }

            // точка на пределе — тянем новую
            if (existing && existing->count >= kStackCap)
                continue;

            int stack = kStackMin + rand() % (kStackMax - kStackMin + 1);
            if (stack > remaining)
                stack = remaining;
            if (existing && existing->count + stack > kStackCap)
                stack = kStackCap - existing->count; // доросли до потолка

            remaining -= stack;

            if (existing)
            {
                existing->count += stack;
            }
            else
            {
                CondomStack s;
                s.tileX = p.first;
                s.tileY = p.second;
                s.count = stack;
                s.x = p.first * tw + (tw - cw) * 0.5f;
                s.y = p.second * th + th * 0.5f - ch; // низ на линии земли
                m_condoms.push_back(s);
            }
        }

        if (remaining > 0)
            TraceLog(LOG_WARNING, "Level%d: %d condoms not placed (all points at cap)",
                     m_currentLevel, remaining);

        TraceLog(LOG_INFO, "Level%d: %d condoms in %d stacks",
                 m_currentLevel, m_diff.condomCount - remaining,
                 static_cast<int>(m_condoms.size()));
    }

    void Game::spawnEnemies()
    {
        m_enemies.clear();
        if (m_freePoints.empty())
            return;

        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());

        auto rndPoint = [&]() -> std::pair<int, int>
        {
            const size_t n = m_freePoints.size();
            return m_freePoints[(n > 4) ? (static_cast<size_t>(rand()) % (n - 4)) + 5
                                        : static_cast<size_t>(rand()) % n];
        };

        int m_nextEnemyId = 0;

        // Обычные враги
        const SpriteSheetGPU *enemyGoSheet = m_renderer.sheet("Enemy1Go");
        const SpriteSheetGPU *enemyAttackSheet = m_renderer.sheet("Enemy1Attack");
        if (enemyGoSheet &&
            enemyAttackSheet)
        {
            for (int i = 0; i < m_diff.enemyCountMax; ++i)
            {
                auto p = rndPoint();
                Enemy e;
                e.init(m_map,
                       enemyGoSheet,
                       enemyAttackSheet,
                       p.first, p.second, m_diff.enemySpeed * kSpeedScale, false, tw, th);
                e.id = m_nextEnemyId++;
                m_enemies.push_back(std::move(e));
            }
        }
        else
        {
            TraceLog(LOG_WARNING, "No Enemy animation sheets — enemy not spawned");
        }

        // Босс (EnemyGirl) — одна на уровень
        const SpriteSheetGPU *enemyGirlGoSheet = m_renderer.sheet("Enemy2Go");
        const SpriteSheetGPU *enemyGirlAttackSheet = m_renderer.sheet("Enemy2Attack");
        if (enemyGirlGoSheet &&
            enemyGirlAttackSheet)
        {
            auto p = rndPoint();
            Enemy b;
            b.init(m_map,
                   enemyGirlGoSheet,
                   enemyGirlAttackSheet,
                   p.first, p.second, m_diff.enemyGirlSpeed * kSpeedScale, true, tw, th);
            b.id = m_nextEnemyId++;
            m_enemies.push_back(std::move(b));
            TraceLog(LOG_INFO, "Boss spawned at tile (%d, %d)", p.first, p.second);
        }
        else
        {
            TraceLog(LOG_WARNING, "No EnemyGirl animation sheets — boss not spawned");
        }
    }

    void Game::spawnBombAtPlayer()
    {
        const int tx = m_player.getTileX();
        const int ty = m_player.getTileY();
        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());
        const SpriteSheetGPU *bs = m_renderer.sheet("Bomb");
        const float cw = bs ? static_cast<float>(bs->patternW) : 32.0f;
        const float ch = bs ? static_cast<float>(bs->patternH) : 32.0f;

        Bomb b;
        b.tileX = tx;
        b.tileY = ty;
        b.x = tx * tw + (tw - cw) * 0.5f;
        b.y = ty * th + th * 0.5f - ch;
        b.fuse = kBombFuse;
        m_bombs.push_back(b);
    }

    void Game::spawnLevelExit()
    {
        if (m_freePoints.empty())
            return;

        // Выбираем случайную свободную точку
        std::size_t idx = static_cast<std::size_t>(GetRandomValue(0, m_freePoints.size() - 1));
        auto [tx, ty] = m_freePoints[idx];

        // Инициализация портала в выбранной точке
        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());

        m_levelExit.init(m_map, m_renderer.sheet("LevelExit"), tx, ty, tw, th);
        m_levelExit.setActive(false);
    }

    void Game::updateBombs(float dt)
    {
        const SpriteSheetGPU *es = m_renderer.sheet("Explosion");
        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());

        if (m_player.popMakeBombFinished())
            spawnBombAtPlayer();

        for (auto it = m_bombs.begin(); it != m_bombs.end();)
        {
            auto &b = *it;
            if (!b.exploding)
            {
                b.t += dt;
                if (b.t >= b.fuse)
                {
                    b.exploding = true;
                    b.explodeT = 0.0f;
                    b.explodeDuration = es ? es->frameCount * kExplosionFrameTime : 0.5f;
                    playSound("Explosion");
                }
            }
            else
            {
                Rectangle exRect = getExplosionBounds(b, es, tw, th);

                explosionDamage(b, exRect);

                b.explodeT += dt;
                if (b.explodeT >= b.explodeDuration)
                {
                    it = m_bombs.erase(it); // взрыв доиграл — бомба удаляется
                    continue;
                }
            }
            ++it;
        }
    }

    Rectangle Game::getExplosionBounds(const Bomb &b, const SpriteSheetGPU *es, float tw, float th)
    {
        if (!es || es->visibleBounds.empty())
        {
            // Fallback: номинальный размер
            return Rectangle{b.tileX * tw, b.tileY * th, tw, th};
        }

        // Определяем текущий кадр взрыва
        int currentFrame = static_cast<int>(b.explodeT / kExplosionFrameTime);
        if (currentFrame >= es->frameCount)
            currentFrame = es->frameCount - 1;

        // Получаем видимые границы без чёрного фона
        Rectangle vb = es->visibleBounds[currentFrame];

        // Позиция взрыва (как в render)
        const float drawX = b.tileX * tw + (tw - es->patternW) * 0.5f;
        const float drawY = b.tileY * th + (th - es->patternH) * 0.5f + kExplosionDy;

        return Rectangle{drawX + vb.x, drawY + vb.y, vb.width, vb.height};
    }

    void Game::explosionDamage(Bomb &b, const Rectangle &exRect)
    {
        // игрок: в тайле взрыва в любой момент анимации — фатально, один раз
        if (!b.playerHit &&
            CheckCollisionRecs(exRect, m_player.getBounds()))
        {
            b.playerHit = true;
            killPlayer();
        }

        // враги: разово каждому (id в hitEnemyIds)
        for (auto &e : m_enemies)
        {
            if (!e.alive)
                continue;
            if (std::find(b.hitEnemyIds.begin(), b.hitEnemyIds.end(), e.id) != b.hitEnemyIds.end())
                continue;
            if (!CheckCollisionRecs(exRect, e.getBounds()))
                continue;

            b.hitEnemyIds.push_back(e.id);

            if (e.isBoss)
            {
                if (++e.bombHits >= m_diff.enemyGirlLifeMax)
                {
                    e.alive = false;
                    m_stats.score = std::min(100.0f, m_stats.score + 12.0f);
                }
            }
            else
            {
                e.alive = false;
                m_stats.score = std::min(100.0f, m_stats.score + 2.0f);
            }
        }

        m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(),
                                       [](const Enemy &e)
                                       { return !e.alive; }),
                        m_enemies.end());
    }

    void Game::updateLevelExit()
    {
        // Активируем портал, когда Score >= ScoreLevel
        bool shouldActivate = m_stats.score >= static_cast<float>(m_diff.scoreLevel);

        if (shouldActivate &&
            !m_levelExit.isActive())
        {
            activateLevelExit();
        }
        else if (!shouldActivate &&
                 m_levelExit.isActive())
        {
            m_levelExit.setActive(false);
        }

        m_levelExit.update(GetFrameTime());
    }

    void Game::activateLevelExit()
    {
        m_levelExit.setActive(true);
        playSound("WNCE");
    }

    void Game::nextLevel()
    {
        playSound("LevelComplete");

        if (m_currentLevel >= 12)
        {
            // Победа!
            showVictory();
        }
        else
        {
            // Следующий уровень
            setLevel(m_currentLevel + 1);
        }
    }

    void Game::showVictory()
    {
        m_victory = true;
        m_victoryTimer = 0.0f;
        playSound("Victory");
        // Можно показать экран победы
    }

    void Game::damagePlayer(int dmg)
    {
        if (m_hurtTimer > 0.0f)
            return;
        m_hurtTimer = 1.0f;
        m_stats.health -= dmg;
        if (m_stats.health <= 0)
        {
            m_stats.health = 0;
            killPlayer();
        }
    }

    void Game::killPlayer()
    {
        m_stats.health = 0;
        playSound("Scream");
        m_deathTimer = 1.0f;
    }

    void Game::loadSounds()
    {
        unloadSounds();

        namespace fs = std::filesystem;
        auto loadDir = [&](const fs::path &dir)
        {
            std::error_code ec;
            if (!fs::is_directory(dir, ec))
                return;
            for (auto &e : fs::directory_iterator(dir, ec))
            {
                if (ec || !e.is_regular_file())
                    continue;
                if (toLower(e.path().extension().string()) != ".wav")
                    continue;
                const std::string key = toLower(e.path().stem().string());
                if (m_sounds.count(key))
                    continue;
                Sound snd = LoadSound(e.path().string().c_str());
                if (snd.frameCount > 0)
                    m_sounds.emplace(key, snd);
                else
                    TraceLog(LOG_WARNING, "Failed to load sound: %s", e.path().string().c_str());
            }
        };

        loadDir(resolveDirCI(m_dataRoot, {"Common", "LevelSounds"}));
        loadDir(resolveDirCI(m_dataRoot, {("LEVEL" + std::to_string(m_currentLevel)).c_str(), "Sound"}));
        TraceLog(LOG_INFO, "Sounds loaded: %d", (int)m_sounds.size());
    }

    void Game::unloadSounds()
    {
        for (auto &[name, s] : m_sounds)
            UnloadSound(s);
        m_sounds.clear();
    }

    void Game::playSound(const char *name)
    {
        auto it = m_sounds.find(toLower(name));
        if (it != m_sounds.end())
            PlaySound(it->second);
        else
            TraceLog(LOG_WARNING, "Sound not found: %s", name);
    }

    void Game::stopSound(const char *name)
    {
        auto it = m_sounds.find(toLower(name));
        if (it != m_sounds.end())
            StopSound(it->second);
    }

    float Game::soundDuration(const char *name) const
    {
        auto it = m_sounds.find(toLower(name));
        if (it == m_sounds.end() || it->second.stream.sampleRate == 0)
            return 0.0f;
        return static_cast<float>(it->second.frameCount) /
               static_cast<float>(it->second.stream.sampleRate);
    }

} // namespace vovochka