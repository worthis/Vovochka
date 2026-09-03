#pragma once

namespace vovochka
{

    struct FrameBlock
    {
        int first = 0;
        int count = 1;
        bool loop = true;
    };

    // Раскладки спрайтов. Количество кадров на анимацию вычисляется
    // автоматически: totalFrames / animCount.
    //
    // PlayerStand0: 16 кадров, 2 анимации (вправо, влево) → 8 кадров каждая
    // PlayerGo:     32 кадра, 4 анимации (→, ↑, ←, ↓)   → 8 кадров каждая
    // Enemy1Go/2Go: 32 кадра, 4 анимации (→, ↑, ←, ↓)   → 8 кадров каждая
    // Girl1Wait:     2 кадра, 2 состояния (ждёт, нет)     → 1 кадр каждое
    struct SpriteLayout
    {

        // --- PlayerStand0: 2 анимации ---
        static FrameBlock playerStand(bool faceRight, int totalFrames = 16)
        {
            int perAnim = totalFrames / 2;
            return faceRight
                       ? FrameBlock{0, perAnim, true}
                       : FrameBlock{perAnim, perAnim, true};
        }

        // --- PlayerGo / EnemyGo: 4 анимации ---
        enum class WalkAnim
        {
            Right,
            Up,
            Left,
            Down
        };

        static FrameBlock walk(WalkAnim anim, int totalFrames = 32)
        {
            int perAnim = totalFrames / 4;
            switch (anim)
            {
            case WalkAnim::Right:
                return {perAnim * 0, perAnim, true};
            case WalkAnim::Up:
                return {perAnim * 1, perAnim, true};
            case WalkAnim::Left:
                return {perAnim * 2, perAnim, true};
            case WalkAnim::Down:
                return {perAnim * 3, perAnim, true};
            }
            return {0, perAnim, true};
        }

        // --- Girl1Wait: 2 состояния ---
        static FrameBlock girlWait(bool present, int totalFrames = 2)
        {
            int perAnim = totalFrames / 2;
            return present
                       ? FrameBlock{0, perAnim, false}
                       : FrameBlock{perAnim, perAnim, false};
        }
    };

} // namespace vovochka