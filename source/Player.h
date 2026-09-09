#pragma once
#include "Animation.h"
#include "MapData.h"
#include "Character.h"
#include "raylib.h"

namespace vovochka
{

    class Player : public Character
    {
    public:
        void init(const LevelMap &map,
                  const SpriteSheetGPU *standSheet,
                  const SpriteSheetGPU *walkSheet,
                  const SpriteSheetGPU *makeBombSheet,
                  const SpriteSheetGPU *hurtSheet,
                  float tileW, float tileH, float speedPx);

        Rectangle getBounds() const override;

        void setInputEnabled(bool b) { m_inputEnabled = b; }
        void update(float dt, const LevelMap &map, int inX, int inY, bool wantBomb);
        void draw() const;

        void cancelMakeBomb();
        bool isMakingBomb() const { return m_makingBomb || m_wantMakeBomb; }
        bool popMakeBombFinished();
        void startHurt();
        bool isHurt() const { return m_hurt; }

    protected:
        int patW() const override;
        int patH() const override;

    private:
        bool m_inputEnabled = true;

        Animation m_animStand;
        Animation m_animWalk;
        Animation m_animBomb;
        Animation m_animHurt;

        SpriteLayout::WalkAnim m_currentWalkAnim = SpriteLayout::WalkAnim::Right;

        bool m_makingBomb = false;
        float m_makeBombT = 0.0f;
        float m_makeBombDuration = 0.0f;
        bool m_makeBombPending = false;
        bool m_makeBombFinishedFlag = false;
        bool m_wantMakeBomb = false;
        bool m_hurt = false;
        float m_hurtT = 0.0f;
        float m_hurtDuration = 0.0f;

        Rectangle offsetBounds(const Animation &anim) const;
        void beginBombAnim();
        void switchWalkAnim(int dirX, int dirY);
        void switchToStandAnim();
        bool isOnTileCenter() const;
        void checkTileCrossing(const LevelMap &map);
        void startForwardSnap(const LevelMap &map);
        SpriteLayout::WalkAnim directionToWalkAnim(int dx, int dy) const;
    };

} // namespace vovochka