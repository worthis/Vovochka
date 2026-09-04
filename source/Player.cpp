#include "Player.h"
#include "SpriteLayout.h"
#include <cmath>
#include <algorithm>

namespace vovochka
{

    namespace
    {
        // допуск попадания в центр тайла при входе на лестницу
        constexpr float kCenterEps = 4.0f;
        // допуск "на уровне земли" для схода с лестницы
        constexpr float kSurfaceEps = 3.0f;
    }

    void Player::init(const LevelMap &map,
                      const SpriteSheetGPU *standSheet,
                      const SpriteSheetGPU *walkSheet,
                      float tileW, float tileH)
    {
        // --- полный сброс состояния (критично при смене уровня) ---
        m_dirX = m_dirY = 0;
        m_lastDirX = m_lastDirY = 0;
        m_snapTargetX = m_snapTargetY = 0;
        m_snapping = false;
        m_xLocked = false;
        m_frozenClimb = false;
        m_facingRight = true;
        m_onLadder = false;
        m_prevOnLadder = false;
        m_currentWalkAnim = SpriteLayout::WalkAnim::Right;

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
        m_animWalk.frameTime = 0.06f;
        m_animWalk.timer = 0.0f;
        m_animWalk.frame = 0;

        updateState(map);
    }

    int Player::patW() const { return m_animWalk.sheet ? m_animWalk.sheet->patternW : 104; }
    int Player::patH() const { return m_animWalk.sheet ? m_animWalk.sheet->patternH : 104; }

    float Player::surfaceYForTile(int tileY) const
    {
        return (tileY + 1) * m_tileH - m_tileH * 0.4f;
    }

