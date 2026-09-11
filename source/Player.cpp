#include "Player.h"
#include "SpriteLayout.h"
#include <cmath>
#include <algorithm>

namespace vovochka
{

    void Player::init(const LevelMap &map,
                      const SpriteSheetGPU *standSheet,
                      const SpriteSheetGPU *walkSheet,
                      const SpriteSheetGPU *makeBombSheet,
                      const SpriteSheetGPU *hurtSheet,
                      const SpriteSheetGPU *deathSheet,
                      float tileW, float tileH, float speedPx)
    {
        // --- полный сброс состояния (критично при смене уровня) ---
        m_dirX = m_dirY = 0;
        m_lastDirX = m_lastDirY = 0;
        m_speed = speedPx;
        m_snapTargetX = m_snapTargetY = 0;
        m_snapping = false;
        m_facingRight = true;
        m_currentWalkAnim = SpriteLayout::WalkAnim::Right;

        m_animBomb.sheet = makeBombSheet;
        m_animBomb.frameTime = 0.08f;

        m_animHurt.sheet = hurtSheet;
        m_animHurt.frameTime = 0.08f;

        m_animDeath.sheet = deathSheet;
        m_animDeath.frameTime = 0.08f;
        m_dying = false;
        m_deathT = 0.0f;

        m_tileW = tileW;
        m_tileH = tileH;

        // --- позиция из точки спавна уровня ---
        if (auto start = map.playerStart())
        {
            m_tileX = start->x;
            m_tileY = start->y;
        }
        else
        {
            m_tileX = m_tileY = 0;
        }

        int pw = standSheet ? standSheet->patternW : 104;
        int ph = standSheet ? standSheet->patternH : 104;
        m_pos.x = m_tileX * m_tileW + (m_tileW - pw) * 0.5f;
        m_pos.y = surfaceYForTile(m_tileY) - ph;

        // --- анимации в начальное состояние ---
        m_animStand.sheet = standSheet;
        m_animStand.setBlock(SpriteLayout::playerStand(true, standSheet ? standSheet->frameCount : 16));
        m_animStand.frameTime = 0.10f;
        m_animStand.timer = 0.0f;
        m_animStand.frame = 0;

        m_animWalk.sheet = walkSheet;
        m_animWalk.setBlock(SpriteLayout::walk(SpriteLayout::WalkAnim::Right, walkSheet ? walkSheet->frameCount : 32));
        m_animWalk.frameTime = 0.10f;
        m_animWalk.timer = 0.0f;
        m_animWalk.frame = 0;

        m_lastTileX = m_tileX;
        m_lastTileY = m_tileY;
        m_tileChangedFlag = false;

        updateClimbing(map);
    }

    int Player::patW() const { return m_animWalk.sheet ? m_animWalk.sheet->patternW : 104; }
    int Player::patH() const { return m_animWalk.sheet ? m_animWalk.sheet->patternH : 104; }

    Rectangle Player::getBounds() const
    {
        if (m_hurt)
            return offsetBounds(m_animHurt);

        if (m_makingBomb)
            return offsetBounds(m_animBomb);

        if (m_snapping ||
            m_climbing ||
            m_dirX != 0 ||
            m_dirY != 0)
            return offsetBounds(m_animWalk);

        return offsetBounds(m_animStand);
    }

    Rectangle Player::offsetBounds(const Animation &anim) const
    {
        Rectangle vb = anim.getVisibleBounds();
        return Rectangle{m_pos.x + vb.x, m_pos.y + vb.y, vb.width, vb.height};
    }

