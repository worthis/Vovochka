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
        m_camera.target = {VIEW_W * 0.5f, VIEW_H * 0.5f};
        m_camera.offset = {screenW * 0.5f, screenH * 0.5f};

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
        const float tw = static_cast<float>(m_renderer.tileW());
        const float th = static_cast<float>(m_renderer.tileH());

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

        m_entities.clear();
        for (const auto &o : m_map.objects)
        {
            PreviewEntity e;
            e.type = o.type;

            switch (o.type)
            {
            case MapObjectType::Player:
            {
                e.anim.sheet = m_renderer.sheet("PlayerStand0");
                e.anim.setBlock(SpriteLayout::playerStand(m_playerFaceRight));
                e.anim.frameTime = 0.10f;
                if (e.anim.sheet)
                {
                    e.x = o.x * tw + (tw - e.anim.sheet->patternW) * 0.5f;
                    e.baseY = (o.y + 1) * th - e.anim.sheet->patternH - th * 0.4f;
                }
                break;
            }
            case MapObjectType::Girl:
            {
                e.anim.sheet = m_renderer.sheet("Girl1Wait");
                e.anim.setBlock(SpriteLayout::girlWait(m_girlsPresent));
                if (e.anim.sheet)
                {
                    e.x = o.x * tw + (tw - e.anim.sheet->patternW) * 0.5f;
                    e.baseY = (o.y + 1) * th - e.anim.sheet->patternH - th * 0.4f;
                }
                break;
            }
            default:
                continue;
            }

            if (!e.anim.sheet)
            {
                TraceLog(LOG_WARNING, "No sheet for object type %d", static_cast<int>(o.type));
                continue;
            }

            m_entities.push_back(std::move(e));
        }

        // Сбросим камеру в стартовую позицию (player type=1)
        if (auto start = m_map.playerStart())
        {
            m_camera.target = {start->x * tw + tw * 0.5f,
                               start->y * th + th * 0.5f};
        }
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
        // F — развернуть игрока (влево/вправо), H — девушка есть/нет
        if (IsKeyPressed(KEY_F))
        {
            m_playerFaceRight = !m_playerFaceRight;
            for (auto &e : m_entities)
                if (e.type == MapObjectType::Player)
                    e.anim.setBlock(SpriteLayout::playerStand(m_playerFaceRight));
        }
        if (IsKeyPressed(KEY_H))
        {
            m_girlsPresent = !m_girlsPresent;
            for (auto &e : m_entities)
                if (e.type == MapObjectType::Girl)
                    e.anim.setBlock(SpriteLayout::girlWait(m_girlsPresent));
        }

        // Скролл камеры мышью/стрелками
        const float pan = 8.0f;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
            m_camera.target.y -= pan;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
            m_camera.target.y += pan;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
            m_camera.target.x -= pan;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
            m_camera.target.x += pan;
        m_camera.zoom += GetMouseWheelMove() * 0.1f;
        m_camera.zoom = std::clamp(m_camera.zoom, 0.3f, 3.0f);
    }

    void Game::update(float dt)
    {
        for (auto &e : m_entities)
            e.anim.update(dt);
    }

    void Game::render()
    {
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(m_camera);
        m_renderer.draw(m_map, true);

        for (const auto &e : m_entities)
        {
            e.anim.draw(e.x, e.baseY);
        }

        if (m_debugGrid)
        {
            for (int x = 0; x <= m_map.width; ++x)
                DrawLineV({x * 80.0f, 0}, {x * 80.0f, m_map.height * 80.0f}, ColorAlpha(WHITE, 0.15f));
            for (int y = 0; y <= m_map.height; ++y)
                DrawLineV({0, y * 80.0f}, {m_map.width * 80.0f, y * 80.0f}, ColorAlpha(WHITE, 0.15f));
        }
        EndMode2D();

        DrawText(TextFormat("Level %d/12   A/D: level   WASD: pan   Wheel: zoom   G: grid   ESC: quit",
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