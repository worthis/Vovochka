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
                  const SpriteSheetGPU *deathSheet,
                  float tileW, float tileH, float speedPx);

        Rectangle getBounds() const override;
        int patW() const override;
        int patH() const override;

        void setInputEnabled(bool b) { m_inputEnabled = b; }
        void update(float dt, const LevelMap &map, int inX, int inY, bool wantBomb);
        void draw() const;

        void startDeath();
        bool isDying() const { return m_dying; }
        bool isDeathFinished() const;
        float deathDuration() const { return m_deathDuration; }

        void cancelMakeBomb();
        bool isMakingBomb() const { return m_makingBomb || m_wantMakeBomb; }
        bool popMakeBombFinished();
        void startHurt();
        bool isHurt() const { return m_hurt; }

    private:
        static constexpr float kDeathFallSpeed = 300.0f;

        bool m_inputEnabled = true;

        Animation m_animStand;
        Animation m_animWalk;
        Animation m_animBomb;
        Animation m_animHurt;
        Animation m_animDeath;

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
        bool m_dying = false;
        float m_deathT = 0.0f;
        float m_deathDuration = 0.0f;
        float m_deathVelX = 0.0f, m_deathVelY = 0.0f;

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