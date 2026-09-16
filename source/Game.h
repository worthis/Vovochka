#pragma once

#include "InputSystem.h"
#include "MapData.h"
#include "LevelLoader.h"
#include "LevelRenderer.h"
#include "Animation.h"
#include "Player.h"
#include "Enemy.h"
#include "LevelExit.h"
#include "ConfigSystem.h"
#include "FontSystem.h"
#include "raylib.h"
#include <unordered_map>

namespace vovochka
{

    class Game
    {
    public:
        explicit Game(std::string dataRoot);
        ~Game();

        bool init();
        bool isRunning() const { return m_running; }
        void update(float dt, InputSystem &input);
        void render();
        void shutdown();
        void setLevel(int level, int difficulty, bool isNewGame); // 1..12, 1..3
        bool isLevelCompleted() const { return m_levelCompletedPending; }
        int completedLevel() const { return m_completedLevel; }
        void proceedToNextLevel();

    private:
        static constexpr float kSpeedScale = 3000.0f;
        static constexpr float kBombFuse = 1.2f;            // время фитиля бомбы
        static constexpr float kExplosionFrameTime = 0.05f; // время анимации взрыва
        static constexpr float kExplosionDy = -70.0f;       // положение анимации взрыва по оси Y
        static constexpr float kParallaxFactor = 0.3f;      // Параллакс фона: 0 = неподвижен, 1 = вместе с камерой
        static constexpr float kBossGrabCooldown = 1.5f;    // кулдаун атаки босса 1500 мс
        static constexpr int kMinimapLineTh = 4;            // толщина линий карты, px
        static constexpr int kMinimapMarkerSize = 4;        // размер квадратиков-меток, px

        struct GirlEntity
        {
            Animation anim;
            MapObjectType type = MapObjectType::Unknown;
            float x = 0.0f, y = 0.0f;
            int tileX = 0, tileY = 0; // точка спавна
            bool used = false;        // девушка потрачена
        };

        struct CondomStack
        {
            int tileX = 0, tileY = 0;
            int count = 0;
            float x = 0, y = 0; // верхний левый угол спрайта
        };

        struct Stats
        {
            int lives = 3;
            int health = 8, healthMax = 8;
            int power = 6, powerMax = 6;
            float score = 0.0f, scoreMax = 100.0f;
        };

        struct IntimacySession
        {
            bool active = false;
            int girlIdx = -1;
            float t = 0.0f;
            float duration = 3.326f;
            float powerStart = 0;
        };

        struct Bomb
        {
            int tileX = 0, tileY = 0;
            float x = 0, y = 0;
            float t = 0;
            float fuse = 2.0f; // время жизни до взрыва
            bool exploding = false;
            float explodeT = 0;
            float explodeDuration = 0;
            bool playerHit = false;       // игрок задел один раз
            std::vector<int> hitEnemyIds; // каждый враг — один раз
        };

        std::string m_dataRoot;
        LevelLoader m_loader{m_dataRoot};
        LevelRenderer m_renderer;
        Camera2D m_camera{};
        LevelMap m_map;
        Player m_player;
        Stats m_stats{};
        DifficultyParams m_diff{};
        IntimacySession m_intimacy{};
        IntimacySession m_bossIntimacy{};
        std::vector<GirlEntity> m_entities;
        std::vector<std::pair<int, int>> m_freePoints; // тайлы земли для спавна
        std::vector<CondomStack> m_condoms;
        std::vector<Bomb> m_bombs;
        std::unordered_map<std::string, Sound> m_sounds;
        std::vector<Enemy> m_enemies;
        LevelExit m_levelExit;
        BitmapFont m_fontHud;

        Sound m_stairsSound{};
        Sound m_moveSound{};
        Music m_music{};
        bool m_musicLoaded = false;
        bool m_musicPlaying = false;
        float m_musicVolume = 0.5f;

        float m_laughTimer = 0.0f;
        bool m_exitActive = false;
        int m_difficulty = 1; // 1..3
        int m_currentLevel = 1;
        bool m_debugGrid = false;
        bool m_running = false;
        float m_deathTimer = 0.0f;
        float m_hurtTimer = 0.0f;        // i-frames после урона
        float m_bossGrabCooldown = 0.0f; // кулдаун до повторного захвата
        bool m_gameOver = false;
        bool m_levelCompletedPending = false;
        int m_completedLevel = 0;

        void buildFreePoints();
        float calculateScoreMax(int girlsNum, int enemyNum, int enemyGirlNum) const;

        void spawnCondoms();
        void spawnBombAtPlayer();
        void spawnEnemies();
        void spawnLevelExit();

        void updateGameplay(float dt); // игровая логика: предметы, враги, бомбы, близость
        void updateFootsteps();        // звуки шагов игрока
        void updateBombs(float dt);
        void updateCamera();
        void updateLevelExit();

        void renderHUD();
        void drawBar(Rectangle r, float fraction);
        void drawMinimap(Rectangle r);

        bool checkBombConditions(InputSystem &input); // проверка условий создания бомбы
        Rectangle getExplosionBounds(const Bomb &b, const SpriteSheetGPU *es, float tw, float th);
        void explosionDamage(Bomb &b, const Rectangle &exRect);
        void damagePlayer(int dmg);
        void killPlayer();

        void startIntimacy(int girlIdx);
        void endIntimacy();
        void startBossIntimacy(int bossIdx);
        void endBossIntimacy();

        void nextLevel();

        void loadSounds();
        void unloadSounds();
        Sound getSound(const char *name);
        void playSound(const char *name);
        void stopSound(const char *name);
        float soundDuration(const char *name) const;
    };

} // namespace vovochka