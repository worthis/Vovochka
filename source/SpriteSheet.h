#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "raylib.h"

namespace vovochka
{

    // Описание одного спрайт-листа — секции [Name] в GameSprites.dat.
    struct SpriteSheetDef
    {
        std::string name; // имя секции, например "Set2", "PlayerGo", "Girl1Wait"
        std::string file; // имя файла (Picture=...), по умолчанию "<name>.bmp"

        int pictureW = 0; // ширина атласа спрайтов
        int pictureH = 0; // высота атласа спрайтов
        int patternW = 0; // ширина спрайта
        int patternH = 0; // высота спрайта
        int skipW = 0;
        int skipH = 0;

        bool transparent = false;
        uint32_t transparentColor = 0x00FF00; // clLime

        int frameCount(int texW, int texH) const noexcept
        {
            const int cellW = patternW + skipW;
            const int cellH = patternH + skipH;
            if (cellW <= 0 || cellH <= 0)
                return 0;
            return (texW / cellW) * (texH / cellH);
        }
    };

    // GPU-представление спрайт-листа: текстура + параметры нарезки.
    struct SpriteSheetGPU
    {
        Texture2D texture{};
        int patternW = 0;
        int patternH = 0;
        int skipW = 0;
        int skipH = 0;
        int cols = 0;
        int rows = 0;
        int frameCount = 0;

        std::vector<Rectangle> visibleBounds;

        bool valid() const noexcept { return texture.id != 0; }

        Rectangle frame(int index) const noexcept
        {
            const int cellW = patternW + skipW;
            const int cellH = patternH + skipH;
            const int ix = index % cols;
            const int iy = index / cols;
            return Rectangle{static_cast<float>(skipW + ix * cellW),
                             static_cast<float>(skipH + iy * cellH),
                             static_cast<float>(patternW),
                             static_cast<float>(patternH)};
        }
    };

    // Общий менеджер всех спрайт-листов (и тайлсетов, и одиночных спрайтов).
    class SpriteSheetManager
    {
    public:
        SpriteSheetManager() = default;
        ~SpriteSheetManager();

        SpriteSheetManager(const SpriteSheetManager &) = delete;
        SpriteSheetManager &operator=(const SpriteSheetManager &) = delete;
        SpriteSheetManager(SpriteSheetManager &&) noexcept = default;
        SpriteSheetManager &operator=(SpriteSheetManager &&) noexcept = default;

        // Парсит INI и грузит все секции как спрайт-листы.
        // graphicsDir — папка, откуда берутся BMP/JPG по Picture=<file>.
        bool loadFromIni(const std::string &iniPath, const std::string &graphicsDir);
        void unload();

        const SpriteSheetGPU *get(const std::string &name) const;
        bool has(const std::string &name) const;

        // Все имена загруженных листов
        std::vector<std::string> names() const;

    private:
        std::unordered_map<std::string, SpriteSheetGPU> m_sheets;
        static Color unpackColor(uint32_t rgb);
        static void computeVisibleBounds(SpriteSheetGPU& sheet, const Image& img);
    };

} // namespace vovochka