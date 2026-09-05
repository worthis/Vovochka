#include "Enemy.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace vovochka
{

    void Enemy::init(const LevelMap &map,
                     const SpriteSheetGPU *walkSheet,
                     const SpriteSheetGPU *attackSheet,
                     int tileX, int tileY, float speedPx, bool boss,
                     float tileW, float tileH)
    {
        m_tileW = tileW;
        m_tileH = tileH;
        m_speed = speedPx;
        isBoss = boss;

        m_tileX = tileX;
        m_tileY = tileY;
        const int pw = walkSheet ? walkSheet->patternW : 104;
        const int ph = walkSheet ? walkSheet->patternH : 128;
        m_pos.x = tileX * m_tileW + (m_tileW - pw) * 0.5f;
        m_pos.y = surfaceYForTile(tileY) - ph;

        m_animWalk.sheet = walkSheet;
        if (walkSheet)
            m_animWalk.setBlock(SpriteLayout::walk(SpriteLayout::WalkAnim::Right, walkSheet->frameCount));
        m_animWalk.frameTime = 0.08f;

        m_animAttack.sheet = attackSheet;
        m_animAttack.frameTime = 0.08f;

        m_dirX = 1; // старт вправо; первое решение — в update
        m_snapping = false;
    }

    int Enemy::patW() const { return m_animWalk.sheet ? m_animWalk.sheet->patternW : 104; }
    int Enemy::patH() const { return m_animWalk.sheet ? m_animWalk.sheet->patternH : 128; }
    float Enemy::surfaceYForTile(int tileY) const
    {
        return (tileY + 1) * m_tileH - m_tileH * 0.4f;
    }

    Rectangle Enemy::getBounds() const
    {
        return Rectangle{m_pos.x, m_pos.y, (float)patW(), (float)patH()};
    }

    void Enemy::update(float dt, const LevelMap &map)
    {
        if (!alive)
            return;

        if (m_attacking)
        {
            m_attackT += dt;
            m_animAttack.update(dt);
            if (m_attackT >= m_attackDuration)
            {
                m_attacking = false;
                chooseDirection(map); // отыграл — вернулся в патруль
            }
            return;
        }

        if (m_snapping)
        {
            stepSnap(dt, map);
            m_animWalk.update(dt);
            if (!m_snapping)
                chooseDirection(map); // доехали до центра — новое решение
            clampToMap(map);
            return;
        }

        chooseDirection(map); // стоим в центре — выбираем, куда идти
        m_animWalk.update(dt);
        clampToMap(map);
    }

    void Enemy::getAllowed(const LevelMap &map, bool &L, bool &R, bool &U, bool &D) const
    {
        L = R = U = D = false;
        const TileKind here = map.kindAt(m_tileX, m_tileY);
        const TileKind above = map.kindAt(m_tileX, m_tileY - 1);
        const TileKind below = map.kindAt(m_tileX, m_tileY + 1);

        switch (here)
        {
        case TileKind::Platform:
            L = canOccupy(m_tileX - 1, m_tileY, map);
            R = canOccupy(m_tileX + 1, m_tileY, map);
            if (above == TileKind::Ladder || above == TileKind::LadderBase)
                U = canClimbUp(m_tileX, m_tileY - 1, map);
            break;
        case TileKind::Ladder:
            U = canClimbUp(m_tileX, m_tileY - 1, map);
            D = canClimbDown(m_tileX, m_tileY + 1, map);
            break;
        case TileKind::LadderBase:
            L = canOccupy(m_tileX - 1, m_tileY, map);
            R = canOccupy(m_tileX + 1, m_tileY, map);
            if (above != TileKind::LadderTop)
                U = canClimbUp(m_tileX, m_tileY - 1, map);
            if (below == TileKind::Ladder || below == TileKind::LadderBase)
                D = true;
            break;
        case TileKind::LadderTop:
            L = canOccupy(m_tileX - 1, m_tileY, map);
            R = canOccupy(m_tileX + 1, m_tileY, map);
            if (below == TileKind::Ladder || below == TileKind::LadderBase)
                D = true;
            break;
        default:
            break;
        }
    }

    void Enemy::chooseDirection(const LevelMap &map)
    {
        bool L, R, U, D;
        getAllowed(map, L, R, U, D);

        struct Dir
        {
            int x, y;
        };
        std::vector<Dir> opts;
        if (L)
            opts.push_back({-1, 0});
        if (R)
            opts.push_back({1, 0});
        if (U)
            opts.push_back({0, -1});
        if (D)
            opts.push_back({0, 1});

        if (opts.empty())
        {
            m_dirX = m_dirY = 0;
            return;
        }

        auto has = [&](Dir d)
        {
            return std::find_if(opts.begin(), opts.end(), [&](Dir o)
                                { return o.x == d.x && o.y == d.y; }) != opts.end();
        };

        Dir pick{m_dirX, m_dirY};
        const Dir reverse{-m_dirX, -m_dirY};

        // 70% — прямо, иначе случайный не-разворот; разворот — только в тупике
        if (!(has(pick) && (std::rand() % 100) < 70))
        {
            std::vector<Dir> nonRev;
            for (Dir d : opts)
                if (!(d.x == reverse.x && d.y == reverse.y))
                    nonRev.push_back(d);
            pick = nonRev.empty()
                       ? reverse
                       : nonRev[static_cast<size_t>(std::rand()) % nonRev.size()];
        }

        m_dirX = pick.x;
        m_dirY = pick.y;
        m_snapTargetX = m_tileX + pick.x;
        m_snapTargetY = m_tileY + pick.y;
        m_snapping = true;

        // анимация по направлению
        SpriteLayout::WalkAnim a;
        if (m_dirY != 0)
            a = (m_dirY < 0) ? SpriteLayout::WalkAnim::Up : SpriteLayout::WalkAnim::Down;
        else if (m_dirX > 0)
            a = SpriteLayout::WalkAnim::Right;
        else
            a = SpriteLayout::WalkAnim::Left;
        if (a != m_currentWalkAnim && m_animWalk.sheet)
        {
            m_currentWalkAnim = a;
            m_animWalk.setBlock(SpriteLayout::walk(a, m_animWalk.sheet->frameCount));
        }
    }

    void Enemy::stepSnap(float dt, const LevelMap &map)
    {
        const int pw = patW(), ph = patH();
        const float tx = m_snapTargetX * m_tileW + (m_tileW - pw) * 0.5f;
        const float ty = surfaceYForTile(m_snapTargetY) - ph;

        const float ddx = tx - m_pos.x;
        const float ddy = ty - m_pos.y;
        const float dist = std::sqrt(ddx * ddx + ddy * ddy);

        if (dist < 2.0f)
        {
            m_pos.x = tx;
            m_pos.y = ty;
            m_tileX = m_snapTargetX;
            m_tileY = m_snapTargetY;
            m_snapping = false;
        }
        else
        {
            float step = m_speed * dt;
            if (step > dist)
                step = dist;
            m_pos.x += (ddx / dist) * step;
            m_pos.y += (ddy / dist) * step;
        }
    }

    bool Enemy::canClimbUp(int x, int y, const LevelMap &map) const
    {
        if (!map.inBounds(x, y))
            return false;
        TileKind k = map.kindAt(x, y);
        return k == TileKind::Ladder || k == TileKind::LadderBase;
    }

    bool Enemy::canClimbDown(int x, int y, const LevelMap &map) const
    {
        if (!map.inBounds(x, y))
            return false;
        TileKind k = map.kindAt(x, y);
        return k == TileKind::Ladder || k == TileKind::LadderBase ||
               k == TileKind::LadderTop || k == TileKind::Platform;
    }

    bool Enemy::canOccupy(int x, int y, const LevelMap &map) const
    {
        if (!map.inBounds(x, y))
            return false;
        TileKind k = map.kindAt(x, y);
        return k == TileKind::Platform || k == TileKind::Ladder ||
               k == TileKind::LadderBase || k == TileKind::LadderTop;
    }

    void Enemy::clampToMap(const LevelMap &map)
    {
        const int pw = patW(), ph = patH();
        float cx = std::clamp(m_pos.x + pw * 0.5f, 0.0f, map.width * m_tileW);
        m_pos.x = cx - pw * 0.5f;
        float feet = std::clamp(m_pos.y + ph, 0.0f, map.height * m_tileH);
        m_pos.y = feet - ph;
    }

    bool Enemy::isOnLadder(const LevelMap &map) const
    {
        const TileKind k = map.kindAt(m_tileX, m_tileY);
        return k == TileKind::Ladder || k == TileKind::LadderBase || k == TileKind::LadderTop;
    }

    void Enemy::startAttack(bool faceRight)
    {
        if (!m_animAttack.sheet || m_animAttack.sheet->frameCount < 2)
            return;

        m_attacking = true;
        m_attackT = 0.0f;
        m_snapping = false;
        m_dirX = m_dirY = 0;

        const int half = m_animAttack.sheet->frameCount / 2;
        m_animAttack.setBlock(FrameBlock{faceRight ? 0 : half, half, false});
        m_attackDuration = half * m_animAttack.frameTime;
    }

    void Enemy::draw() const
    {
        if (m_attacking)
        {
            m_animAttack.draw(m_pos.x, m_pos.y, false);
            return;
        }
        if (alive)
            m_animWalk.draw(m_pos.x, m_pos.y, false);
    }

} // namespace vovochka