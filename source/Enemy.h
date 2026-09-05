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
        Rectangle getBounds() const;
        bool isBehindFrontLayer() const { return m_snapping && m_dirY != 0; }
        bool isOnLadder(const LevelMap &map) const;
        void startAttack(bool faceRight, bool loop = false);
        void stopAttack();
        Vector2 getPixelPos() const { return m_pos; }
        void setTarget(const LevelMap &map, int tx, int ty);

        bool alive = true;
        bool isBoss = false;
        int id = 0;
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
        int m_dirX = 0, m_dirY = 0;

        bool m_snapping = false;
        int m_snapTargetX = 0, m_snapTargetY = 0;

        float m_tileW = 80, m_tileH = 80;
        float m_speed = 37.5f;

        Animation m_animAttack;
        bool m_attacking = false;
        float m_attackT = 0.0f;
        float m_attackDuration = 0.0f;

        Animation m_animWalk;
        SpriteLayout::WalkAnim m_currentWalkAnim = SpriteLayout::WalkAnim::Right;

        int patW() const;
        int patH() const;
        float surfaceYForTile(int tileY) const;

        bool canClimbUp(int x, int y, const LevelMap &map) const;
        bool canClimbDown(int x, int y, const LevelMap &map) const;
        bool canOccupy(int x, int y, const LevelMap &map) const;

        void getAllowed(const LevelMap &map, bool &L, bool &R, bool &U, bool &D) const;
        void chooseDirection(const LevelMap &map);
        void stepSnap(float dt, const LevelMap &map);
        void clampToMap(const LevelMap &map);

        void updateAI(const LevelMap &map);
    };

} // namespace vovochka