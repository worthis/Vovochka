#include "LevelExit.h"
#include "SpriteLayout.h"
#include <cmath>

namespace vovochka
{

    void LevelExit::init(const LevelMap &map, const SpriteSheetGPU *exitSheet,
                         int tileX, int tileY,
                         float tileW, float tileH)
    {
        m_tileW = tileW;
        m_tileH = tileH;
        m_tileX = tileX;
        m_tileY = tileY;
        m_active = false;
        m_flipped = GetRandomValue(0, 1) > 0 ? true : false;

        int pw = exitSheet ? exitSheet->patternW : 80;
        int ph = exitSheet ? exitSheet->patternH : 80;

        m_pos.x = m_tileX * m_tileW + (m_tileW - pw) * 0.5f;
        m_pos.y = surfaceYForTile(m_tileY) - ph;

        m_anim.sheet = exitSheet;
        if (exitSheet && exitSheet->frameCount > 1)
        {
            m_anim.setBlock(FrameBlock{0, exitSheet->frameCount, true});
            m_anim.frameTime = 0.08f;
        }
    }

    void LevelExit::update(float dt)
    {
        m_anim.update(dt);
    }

    void LevelExit::draw() const
    {
        if (!m_active ||
            !m_anim.sheet)
            return;

        m_anim.draw(m_pos.x, m_pos.y, m_flipped);
    }

    bool LevelExit::checkCollision(const Rectangle &playerBounds) const
    {
        if (!m_active)
            return false;

        Rectangle vb = m_anim.getVisibleBounds();
        Rectangle exitBounds{m_pos.x + vb.x, m_pos.y + vb.y, vb.width, vb.height};

        return CheckCollisionRecs(playerBounds, exitBounds);
    }

    float LevelExit::surfaceYForTile(int tileY) const
    {
        return (tileY + 1) * m_tileH - m_tileH * 0.4f;
    }

} // namespace vovochka