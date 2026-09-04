#pragma once
#include "MapData.h"
#include "LevelLoader.h"
#include "LevelRenderer.h"
#include "Animation.h"
#include "Player.h"
#include "raylib.h"
#include <unordered_map>

namespace vovochka
{

    class Game
    {
    public:
        explicit Game(std::string dataRoot);
        ~Game();

        bool init(int screenW, int screenH, const char *title);
        void run();
        void shutdown();

        void setLevel(int n); // 1..12

    private:
        struct PreviewEntity
        {
            Animation anim;
            MapObjectType type = MapObjectType::Unknown;
            float x = 0.0f, baseY = 0.0f;
            int tileX = 0, tileY = 0; // точка спавна (для близости)
            bool consumed = false;    // девушка потрачена
        };

        struct CondomStack
        {
            int tileX = 0, tileY = 0;
            int count = 0;
            float x = 0, y = 0; // верхний левый угол спрайта
        };

        struct Stats
        {
            int health = 80, healthMax = 80; // Energy: 1 деление = 10
            int power = 6, powerMax = 6;     // Progress1: 0 пусто, 1..6
            float score = 0;                 // 0..100
        };

        struct IntimacySession
        {
            bool active = false;
            float t = 0.0f;
            float duration = 3.326f;
            int girlIdx = -1;
            float powerStart = 0;
        };

        Camera2D m_camera{};
        std::string m_dataRoot;
        LevelLoader m_loader{m_dataRoot};
        LevelRenderer m_renderer;
        LevelMap m_map;
        Player m_player;
        Stats m_stats{};
        DifficultyParams m_diff{};
        IntimacySession m_intimacy{};
        std::vector<PreviewEntity> m_entities;
        std::vector<std::pair<int, int>> m_freePoints; // тайлы земли для спавна
        std::vector<CondomStack> m_condoms;
        std::unordered_map<std::string, Sound> m_sounds;

        float m_laughTimer = 0.0f;
        bool m_exitActive = false;
        int m_difficulty = 1; // 1..3, клавиши 1/2/3
        int m_currentLevel = 1;
        bool m_debugGrid = false;
        bool m_running = false;

        static constexpr float kParallaxFactor = 0.3f; // Параллакс фона: 0 = неподвижен, 1 = вместе с камерой

        void buildFreePoints();
        void spawnCondoms();

        void updateGameplay(float dt);
        void startIntimacy(int girlIdx);
        void endIntimacy();

        void processInput();
        void update(float dt);
        void render();
        void updateCamera();

        void loadSounds();
        void unloadSounds();
        void playSound(const char *name);
        float soundDuration(const char *name) const;
    };

} // namespace vovochka