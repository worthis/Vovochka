#pragma once
#include "Animation.h"
#include "MapData.h"
#include "raylib.h"

namespace vovochka
{

    class Player
    {
    public:
        void init(const LevelMap &map,
                  const SpriteSheetGPU *standSheet,
                  const SpriteSheetGPU *walkSheet,
                  float tileW, float tileH);
        void setInputEnabled(bool b) { m_inputEnabled = b; }
        void update(float dt, const LevelMap &map);
        void draw() const;

        bool isClimbing() const;
        bool isFrozenClimb() const { return m_frozenClimb; }
        bool isBehindFrontLayer() const { return isClimbing() || m_frozenClimb; }
        void placeAt(const LevelMap &map, int tileX, int tileY);

        Vector2 getPixelPos() const { return m_pos; }
        int getTileX() const { return m_tileX; }
        int getTileY() const { return m_tileY; }
        Rectangle getBounds() const; // Габариты спрайта в мировых координатах

    private:
        Vector2 m_pos{};
        int m_tileX = 0, m_tileY = 0;
        int m_dirX = 0, m_dirY = 0;
        int m_lastDirX = 0, m_lastDirY = 0;

        bool m_snapping = false;
        bool m_xLocked = false;
        int m_snapTargetX = 0, m_snapTargetY = 0;
        bool m_frozenClimb = false;

        float m_tileW = 80, m_tileH = 80;
        float m_speed = 200.0f;

        Animation m_animStand;
        Animation m_animWalk;
        bool m_facingRight = true;
        bool m_onLadder = false;
        bool m_prevOnLadder = false;

        bool m_inputEnabled = true;

        SpriteLayout::WalkAnim m_currentWalkAnim = SpriteLayout::WalkAnim::Right;

        int patW() const;
        int patH() const;
        float surfaceYForTile(int tileY) const;
        float feetY() const { return m_pos.y + patH(); }

        bool canClimbUp(int x, int y, const LevelMap &map) const;
        bool canClimbDown(int x, int y, const LevelMap &map) const;
        bool canOccupy(int x, int y, const LevelMap &map) const;
        bool canPassThrough(int fx, int fy, int tx, int ty, const LevelMap &map) const;
        bool isFrozenClimbSpot(const LevelMap &map) const;

        void clampToMap(const LevelMap &map);
        void switchWalkAnim(int dirX, int dirY);
        void switchToStandAnim();
        bool isOnTileCenter() const;
        void checkTileCrossing(const LevelMap &map);
        void startForwardSnap(const LevelMap &map);
        void updateState(const LevelMap &map);
        SpriteLayout::WalkAnim directionToWalkAnim(int dx, int dy) const;
    };

} // namespace vovochka