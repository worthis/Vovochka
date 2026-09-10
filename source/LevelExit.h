#pragma once

#include "MapData.h"
#include "Animation.h"
#include "raylib.h"

namespace vovochka
{

    class LevelExit
    {
    public:
        void init(const LevelMap &map, const SpriteSheetGPU *exitSheet,
                  int tileX, int tileY,
                  float tileW, float tileH);
        int getTileX() const { return m_tileX; }
        int getTileY() const { return m_tileY; }
        void update(float dt);
        void draw() const;
        void setActive(bool active) { m_active = active; };
        bool isActive() const { return m_active; }
        bool checkCollision(const Rectangle &playerBounds) const;

    private:
        Vector2 m_pos{};
        int m_tileX = 0, m_tileY = 0;
        float m_tileW = 80, m_tileH = 80;
        bool m_active = false;
        bool m_flipped = false;
        Animation m_anim;

        float surfaceYForTile(int tileY) const;
    };

} // namespace vovochka