    void Player::update(float dt, const LevelMap &map, int inX, int inY, bool wantBomb)
    {
        const int pw = patW();

        if (m_dying)
        {
            m_deathT += dt;
            m_animDeath.update(dt);
            m_pos.x += m_deathVelX * dt;
            m_pos.y += m_deathVelY * dt;
            return;
        }

        updateClimbing(map);
        updateTileChanging();

        // --- получение урона: стоим, анимация один раз ---
        if (m_hurt)
        {
            m_hurtT += dt;
            m_animHurt.update(dt);
            if (m_hurtT >= m_hurtDuration)
            {
                m_hurt = false;
                switchToStandAnim();
                m_animStand.update(dt);
            }
            return;
        }

        // --- подготовка к бомбе: сначала дойти ВПЕРЁД с ходьбой, потом анимация ---
        if (m_wantMakeBomb)
        {
            if (m_snapping)
            {
                moveTowardsSnap(dt, map);
                switchWalkAnim(m_lastDirX, m_lastDirY); // ходьба как обычно
                m_animWalk.update(dt);
                if (!m_snapping)
                    beginBombAnim(); // доехали — запускаем анимацию
            }
            else
            {
                beginBombAnim();
            }
            clampToMap(map);
            return;
        }

        // --- сама анимация создания бомбы: стоим на центре тайла ---
        if (m_makingBomb)
        {
            m_makeBombT += dt;
            m_animBomb.update(dt);
            if (m_makeBombT >= m_makeBombDuration)
            {
                m_makingBomb = false;
                m_makeBombFinishedFlag = m_makeBombPending;
                m_makeBombPending = false;
                switchToStandAnim();
                m_animStand.update(dt);
            }
            return;
        }

        // --- сигнал на бомбу извне ---
        if (wantBomb && m_inputEnabled)
        {
            m_dirX = m_dirY = 0;

            // стоим на центре (или направления нет) — анимация сразу
            if (isOnTileCenter() ||
                (m_lastDirX == 0 && m_lastDirY == 0))
            {
                beginBombAnim();
                return;
            }

            const float centerX = m_pos.x + patW() * 0.5f;
            const float colC = m_tileX * m_tileW + m_tileW * 0.5f;
            const float feet = m_pos.y + patH();
            const float rowC = surfaceYForTile(m_tileY);

            // ближайший центр тайла ВПЕРЕД по последнему направлению
            int tx = m_tileX, ty = m_tileY;
            if (m_lastDirX > 0)
                tx = (centerX <= colC) ? m_tileX : m_tileX + 1;
            if (m_lastDirX < 0)
                tx = (centerX >= colC) ? m_tileX : m_tileX - 1;
            if (m_lastDirY > 0)
                ty = (feet <= rowC) ? m_tileY : m_tileY + 1;
            if (m_lastDirY < 0)
                ty = (feet >= rowC) ? m_tileY : m_tileY - 1;

            // вперёд закрыто — НЕ откатываемся: бомба на месте
            if ((tx != m_tileX || ty != m_tileY) &&
                !canPassThrough(m_tileX, m_tileY, tx, ty, map))
            {
                beginBombAnim();
                return;
            }

            // идём вперёд с ходьбой, анимация бомбы — после прибытия
            m_wantMakeBomb = true;
            m_snapping = true;
            m_snapTargetX = tx;
            m_snapTargetY = ty;
            return;
        }

        if (!m_inputEnabled)
        {
            inX = inY = 0;
        }

        // --- разрешённые направления по правилам тайлов ---
        bool allowLeft = false, allowRight = false, allowUp = false, allowDown = false;
        getAllowed(map, allowLeft, allowRight, allowUp, allowDown);

        // вертикаль приоритетнее, если разрешена
        int dirX = 0, dirY = 0;
        if (inY != 0 && ((inY < 0 && allowUp) || (inY > 0 && allowDown)))
            dirY = inY;
        if (dirY == 0 && inX != 0 && ((inX < 0 && allowLeft) || (inX > 0 && allowRight)))
            dirX = inX;

        const bool hasInput = (dirX != 0 || dirY != 0);

        // ================= ДВИЖЕНИЕ =================
        if (hasInput)
        {
            m_snapping = false;
            m_dirX = dirX;
            m_dirY = dirY;
            m_lastDirX = dirX;
            m_lastDirY = dirY;
            m_facingRight = (dirX > 0);

            const float colCenter = m_tileX * m_tileW + m_tileW * 0.5f;

            if (dirY != 0)
            {
                m_pos.x = colCenter - pw * 0.5f;
                m_pos.y += dirY * m_speed * dt;
            }
            else
            {
                m_pos.x += dirX * m_speed * dt;
            }

            checkTileCrossing(map);
            switchWalkAnim(dirX, dirY);
            m_animWalk.update(dt);
            clampToMap(map);
            return;
        }

        // ================= НЕТ ВВОДА =================
        m_dirX = 0;
        m_dirY = 0;

        if (m_snapping)
        {
            moveTowardsSnap(dt, map);
            if (m_snapping)
            {
                // ещё едем — анимация ходьбы в последнем направлении
                switchWalkAnim(m_lastDirX, m_lastDirY);
                m_animWalk.update(dt);
            }
            else if (!m_climbing)
            {
                switchToStandAnim();
                m_animStand.update(dt);
            }
            clampToMap(map);
            return;
        }

        // стоим: если не на центре тайла — доводим вперёд до тайла
        if (!isOnTileCenter())
        {
            startForwardSnap(map);
            switchWalkAnim(m_lastDirX, m_lastDirY);
            m_animWalk.update(dt);
        }
        else if (!m_climbing)
        {
            switchToStandAnim();
            m_animStand.update(dt);
        }

        clampToMap(map);
    }

