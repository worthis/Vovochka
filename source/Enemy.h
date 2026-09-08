#pragma once

#include "Animation.h"
#include "MapData.h"
#include "SpriteLayout.h"
#include "Character.h"
#include "raylib.h"

namespace vovochka
{

    class Enemy : public Character
    {
    public:
        void init(const LevelMap &map,
                  const SpriteSheetGPU *walkSheet,
                  const SpriteSheetGPU *attackSheet,
                  int tileX, int tileY, float speedPx, bool boss,
                  float tileW, float tileH);

        Rectangle getBounds() const override;

        void update(float dt, const LevelMap &map);
        void draw() const;

        void startAttack(bool faceRight, bool loop = false);
        bool isAttacking() const { return m_attacking; }
        void stopAttack() { m_attacking = false; };

        void bossThink(float dt, const LevelMap &map,
                       int playerTileX, int playerTileY,
                       int playerPower, int powerThreshold);

        int id = 0;
        int bombHits = 0;
        bool alive = true;
        bool isBoss = false;

    protected:
        int patW() const override;
        int patH() const override;

    private:
        // A* pathfinding к цели
        struct PathNode
        {
            int x, y;
            float g, h, f;
            int parentX, parentY;
        };

        Animation m_animAttack;
        bool m_attacking = false;
        float m_attackT = 0.0f;
        float m_attackDuration = 0.0f;

        Animation m_animWalk;
        SpriteLayout::WalkAnim m_currentWalkAnim = SpriteLayout::WalkAnim::Right;

        std::vector<std::pair<int, int>> m_path;
        std::size_t m_pathIdx = 0;
        float m_thinkTimer = 0.0f;

        static constexpr float kThinkTimer = 6.0f;

        bool bfs(const LevelMap &map, int sx, int sy, int tx, int ty,
                 std::vector<std::pair<int, int>> &out) const;
        void buildPath(const LevelMap &map, int tx, int ty);
        void buildRandomPath(const LevelMap &map);
        void chooseDirection(float dt, const LevelMap &map);
        Rectangle offsetBounds(const Animation &anim) const;
    };

} // namespace vovochka