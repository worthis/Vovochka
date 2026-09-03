#pragma once
#include "MapData.h"
#include "LevelLoader.h"
#include "LevelRenderer.h"
#include "Animation.h"
#include "raylib.h"

namespace vovochka
{

    class Game
    {
    public:
        explicit Game(std::string dataRoot);
        ~Game();

        bool init(int screenW, int screenH, const char *title);
        void run(); // главный цикл
        void shutdown();

        void setLevel(int n); // 1..12

    private:
        struct PreviewEntity
        {
            Animation anim;
            MapObjectType type = MapObjectType::Unknown;
            float x = 0.0f;
            float baseY = 0.0f;
        };

        std::vector<PreviewEntity> m_entities;

        void processInput();
        void update(float dt);
        void render();

        std::string m_dataRoot;
        LevelLoader m_loader{m_dataRoot};
        LevelRenderer m_renderer;
        LevelMap m_map;

        Camera2D m_camera{};
        int m_currentLevel = 1;
        bool m_debugGrid = false;
        bool m_running = false;

        bool m_playerFaceRight = true;
        bool m_girlsPresent = true;

        static constexpr int VIEW_W = 640;
        static constexpr int VIEW_H = 400;
    };

} // namespace vovochka