    void Player::checkTileCrossing(const LevelMap &map)
    {
        const int pw = patW();
        int nx = static_cast<int>((m_pos.x + pw * 0.5f) / m_tileW);
        int ny = static_cast<int>(getFeetY() / m_tileH);

        if (nx == m_tileX && ny == m_tileY)
            return;

        bool ok = false;
        if (ny != m_tileY && nx == m_tileX)
            ok = canPassThrough(m_tileX, m_tileY, nx, ny, map);
        else if (nx != m_tileX && ny == m_tileY)
            ok = canPassThrough(m_tileX, m_tileY, nx, ny, map);

        if (ok)
        {
            m_tileX = nx;
            m_tileY = ny;
            updateClimbing(map);
        }
        else
        {
            // уткнулись — snap к текущему тайлу (на уровень земли)
            m_snapping = true;
            m_snapTargetX = m_tileX;
            m_snapTargetY = m_tileY;
        }
    }

    void Player::startForwardSnap(const LevelMap &map)
    {
        int tx = m_tileX + m_lastDirX;
        int ty = m_tileY + m_lastDirY;

        if (m_lastDirX == 0 && m_lastDirY == 0)
        {
            tx = m_tileX;
            ty = m_tileY;
        }

        bool ok = (tx != m_tileX || ty != m_tileY)
                      ? canPassThrough(m_tileX, m_tileY, tx, ty, map)
                      : true;
        if (!ok)
        {
            tx = m_tileX;
            ty = m_tileY;
        }

        m_snapping = true;
        m_snapTargetX = tx;
        m_snapTargetY = ty;
    }

    SpriteLayout::WalkAnim Player::directionToWalkAnim(int dx, int dy) const
    {
        if (m_climbing && dy != 0)
            return (dy < 0) ? SpriteLayout::WalkAnim::Up : SpriteLayout::WalkAnim::Down;
        if (dx != 0)
            return (dx > 0) ? SpriteLayout::WalkAnim::Right : SpriteLayout::WalkAnim::Left;
        if (dy != 0)
            return (dy < 0) ? SpriteLayout::WalkAnim::Up : SpriteLayout::WalkAnim::Down;
        return m_currentWalkAnim;
    }

    void Player::switchWalkAnim(int dirX, int dirY)
    {
        if (!m_animWalk.sheet)
            return;
        SpriteLayout::WalkAnim desired = directionToWalkAnim(dirX, dirY);
        if (desired != m_currentWalkAnim)
        {
            m_currentWalkAnim = desired;
            m_animWalk.setBlock(SpriteLayout::walk(desired, m_animWalk.sheet->frameCount));
        }
    }

