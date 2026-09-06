#include "Enemy.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <queue>

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
            m_animAttack.update(dt);
            m_attackT += dt;
            if (!m_animAttack.block.loop &&
                m_attackT >= m_attackDuration)
            {
                m_attacking = false;
                chooseDirection(map);
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

    void Enemy::startAttack(bool faceRight, bool loop)
    {
        if (!m_animAttack.sheet || m_animAttack.sheet->frameCount < 2)
            return;

        m_attacking = true;
        m_attackT = 0.0f;
        m_snapping = false;
        m_dirX = m_dirY = 0;

        const int half = m_animAttack.sheet->frameCount / 2;
        m_animAttack.setBlock(FrameBlock{faceRight ? 0 : half, half, loop});
        m_attackDuration = half * m_animAttack.frameTime;
    }

    void Enemy::stopAttack()
    {
        m_attacking = false;
    }

    void Enemy::getAllowedAt(const LevelMap &map, int x, int y,
                             bool &L, bool &R, bool &U, bool &D) const
    {
        L = R = U = D = false;
        const TileKind here = map.kindAt(x, y);
        const TileKind above = map.kindAt(x, y - 1);
        const TileKind below = map.kindAt(x, y + 1);

        switch (here)
        {
        case TileKind::Platform:
            L = canOccupy(x - 1, y, map);
            R = canOccupy(x + 1, y, map);
            if (above == TileKind::Ladder || above == TileKind::LadderBase)
                U = canClimbUp(x, y - 1, map);
            break;
        case TileKind::Ladder:
            U = canClimbUp(x, y - 1, map);
            D = canClimbDown(x, y + 1, map);
            break;
        case TileKind::LadderBase:
            L = canOccupy(x - 1, y, map);
            R = canOccupy(x + 1, y, map);
            if (above != TileKind::LadderTop)
                U = canClimbUp(x, y - 1, map);
            if (below == TileKind::Ladder || below == TileKind::LadderBase)
                D = true;
            break;
        case TileKind::LadderTop:
            L = canOccupy(x - 1, y, map);
            R = canOccupy(x + 1, y, map);
            if (below == TileKind::Ladder || below == TileKind::LadderBase)
                D = true;
            break;
        default:
            break;
        }
    }

    bool Enemy::bfs(const LevelMap &map, int sx, int sy, int tx, int ty,
                    std::vector<std::pair<int, int>> &out) const
    {
        out.clear();
        if (!map.inBounds(tx, ty))
            return false;
        if (sx == tx && sy == ty)
            return true;

        const int w = map.width;
        std::vector<int> prev(static_cast<size_t>(map.width) * map.height, -1);
        std::queue<int> q;
        auto id = [w](int x, int y)
        { return y * w + x; };

        prev[id(sx, sy)] = id(sx, sy);
        q.push(id(sx, sy));

        while (!q.empty())
        {
            const int cur = q.front();
            q.pop();
            const int cx = cur % w, cy = cur / w;
            if (cx == tx && cy == ty)
                break;

            bool L, R, U, D;
            getAllowedAt(map, cx, cy, L, R, U, D);
            const int dx[4] = {-1, 1, 0, 0};
            const int dy[4] = {0, 0, -1, 1};
            const bool ok[4] = {L, R, U, D};
            for (int k = 0; k < 4; ++k)
            {
                if (!ok[k])
                    continue;
                const int nx = cx + dx[k], ny = cy + dy[k];
                if (!map.inBounds(nx, ny) || prev[id(nx, ny)] != -1)
                    continue;
                prev[id(nx, ny)] = cur;
                q.push(id(nx, ny));
            }
        }

        if (prev[id(tx, ty)] == -1)
            return false;

        int cur = id(tx, ty);
        while (cur != id(sx, sy))
        {
            out.emplace_back(cur % w, cur / w);
            cur = prev[cur];
        }
        std::reverse(out.begin(), out.end());
        return true;
    }

    void Enemy::buildPath(const LevelMap &map, int tx, int ty)
    {
        m_path.clear();
        m_pathIdx = 0;
        bfs(map, m_tileX, m_tileY, tx, ty, m_path);
    }

    void Enemy::buildRandomPath(const LevelMap &map)
    {
        for (int attempt = 0; attempt < 16; ++attempt)
        {
            const int tx = std::rand() % map.width;
            const int ty = std::rand() % map.height;
            if (bfs(map, m_tileX, m_tileY, tx, ty, m_path) && !m_path.empty())
            {
                m_pathIdx = 0;
                return;
            }
        }
        m_path.clear();
        m_pathIdx = 0;
    }

    void Enemy::bossThink(float dt, const LevelMap &map,
                          int playerTileX, int playerTileY,
                          int playerPower, int powerThreshold)
    {
        m_thinkTimer += dt;

        const bool pathDone = m_path.empty() || m_pathIdx >= m_path.size();

        // не чаще 1000 мс, если пути нет — сразу
        if (!pathDone && m_thinkTimer < 1.0f)
            return;

        m_thinkTimer = 0.0f;

        if (playerPower >= powerThreshold)
            buildPath(map, playerTileX, playerTileY); // ОХОТА
        else
            buildRandomPath(map); // бродит по карте
    }

    void Enemy::chooseDirection(const LevelMap &map)
    {
        // обычным врагам путь строится здесь; боссу — в bossThink
        if (!isBoss &&
            (m_path.empty() || m_pathIdx >= m_path.size()))
            buildRandomPath(map);

        if (m_path.empty() ||
            m_pathIdx >= m_path.size())
        {
            m_dirX = m_dirY = 0;
            return;
        }

        const auto next = m_path[m_pathIdx++];
        m_dirX = next.first - m_tileX;
        m_dirY = next.second - m_tileY;

        // защита от рассинхрона пути
        if (std::abs(m_dirX) + std::abs(m_dirY) != 1)
        {
            m_path.clear();
            m_dirX = m_dirY = 0;
            return;
        }

        m_snapTargetX = next.first;
        m_snapTargetY = next.second;
        m_snapping = true;

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