    void Player::update(float dt, const LevelMap &map)
    {
        const int pw = patW(), ph = patH();

        // --- Ввод ---
        int inX = 0, inY = 0;
        if (m_inputEnabled)
        {
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
                inX = -1;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
                inX = +1;
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
                inY = -1;
            if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
                inY = +1;
        }

        const TileKind here = map.kindAt(m_tileX, m_tileY);
        const TileKind above = map.kindAt(m_tileX, m_tileY - 1);
        const TileKind below = map.kindAt(m_tileX, m_tileY + 1);

        const float centerX = m_pos.x + pw * 0.5f;
        const float colCenter = m_tileX * m_tileW + m_tileW * 0.5f;
        const bool aligned = std::abs(centerX - colCenter) <= kCenterEps;
        const bool atSurface = std::abs(feetY() - surfaceYForTile(m_tileY)) <= kSurfaceEps;

        // --- разрешённые направления по правилам тайлов Set2 ---
        bool allowLeft = false, allowRight = false, allowUp = false, allowDown = false;

        switch (here)
        {
        case TileKind::Platform:
            allowLeft = canOccupy(m_tileX - 1, m_tileY, map);
            allowRight = canOccupy(m_tileX + 1, m_tileY, map);
            // вход на лестницу вверх — ТОЛЬКО по центру колонки
            if ((above == TileKind::Ladder || above == TileKind::LadderBase) && aligned)
                allowUp = true;
            break;

        case TileKind::Ladder:
            // на лестнице только вертикаль
            allowUp = canClimbUp(m_tileX, m_tileY - 1, map);
            allowDown = canClimbDown(m_tileX, m_tileY + 1, map);
            break;

        case TileKind::LadderBase:
            // сход влево/вправо — ТОЛЬКО на уровне земли
            allowLeft = atSurface && canOccupy(m_tileX - 1, m_tileY, map);
            allowRight = atSurface && canOccupy(m_tileX + 1, m_tileY, map);
            // вверх — запрещено, если сверху LadderTop; только по центру
            if (above != TileKind::LadderTop && aligned)
                allowUp = canClimbUp(m_tileX, m_tileY - 1, map);
            // вниз — только если снизу лестница и по центру
            if ((below == TileKind::Ladder || below == TileKind::LadderBase) && aligned)
                allowDown = true;
            break;

        case TileKind::LadderTop:
            allowLeft = canOccupy(m_tileX - 1, m_tileY, map);
            allowRight = canOccupy(m_tileX + 1, m_tileY, map);
            if ((below == TileKind::Ladder || below == TileKind::LadderBase) && aligned)
                allowDown = true;
            // вверх — никогда
            break;

        default:
            break;
        }

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
            m_frozenClimb = false;
            m_dirX = dirX;
            m_dirY = dirY;
            m_lastDirX = dirX;
            m_lastDirY = dirY;
            if (dirX != 0)
                m_facingRight = (dirX > 0);

            if (dirY != 0)
            {
                // вертикаль: одноразовая фиксация X по центру колонки при входе
                if (!m_xLocked)
                {
                    m_pos.x = colCenter - pw * 0.5f;
                    m_xLocked = true;
                }
                m_pos.y += dirY * m_speed * dt;
            }
            else
            {
                m_xLocked = false;
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
            float targetPxX = m_snapTargetX * m_tileW + (m_tileW - pw) * 0.5f;
            float targetPxY = surfaceYForTile(m_snapTargetY) - ph;

            float ddx = targetPxX - m_pos.x;
            float ddy = targetPxY - m_pos.y;
            float dist = std::sqrt(ddx * ddx + ddy * ddy);

            if (dist < 2.0f)
            {
                m_pos.x = targetPxX;
                m_pos.y = targetPxY;
                m_tileX = m_snapTargetX;
                m_tileY = m_snapTargetY;
                m_snapping = false;
                m_xLocked = false;
                updateState(map);
                m_frozenClimb = isFrozenClimbSpot(map);
                if (!m_frozenClimb)
                {
                    switchToStandAnim();
                    m_animStand.update(dt);
                }
            }
            else
            {
                float step = m_speed * dt;
                if (step > dist)
                    step = dist;
                m_pos.x += (ddx / dist) * step;
                m_pos.y += (ddy / dist) * step;
                switchWalkAnim(m_lastDirX, m_lastDirY);
                m_animWalk.update(dt);
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
        else
        {
            // стоим на центре тайла
            m_frozenClimb = isFrozenClimbSpot(map);

            if (m_frozenClimb)
            {
                // Анимация НЕ переключается на stand и НЕ обновляется:
                // текущий кадр карабкания замирает (игрок висит на лестнице).
                // Блок Up/Down уже выбран во время движения — просто не трогаем.
            }
            else
            {
                switchToStandAnim();
                m_animStand.update(dt);
            }
        }

        clampToMap(map);
    }

    bool Player::isClimbing() const
    {
        // на лестнице и движется по вертикали (или доводит вертикальный snap)
        return m_onLadder && (m_dirY != 0 || (m_snapping && m_lastDirY != 0));
    }

    // Точка, где карабкание замирает вместо стойки:
    //  - обычная лестница (земли там нет вообще);
    //  - стык «лестница×земля», но НЕ на уровне земли.
    // На платформе, LadderTop и на стыке НА уровне земли — обычная стойка.
    bool Player::isFrozenClimbSpot(const LevelMap &map) const
    {
        const TileKind k = map.kindAt(m_tileX, m_tileY);
        if (k == TileKind::Ladder)
            return true;
        if (k == TileKind::LadderBase)
            return std::abs(feetY() - surfaceYForTile(m_tileY)) > kSurfaceEps;
        return false;
    }

    Rectangle Player::getBounds() const
    {
        return Rectangle{m_pos.x, m_pos.y,
                         static_cast<float>(patW()),
                         static_cast<float>(patH())};
    }

    // Вверх: Ladder и LadderBase, но НЕ LadderTop  (FIX #1)
    bool Player::canClimbUp(int x, int y, const LevelMap &map) const
    {
        if (!map.inBounds(x, y))
            return false;
        TileKind k = map.kindAt(x, y);
        return k == TileKind::Ladder || k == TileKind::LadderBase;
    }

    // Вниз: лестница любого типа или платформа (сход)
    bool Player::canClimbDown(int x, int y, const LevelMap &map) const
    {
        if (!map.inBounds(x, y))
            return false;
        TileKind k = map.kindAt(x, y);
        return k == TileKind::Ladder || k == TileKind::LadderBase ||
               k == TileKind::LadderTop || k == TileKind::Platform;
    }

    bool Player::canOccupy(int x, int y, const LevelMap &map) const
    {
        if (!map.inBounds(x, y))
            return false;
        TileKind k = map.kindAt(x, y);
        return k == TileKind::Platform || k == TileKind::Ladder ||
               k == TileKind::LadderBase || k == TileKind::LadderTop;
    }

    // Направленная проверка перехода — используется и движением, и snap'ом (FIX #1)
    bool Player::canPassThrough(int fx, int fy, int tx, int ty, const LevelMap &map) const
    {
        if (!map.inBounds(tx, ty))
            return false;
        TileKind from = map.kindAt(fx, fy);

        if (fy == ty)
        { // горизонталь
            if (from == TileKind::Ladder)
                return false; // с лестницы горизонтально не сходят
            return canOccupy(tx, ty, map);
        }
        if (fx == tx)
        { // вертикаль
            if (ty < fy)
                return canClimbUp(tx, ty, map); // LadderTop заблокирован
            return canClimbDown(tx, ty, map);
        }
        return false; // диагональ запрещена
    }

    void Player::checkTileCrossing(const LevelMap &map)
    {
        const int pw = patW();
        int nx = static_cast<int>((m_pos.x + pw * 0.5f) / m_tileW);
        int ny = static_cast<int>(feetY() / m_tileH);

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
            updateState(map);
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

        // FIX #1: валидируем snap через canPassThrough, а не canOccupy.
        // Если вверх нельзя (LadderTop) — остаёмся на текущем тайле,
        // и snap доведёт до уровня земли (surface), а не до границы тайла.
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
        if (m_onLadder && dy != 0)
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

    void Player::updateState(const LevelMap &map)
    {
        TileKind k = map.kindAt(m_tileX, m_tileY);
        m_onLadder = (k == TileKind::Ladder || k == TileKind::LadderBase || k == TileKind::LadderTop);
        m_prevOnLadder = m_onLadder;
    }

    void Player::draw() const
    {
        bool moving = m_snapping || m_dirX != 0 || m_dirY != 0;
        if (moving || m_frozenClimb)
            m_animWalk.draw(m_pos.x, m_pos.y, false);
        else
            m_animStand.draw(m_pos.x, m_pos.y, false);
    }

    void Player::clampToMap(const LevelMap &map)
    {
        const int pw = patW(), ph = patH();
        const float mapW = map.width * m_tileW;
        const float mapH = map.height * m_tileH;

        // центр спрайта не выходит за [0, mapW]
        float cx = m_pos.x + pw * 0.5f;
        cx = std::clamp(cx, 0.0f, mapW);
        m_pos.x = cx - pw * 0.5f;

        // ноги не выходят за [0, mapH]
        float feet = m_pos.y + ph;
        feet = std::clamp(feet, 0.0f, mapH);
        m_pos.y = feet - ph;
    }

    void Player::placeAt(const LevelMap &map, int tileX, int tileY)
    {
        m_tileX = tileX;
        m_tileY = tileY;
        m_dirX = m_dirY = m_lastDirX = m_lastDirY = 0;
        m_snapping = false;
        m_xLocked = false;
        m_frozenClimb = false;
        const int pw = patW(), ph = patH();
        m_pos.x = tileX * m_tileW + (m_tileW - pw) * 0.5f;
        m_pos.y = surfaceYForTile(tileY) - ph;
        updateState(map);
    }

} // namespace vovochka