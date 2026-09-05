#include "Game.h"
#include "SpriteLayout.h"
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
                      tw, th);

        m_loader.loadDifficulty(m_difficulty, m_diff);

        buildFreePoints();
        spawnCondoms();
        spawnEnemies();

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
            e.consumed = false;
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

    void Game::processInput()
    {
        if (IsKeyPressed(KEY_ESCAPE))
            m_running = false;
        if (IsKeyPressed(KEY_G))
            m_debugGrid = !m_debugGrid;
        if (IsKeyPressed(KEY_E))
            setLevel(m_currentLevel + 1);
        if (IsKeyPressed(KEY_Q))
            setLevel(m_currentLevel - 1);

        for (int d = 1; d <= 3; ++d)
            if (IsKeyPressed(static_cast<KeyboardKey>(KEY_ONE + d - 1)))
            {
                m_difficulty = d;
                setLevel(m_currentLevel);
            }
    }

    void Game::update(float dt)
    {
        // гибель игрока: пауза и рестарт уровня
        if (m_deathTimer > 0.0f)
        {
            m_deathTimer -= dt;
            if (m_deathTimer <= 0.0f)
            {
                setLevel(m_currentLevel);
                return;
            }
        }

        m_player.setInputEnabled(!m_intimacy.active && m_deathTimer <= 0.0f);
        m_player.update(dt, m_map);

        if (m_player.takeMakeBombFinished())
            spawnBombAtPlayer();

        updateCamera();
        updateGameplay(dt);
        updateBombs(dt);

        for (auto &e : m_entities)
            e.anim.update(dt);

        m_hurtTimer = std::max(0.0f, m_hurtTimer - dt);
        for (auto &e : m_enemies)
            e.update(dt, m_map);

        // контакт враг-игрок: атака, урон, прерывания
        if (m_deathTimer <= 0.0f &&
            m_hurtTimer <= 0.0f &&
            !m_player.isHurt())
        {
            Rectangle pr = m_player.getBounds();

            for (auto &e : m_enemies)
            {
                if (!e.alive || e.isBoss)
                    continue;

                if (!CheckCollisionRecs(pr, e.getBounds()))
                    continue;

                // 3: контакт прерывает создание бомбы (бомба не ставится, Power не возвращается)
                m_player.cancelMakeBomb();

                // прерывание близости: девушка потрачена в любом случае
                if (m_intimacy.active)
                    endIntimacy();

                // урон: -10 HP (одно деление шкалы), i-frames 1 c
                damagePlayer(10);

                // 2: анимация получения урона игроком (PlayerUndoAttack)
                if (!m_player.isOnLadderNow())
                    m_player.startHurt();

                // 1: анимация атаки врага (Enemy1Attack/Enemy2Attack) в сторону игрока
                if (!e.isOnLadder(m_map))
                    e.startAttack(m_player.getPixelPos().x >= e.getPixelPos().x);

                break; // один контакт за кадр
            }
        }
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

        m_intimacy.duration *= m_stats.power;
        m_intimacy.duration /= m_stats.powerMax;

        playSound("GirlScream");
        playSound("KISS");
    }

    void Game::endIntimacy()
    {
        auto &g = m_entities[m_intimacy.girlIdx];
        g.consumed = true;
        if (const SpriteSheetGPU *wait = m_renderer.sheet("Girl1Wait"))
        {
            g.anim.sheet = wait;
            g.anim.setBlock(SpriteLayout::girlWait(false, wait->frameCount));
        }

        stopSound("GirlScream");
        stopSound("KISS");

        m_intimacy.active = false;
    }

    void Game::updateGameplay(float dt)
    {
        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());
        Rectangle pr = m_player.getBounds();

        // --- бомба: Пробел, стоит BombCost Power, в ТАЙЛЕ игрока ---
        if (!m_intimacy.active &&
            m_deathTimer <= 0.0f &&
            IsKeyPressed(KEY_SPACE) &&
            !m_player.isMakingBomb() &&
            !m_player.isHurt() &&
            m_player.canPlaceBomb(m_map) &&
            m_stats.power >= m_diff.bombCost)
        {
            m_stats.power -= m_diff.bombCost;
            m_player.startMakeBomb(m_map);
            playSound("Breath");
        }

        // --- близость: слив Power, рост Score 1:2 ---
        if (m_intimacy.active)
        {
            m_intimacy.t += dt;
            const float k = std::min(1.0f, m_intimacy.t / m_intimacy.duration);
            m_stats.power = (int)std::ceil(m_intimacy.powerStart * (1.0f - k));
            m_stats.score = std::min(100.0f, m_stats.score + 2.0f * m_intimacy.powerStart * dt / m_intimacy.duration);
            if (k >= 1.0f)
                endIntimacy();
        }

        // --- подбор презервативов: +1 Power, декремент стека ---
        if (!m_intimacy.active)
        {
            const SpriteSheetGPU *cs = m_renderer.sheet("Condom");
            const float cw = cs ? (float)cs->patternW : 28.0f;
            const float ch = cs ? (float)cs->patternH : 28.0f;
            for (auto it = m_condoms.begin(); it != m_condoms.end();)
            {
                Rectangle cr{it->x, it->y, cw, ch};
                if (m_stats.power < m_stats.powerMax && CheckCollisionRecs(pr, cr))
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

        // --- девушки: попытка близости ---
        constexpr float kGirlTriggerMargin = 8.0f; // срабатывать чуть раньше границы (тюнинг)
        m_laughTimer = std::max(0.0f, m_laughTimer - dt);
        if (!m_intimacy.active)
        {
            for (size_t i = 0; i < m_entities.size(); ++i)
            {
                const auto &g = m_entities[i];
                if (g.type != MapObjectType::Girl || g.consumed)
                    continue;

                // прямоугольник ТАЙЛА спавна девушки, а не её спрайта
                Rectangle gr{g.tileX * tw - kGirlTriggerMargin,
                             g.tileY * th - kGirlTriggerMargin,
                             tw + 2.0f * kGirlTriggerMargin,
                             th + 2.0f * kGirlTriggerMargin};

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

        // --- портал: активен при Score >= ScoreLevel ---
        m_exitActive = m_stats.score >= (float)m_diff.scoreLevel;
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

        // --- 5. Портал / LevelExit (появится со спавнером) ---

        // --- 6. Игрок (лезет по лестнице) ---
        const bool playerBehind = m_player.isBehindFrontLayer();
        if (!m_intimacy.active && playerBehind)
            m_player.draw();

        // --- 7. Враги ---
        for (const auto &e : m_enemies)
            if (e.isBehindFrontLayer())
                e.draw();

        // --- 8. Фронт-слой: земля поверх стыков лестница×земля ---
        m_renderer.drawFrontLayer(m_map);

        // --- 9. Игрок (идет мимо лестницы) ---
        if (!m_intimacy.active && !playerBehind)
            m_player.draw();

        // --- 10. Враги (идет мимо лестницы) ---
        for (const auto &e : m_enemies)
            if (!e.isBehindFrontLayer())
                e.draw();

        if (m_debugGrid)
        {
            for (int x = 0; x <= m_map.width; ++x)
                DrawLineV({x * tw, 0}, {x * tw, mapH}, ColorAlpha(WHITE, 0.15f));
            for (int y = 0; y <= m_map.height; ++y)
                DrawLineV({0, y * th}, {mapW, y * th}, ColorAlpha(WHITE, 0.15f));
        }

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

        // --- взрыв: аддитив — чёрный фон не рисуется, огонь «светится» ---
        if (const SpriteSheetGPU *es = m_renderer.sheet("Explosion"))
        {
            BeginBlendMode(BLEND_ADDITIVE);
            for (const auto &b : m_bombs)
            {
                if (!b.exploding)
                    continue;

                const int frame = std::min(
                    static_cast<int>(b.explodeT / kExplosionFrameTime),
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
            processInput();
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
        constexpr float kSpeedScale = 4800.0f;

        auto rndPoint = [&]() -> std::pair<int, int>
        {
            const size_t n = m_freePoints.size();
            return m_freePoints[(n > 4) ? (static_cast<size_t>(rand()) % (n - 4)) + 5
                                        : static_cast<size_t>(rand()) % n];
        };

        int m_nextEnemyId = 0;
        for (int i = 0; i < m_diff.enemyCountMax; ++i)
        {
            auto p = rndPoint();
            Enemy e;
            e.init(m_map,
                   m_renderer.sheet("Enemy1Go"),
                   m_renderer.sheet("Enemy1Attack"),
                   p.first, p.second, m_diff.enemySpeed * kSpeedScale, false, tw, th);
            e.id = m_nextEnemyId++;
            m_enemies.push_back(std::move(e));
        }

        /*if (const SpriteSheetGPU *bs = m_renderer.sheet("EnemyGirlGo"))
        {
            auto p = rndPoint();
            Enemy b;
            b.init(m_map,
                   bs,
                   p.first, p.second, m_diff.enemyGirlSpeed * kSpeedScale, true, tw, th);
            m_enemies.push_back(std::move(b));
        }
        else
        {
            TraceLog(LOG_WARNING, "No EnemyGirlGo sheet — boss not spawned");
        }*/
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

    void Game::updateBombs(float dt)
    {
        const SpriteSheetGPU *es = m_renderer.sheet("Explosion");
        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());

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
                Rectangle exRect{b.tileX * tw, b.tileY * th, tw, th};
                if (es)
                {
                    exRect = Rectangle{b.tileX * tw + (tw - es->patternW) * 0.5f,
                                       b.tileY * th + (th - es->patternH) * 0.5f + kExplosionDy,
                                       static_cast<float>(es->patternW),
                                       static_cast<float>(es->patternH)};
                }

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
                // босс держит 4 взрыва
                if (++e.bombHits >= 4)
                {
                    e.alive = false;
                    m_stats.score = std::min(100.0f, m_stats.score + 12.0f);
                }
            }
            else
            {
                e.alive = false; // обычный умирает с одного
                m_stats.score = std::min(100.0f, m_stats.score + 2.0f);
            }
        }

        m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(),
                                       [](const Enemy &e)
                                       { return !e.alive; }),
                        m_enemies.end());
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