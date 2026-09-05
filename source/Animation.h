#pragma once

#include "SpriteLayout.h"
#include "SpriteSheet.h"
#include "raylib.h"

namespace vovochka
{

    // Покадровая анимация по блоку кадров спрайт-листа.
    struct Animation
    {
        const SpriteSheetGPU *sheet = nullptr;
        FrameBlock block{};
        float frameTime = 0.12f;
        float timer = 0.0f;
        int frame = 0; // индекс внутри блока

        void setBlock(const FrameBlock &b)
        {
            block = b;
            frame = 0;
            timer = 0.0f;
        }

        void update(float dt)
        {
            // статичный кадр
            if (!sheet || block.count <= 1)
                return;

            timer += dt;
            while (timer >= frameTime)
            {
                timer -= frameTime;
                ++frame;
                if (frame >= block.count)
                    frame = block.loop ? 0 : block.count - 1;
            }
        }

        // x, y — верхний левый угол спрайта.
        void draw(float x, float y, bool flipX = false) const
        {
            if (!sheet || !sheet->valid())
                return;
            Rectangle src = sheet->frame(block.first + frame);
            if (flipX)
                src.width = -src.width;
            Rectangle dst{x, y,
                          static_cast<float>(sheet->patternW),
                          static_cast<float>(sheet->patternH)};
            DrawTexturePro(sheet->texture, src, dst, {0, 0}, 0.0f, WHITE);
        }
    };

} // namespace vovochka