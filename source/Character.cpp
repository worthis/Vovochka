#include "Character.h"
#include <cmath>
#include <algorithm>

namespace vovochka
{

    Rectangle Character::getBounds() const
    {
        return Rectangle{m_pos.x, m_pos.y,
                         static_cast<float>(patW()),
                         static_cast<float>(patH())};
    }

    void Character::placeAt(int tileX, int tileY)
    {
        m_tileX = tileX;
        m_tileY = tileY;
        m_pos.x = tileX * m_tileW + (m_tileW - patW()) * 0.5f;
        m_pos.y = surfaceYForTile(tileY) - patH();
        m_dirX = m_dirY = m_lastDirX = m_lastDirY = 0;
        m_snapping = false;
    }

    float Character::surfaceYForTile(int tileY) const
    {
        return (tileY + 1) * m_tileH - m_tileH * 0.4f;
    }

    // Вверх: Ladder и LadderBase, но НЕ LadderTop
    bool Character::canClimbUp(int x, int y, const LevelMap &map) const
    {
        if (!map.inBounds(x, y))
            return false;
        TileKind k = map.kindAt(x, y);
        return k == TileKind::Ladder || k == TileKind::LadderBase;
    }

    // Вниз: лестница любого типа или платформа (сход)
    bool Character::canClimbDown(int x, int y, const LevelMap &map) const
    {
        if (!map.inBounds(x, y))
            return false;
        TileKind k = map.kindAt(x, y);
        return k == TileKind::Ladder || k == TileKind::LadderBase ||
               k == TileKind::LadderTop || k == TileKind::Platform;
    }

    bool Character::canOccupy(int x, int y, const LevelMap &map) const
    {
        if (!map.inBounds(x, y))
            return false;
        TileKind k = map.kindAt(x, y);
        return k == TileKind::Platform || k == TileKind::Ladder ||
               k == TileKind::LadderBase || k == TileKind::LadderTop;
    }

    // Направленная проверка перехода
    bool Character::canPassThrough(int fx, int fy, int tx, int ty, const LevelMap &map) const
    {
        if (!map.inBounds(tx, ty))
            return false;
        TileKind from = map.kindAt(fx, fy);

        // горизонталь
        if (fy == ty)
        {
            if (from == TileKind::Ladder)
                return false; // с лестницы горизонтально не сходят
            return canOccupy(tx, ty, map);
        }

        // вертикаль
        if (fx == tx)
        {
            if (ty < fy)
                return canClimbUp(tx, ty, map); // LadderTop заблокирован
            return canClimbDown(tx, ty, map);
        }

        // диагональ запрещена
        return false;
    }

    void Character::getAllowed(const LevelMap &map, bool &L, bool &R, bool &U, bool &D) const
    {
        getAllowedAt(map, m_tileX, m_tileY, L, R, U, D);
    }

    void Character::getAllowedAt(const LevelMap &map, int x, int y,
                                 bool &L, bool &R, bool &U, bool &D) const
    {
        L = R = U = D = false;

        const TileKind here = map.kindAt(x, y);
        const TileKind above = map.kindAt(x, y - 1);
        const TileKind below = map.kindAt(x, y + 1);

        const float centerX = m_pos.x + patW() * 0.5f;
        const float colCenter = m_tileX * m_tileW + m_tileW * 0.5f;
        const bool aligned = std::abs(centerX - colCenter) <= kCenterEps;
        const bool atSurface = std::abs(getFeetY() - surfaceYForTile(m_tileY)) <= kSurfaceEps;

        switch (here)
        {
        case TileKind::Platform:
            L = canOccupy(x - 1, y, map);
            R = canOccupy(x + 1, y, map);
            if ((above == TileKind::Ladder || above == TileKind::LadderBase) && aligned)
                U = canClimbUp(x, y - 1, map);
            break;
        case TileKind::Ladder:
            U = canClimbUp(x, y - 1, map);
            D = canClimbDown(x, y + 1, map);
            break;
        case TileKind::LadderBase:
            L = canOccupy(x - 1, y, map) && atSurface;
            R = canOccupy(x + 1, y, map) && atSurface;
            if (above != TileKind::LadderTop && aligned)
                U = canClimbUp(x, y - 1, map);
            if ((below == TileKind::Ladder || below == TileKind::LadderBase) && aligned)
                D = true;
            break;
        case TileKind::LadderTop:
            L = canOccupy(x - 1, y, map);
            R = canOccupy(x + 1, y, map);
            if ((below == TileKind::Ladder || below == TileKind::LadderBase) && aligned)
                D = true;
            break;
        default:
            break;
        }
    }

    void Character::clampToMap(const LevelMap &map)
    {
        const int pw = patW(), ph = patH();
        const float mapW = map.width * m_tileW;
        const float mapH = map.height * m_tileH;

        float cx = m_pos.x + pw * 0.5f;
        cx = std::clamp(cx, 0.0f, mapW);
        m_pos.x = cx - pw * 0.5f;

        float feet = m_pos.y + ph;
        feet = std::clamp(feet, 0.0f, mapH);
        m_pos.y = feet - ph;
    }

    void Character::updateClimbing(const LevelMap &map)
    {
        m_climbing = false;

        const TileKind k = map.kindAt(m_tileX, m_tileY);
        if (k == TileKind::Ladder)
            m_climbing = true;
        if (k == TileKind::LadderBase)
            m_climbing = std::abs(getFeetY() - surfaceYForTile(m_tileY)) > kSurfaceEps;
    }

    void Character::moveTowardsSnap(float dt, const LevelMap &map)
    {
        const float targetPxX = m_snapTargetX * m_tileW + (m_tileW - patW()) * 0.5f;
        const float targetPxY = surfaceYForTile(m_snapTargetY) - patH();
        const float ddx = targetPxX - m_pos.x;
        const float ddy = targetPxY - m_pos.y;
        const float dist = std::sqrt(ddx * ddx + ddy * ddy);

        if (dist < 2.0f)
        {
            // достигли цели
            m_pos.x = targetPxX;
            m_pos.y = targetPxY;
            m_tileX = m_snapTargetX;
            m_tileY = m_snapTargetY;
            m_snapping = false;
            updateClimbing(map);
        }
        else
        {
            // ещё не достигли
            float step = m_speed * dt;
            if (step > dist)
                step = dist;
            m_pos.x += (ddx / dist) * step;
            m_pos.y += (ddy / dist) * step;
        }

        clampToMap(map);
    }

    void Character::updateTileChanging()
    {
        if (m_tileX != m_lastTileX || m_tileY != m_lastTileY)
        {
            m_lastTileX = m_tileX;
            m_lastTileY = m_tileY;
            m_tileChangedFlag = true;
        }
    }

    bool Character::popTileChanged()
    {
        bool changed = m_tileChangedFlag;
        m_tileChangedFlag = false;
        return changed;
    }

} // namespace vovochka