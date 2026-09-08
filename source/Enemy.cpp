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

        updateClimbing(map);
    }

    int Enemy::patW() const { return m_animWalk.sheet ? m_animWalk.sheet->patternW : 104; }
    int Enemy::patH() const { return m_animWalk.sheet ? m_animWalk.sheet->patternH : 128; }

    void Enemy::update(float dt, const LevelMap &map)
    {
        if (!alive)
            return;

        updateClimbing(map);

        if (m_attacking)
        {
            m_animAttack.update(dt);
            m_attackT += dt;
            if (!m_animAttack.block.loop &&
                m_attackT >= m_attackDuration)
            {
                m_attacking = false;
                chooseDirection(dt, map);
            }
            return;
        }

        if (m_snapping)
        {
            moveTowardsSnap(dt, map);
            m_animWalk.update(dt);
            if (!m_snapping)
                chooseDirection(dt, map);
            clampToMap(map);
            return;
        }

        chooseDirection(dt, map);
        m_animWalk.update(dt);
        clampToMap(map);
    }

    void Enemy::startAttack(bool faceRight, bool loop)
    {
        if (!m_animAttack.sheet ||
            m_animAttack.sheet->frameCount < 2)
            return;

        m_attacking = true;
        m_attackT = 0.0f;
        m_snapping = false;
        m_dirX = m_dirY = 0;

        const int half = m_animAttack.sheet->frameCount / 2;
        m_animAttack.setBlock(FrameBlock{faceRight ? 0 : half, half, loop});
        m_attackDuration = half * m_animAttack.frameTime;
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
        if (!pathDone && m_thinkTimer < kThinkTimer)
            return;

        m_thinkTimer = 0.0f;

        if (playerPower >= powerThreshold)
            buildPath(map, playerTileX, playerTileY); // ОХОТА
        else
            buildRandomPath(map); // бродит по карте
    }

    void Enemy::chooseDirection(float dt, const LevelMap &map)
    {
        m_thinkTimer += dt;

        // обычным врагам путь строится здесь; боссу — в bossThink
        if (!isBoss &&
            (m_thinkTimer >= kThinkTimer || m_path.empty() || m_pathIdx >= m_path.size()))
        {
            m_thinkTimer = 0.0f;
            buildRandomPath(map);
        }

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