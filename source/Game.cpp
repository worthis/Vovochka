#include "Game.h"
#include "SpriteLayout.h"
#include <algorithm>
#include "raylib.h"

namespace vovochka
{

    Game::Game(std::string dataRoot) : m_dataRoot(std::move(dataRoot)) {}
    Game::~Game() { shutdown(); }

    bool Game::init(int screenW, int screenH, const char *title)
    {
        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
        InitWindow(screenW, screenH, title);
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
            CloseWindow();
            m_running = false;
        }
    }

    void Game::setLevel(int n)
    {
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

        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());

        m_player.init(m_map,
                      m_renderer.sheet("PlayerStand0"),
                      m_renderer.sheet("PlayerGo"),
                      tw, th);

        m_entities.clear();
        for (const auto &o : m_map.objects)
        {
            if (o.type != MapObjectType::Girl)
                continue;
            PreviewEntity e;
            e.type = o.type;
            e.anim.sheet = m_renderer.sheet("Girl1Wait");
            e.anim.setBlock(SpriteLayout::girlWait(m_girlsPresent,
                                                   e.anim.sheet ? e.anim.sheet->frameCount : 2));
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
        if (IsKeyPressed(KEY_H))
        {
            m_girlsPresent = !m_girlsPresent;
            for (auto &e : m_entities)
                if (e.type == MapObjectType::Girl)
                    e.anim.setBlock(SpriteLayout::girlWait(m_girlsPresent,
                                                           e.anim.sheet ? e.anim.sheet->frameCount : 2));
        }
    }

    void Game::update(float dt)
    {
        m_player.update(dt, m_map);
        updateCamera();
        for (auto &e : m_entities)
            e.anim.update(dt);
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

        // --- 4. Презервативы (появятся со спавнером) ---
        // --- 5. Портал / LevelExit (появится со спавнером) ---

        // --- 6. Игрок (лезет по лестнице) ---
        const bool playerBehind = m_player.isBehindFrontLayer();
        if (playerBehind)
            m_player.draw();

        // --- 7. Враги (появятся со спавнером) ---

        // --- 8. Фронт-слой: земля поверх стыков лестница×земля ---
        m_renderer.drawFrontLayer(m_map);

        // --- 9. Игрок (идет мимо лестницы) ---
        if (!playerBehind)
            m_player.draw();

        if (m_debugGrid)
        {
            for (int x = 0; x <= m_map.width; ++x)
                DrawLineV({x * tw, 0}, {x * tw, mapH}, ColorAlpha(WHITE, 0.15f));
            for (int y = 0; y <= m_map.height; ++y)
                DrawLineV({0, y * th}, {mapW, y * th}, ColorAlpha(WHITE, 0.15f));
        }

        EndMode2D();

        DrawText(TextFormat("Level %d/12  Q/E: level  G: grid  H: girl  ESC: quit",
                            m_currentLevel),
                 10, 10, 18, RAYWHITE);
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

} // namespace vovochka