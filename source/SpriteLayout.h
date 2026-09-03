#pragma once

namespace vovochka
{
    // Блок кадров внутри спрайт-листа.
    struct FrameBlock
    {
        int first = 0;
        int count = 1;
        bool loop = true;
    };

    // Восстановленные раскладки оригинальных листов:
    //
    // PlayerStand0 (16 кадров):  [0..7]  — стоит вправо, [8..15] — стоит влево.
    // Girl1Wait    (2 кадра):    статично; 0 — девушка ждёт, 1 — девушки нет.
    // Enemy1Go / Enemy2Go (32):  [0..7]   — бег вправо,
    //                            [8..15]  — подъём по лестнице,
    //                            [16..23] — бег влево,
    //                            [24..31] — спуск по лестнице.
    struct SpriteLayout
    {
        // Игрок
        static FrameBlock playerStand(bool faceRight)
        {
            return faceRight ? FrameBlock{0, 8, true} : FrameBlock{8, 8, true};
        }

        // Девушка (без анимации)
        static FrameBlock girlWait(bool present)
        {
            return present ? FrameBlock{0, 1, false} : FrameBlock{1, 1, false};
        }

        // Враги (Enemy1Go / Enemy2Go)
        static FrameBlock enemyRunRight() { return {0, 8, true}; }
        static FrameBlock enemyClimbUp() { return {8, 8, true}; }
        static FrameBlock enemyRunLeft() { return {16, 8, true}; }
        static FrameBlock enemyClimbDown() { return {24, 8, true}; }
    };

} // namespace vovochka