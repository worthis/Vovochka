#pragma once

#include "MapData.h"
#include "raylib.h"

namespace vovochka
{
    constexpr float kCenterEps = 4.0f;  // допуск попадания в центр тайла при входе на лестницу
    constexpr float kSurfaceEps = 3.0f; // допуск "на уровне земли" для схода с лестницы

    class Character
    {
    public:
        virtual ~Character() = default;

        Vector2 getPixelPos() const { return m_pos; }
        int getTileX() const { return m_tileX; }
        int getTileY() const { return m_tileY; }
        virtual Rectangle getBounds() const;
        float getFeetY() const { return m_pos.y + patH(); }

        void placeAt(int tileX, int tileY);

        bool isClimbing() const { return m_climbing; }

    protected:
        Vector2 m_pos{};
        int m_tileX = 0, m_tileY = 0;
        int m_dirX = 0, m_dirY = 0, m_lastDirX = 0, m_lastDirY = 0;
        float m_speed = 200.0f;
        float m_tileW = 80, m_tileH = 80;
        bool m_climbing = false;
        bool m_snapping = false;
        int m_snapTargetX = 0, m_snapTargetY = 0;

        bool canClimbUp(int x, int y, const LevelMap &map) const;
        bool canClimbDown(int x, int y, const LevelMap &map) const;
        bool canOccupy(int x, int y, const LevelMap &map) const;
        bool canPassThrough(int fx, int fy, int tx, int ty, const LevelMap &map) const;

        void getAllowed(const LevelMap &map, bool &L, bool &R, bool &U, bool &D) const;
        void getAllowedAt(const LevelMap &map, int x, int y,
                          bool &L, bool &R, bool &U, bool &D) const;

        float surfaceYForTile(int tileY) const;

        void clampToMap(const LevelMap &map);
        void updateClimbing(const LevelMap &map);
        void moveTowardsSnap(float dt, const LevelMap &map);

        virtual int patW() const = 0;
        virtual int patH() const = 0;
    };

} // namespace vovochka