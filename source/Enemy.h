#pragma once
#include "Animation.h"
#include "MapData.h"
#include "SpriteLayout.h"
#include "raylib.h"

namespace vovochka
{

    class Enemy
    {
    public:
        void init(const LevelMap &map,
                  const SpriteSheetGPU *walkSheet,
                  const SpriteSheetGPU *attackSheet,
                  int tileX, int tileY, float speedPx, bool boss,
                  float tileW, float tileH);

        void update(float dt, const LevelMap &map);
        void draw() const;

        int getTileX() const { return m_tileX; }
        int getTileY() const { return m_tileY; }
        Vector2 getPixelPos() const { return m_pos; }
        void setPixelPos(float x, float y)
        {
            m_pos.x = x;
            m_pos.y = y;
        }
        Rectangle getBounds() const;

        bool isBehindFrontLayer() const { return m_snapping && m_dirY != 0; }
        bool isOnLadder(const LevelMap &map) const;

        void startAttack(bool faceRight, bool loop = false);
        bool isAttacking() const { return m_attacking; }
        void stopAttack();

        void bossThink(float dt, const LevelMap &map,
                       int playerTileX, int playerTileY,
                       int playerPower, int powerThreshold);

        int id = 0;
        bool alive = true;
        bool isBoss = false;
        int bombHits = 0;

    private:
        // A* pathfinding к цели
        struct PathNode
        {
            int x, y;
            float g, h, f;
            int parentX, parentY;
        };

        Vector2 m_pos{};
        int m_tileX = 0, m_tileY = 0;
        float m_tileW = 80, m_tileH = 80;
        int m_dirX = 0, m_dirY = 0;
        float m_speed = 200.0f;
        bool m_snapping = false;
        int m_snapTargetX = 0, m_snapTargetY = 0;

        Animation m_animAttack;
        bool m_attacking = false;
        float m_attackT = 0.0f;
        float m_attackDuration = 0.0f;

        Animation m_animWalk;
        SpriteLayout::WalkAnim m_currentWalkAnim = SpriteLayout::WalkAnim::Right;

        std::vector<std::pair<int, int>> m_path;
        std::size_t m_pathIdx = 0;
        float m_thinkTimer = 0.0f;

        bool bfs(const LevelMap &map, int sx, int sy, int tx, int ty,
                 std::vector<std::pair<int, int>> &out) const;
        void buildPath(const LevelMap &map, int tx, int ty);
        void buildRandomPath(const LevelMap &map);
        void stepSnap(float dt, const LevelMap &map);

        int patW() const;
        int patH() const;
        float surfaceYForTile(int tileY) const;

        bool canClimbUp(int x, int y, const LevelMap &map) const;
        bool canClimbDown(int x, int y, const LevelMap &map) const;
        bool canOccupy(int x, int y, const LevelMap &map) const;

        void getAllowed(const LevelMap &map, bool &L, bool &R, bool &U, bool &D) const;
        void getAllowedAt(const LevelMap &map, int x, int y,
                          bool &L, bool &R, bool &U, bool &D) const;
        void chooseDirection(const LevelMap &map);
        void clampToMap(const LevelMap &map);
    };

} // namespace vovochka