    void Player::switchToStandAnim()
    {
        if (!m_animStand.sheet)
            return;
        FrameBlock desired = SpriteLayout::playerStand(m_facingRight, m_animStand.sheet->frameCount);
        if (m_animStand.block.first != desired.first ||
            m_animStand.block.count != desired.count)
        {
            m_animStand.setBlock(desired);
        }
    }

    bool Player::isOnTileCenter() const
    {
        const int pw = patW(), ph = patH();
        float cx = m_tileX * m_tileW + m_tileW * 0.5f;
        float cy = surfaceYForTile(m_tileY);
        return std::abs((m_pos.x + pw * 0.5f) - cx) < 2.0f &&
               std::abs((m_pos.y + ph) - cy) < 2.0f;
    }

    void Player::draw() const
    {
        if (m_dying)
        {
            m_animDeath.draw(m_pos.x, m_pos.y, false);
            return;
        }

        if (m_hurt)
        {
            m_animHurt.draw(m_pos.x, m_pos.y, false);
            return;
        }

        if (m_makingBomb)
        {
            m_animBomb.draw(m_pos.x, m_pos.y, false);
            return;
        }

        bool moving = m_snapping || m_dirX != 0 || m_dirY != 0;

        if (moving || m_climbing)
            m_animWalk.draw(m_pos.x, m_pos.y, false);
        else
            m_animStand.draw(m_pos.x, m_pos.y, false);
    }

    void Player::beginBombAnim()
    {
        if (!m_animBomb.sheet ||
            m_animBomb.sheet->frameCount < 2)
            return;

        m_wantMakeBomb = false;
        m_makingBomb = true;
        m_makeBombPending = true;
        m_makeBombT = 0.0f;
        m_snapping = false;

        const int half = m_animBomb.sheet->frameCount / 2;
        m_animBomb.setBlock(FrameBlock{m_facingRight ? 0 : half, half, false});
        m_makeBombDuration = half * m_animBomb.frameTime;
    }

    void Player::cancelMakeBomb()
    {
        m_wantMakeBomb = false;
        m_makingBomb = false;
        m_makeBombPending = false;
    }

    bool Player::popMakeBombFinished()
    {
        const bool f = m_makeBombFinishedFlag;
        m_makeBombFinishedFlag = false;
        return f;
    }

    void Player::startHurt()
    {
        if (!m_animHurt.sheet || m_animHurt.sheet->frameCount < 2)
            return;

        m_hurt = true;
        m_hurtT = 0.0f;
        m_snapping = false;
        m_dirX = m_dirY = 0;

        // две половинки: вправо / влево — по текущей ориентации
        const int half = m_animHurt.sheet->frameCount / 2;
        m_animHurt.setBlock(FrameBlock{m_facingRight ? 0 : half, half, false});
        m_hurtDuration = half * m_animHurt.frameTime;
    }

    void Player::startDeath()
    {
        if (m_dying)
            return;

        m_dying = true;
        m_deathT = 0.0f;
        m_deathDuration = 3.0f;
        m_snapping = false;
        m_dirX = m_dirY = 0;
        m_makingBomb = false;
        m_wantMakeBomb = false;
        m_hurt = false;

        // Две половины листа: 0..half-1 — вправо, half..end — влево; НЕ зациклена
        if (m_animDeath.sheet && m_animDeath.sheet->frameCount >= 2)
        {
            const int half = m_animDeath.sheet->frameCount / 2;
            m_animDeath.setBlock(FrameBlock{m_facingRight ? 0 : half, half, false});
        }

        // Падение под 45°: вниз + в сторону от взгляда
        m_deathVelX = (m_facingRight ? -1.0f : 1.0f) * kDeathFallSpeed;
        m_deathVelY = kDeathFallSpeed;
    }

    bool Player::isDeathFinished() const
    {
        return m_dying &&
               m_deathT >= m_deathDuration;
    }

} // namespace vovochka