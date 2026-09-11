#include "SpriteSheet.h"
#include "IniReader.h"
#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdlib>

#include "raylib.h"

namespace vovochka
{

    SpriteSheetManager::~SpriteSheetManager() { unload(); }

    void SpriteSheetManager::computeVisibleBounds(SpriteSheetGPU &sheet, const Image &img)
    {
        sheet.visibleBounds.resize(sheet.frameCount);

        const int cellW = sheet.patternW + sheet.skipW;
        const int cellH = sheet.patternH + sheet.skipH;

        for (int frameIdx = 0; frameIdx < sheet.frameCount; ++frameIdx)
        {
            const int ix = frameIdx % sheet.cols;
            const int iy = frameIdx / sheet.cols;

            const int startX = sheet.skipW + ix * cellW;
            const int startY = sheet.skipH + iy * cellH;

            int minX = sheet.patternW, minY = sheet.patternH;
            int maxX = -1, maxY = -1;

            for (int y = 0; y < sheet.patternH; ++y)
            {
                for (int x = 0; x < sheet.patternW; ++x)
                {
                    Color c = GetImageColor(img, startX + x, startY + y);
                    if (c.a > 0) // непрозрачный пиксель
                    {
                        if (x < minX)
                            minX = x;
                        if (y < minY)
                            minY = y;
                        if (x > maxX)
                            maxX = x;
                        if (y > maxY)
                            maxY = y;
                    }
                }
            }

            if (maxX < 0) // полностью прозрачный кадр
            {
                sheet.visibleBounds[frameIdx] = Rectangle{0, 0, 0, 0};
            }
            else
            {
                sheet.visibleBounds[frameIdx] = Rectangle{static_cast<float>(minX),
                                                          static_cast<float>(minY),
                                                          static_cast<float>(maxX - minX + 1),
                                                          static_cast<float>(maxY - minY + 1)};
            }
        }
    }

    bool SpriteSheetManager::loadFromIni(const std::string &iniPath, const std::string &graphicsDir)
    {
        IniReader ini;
        if (!ini.loadFile(iniPath))
        {
            TraceLog(LOG_WARNING, "INI not loaded: %s", iniPath.c_str());
            return false;
        }

        const auto fileIndex = buildFileIndex(graphicsDir);

        int loaded = 0;
        for (const std::string &sec : ini.sections())
        {
            SpriteSheetDef def;
            def.name = sec;
            def.pictureW = ini.getInt(sec, "PictureWidth", 0);
            def.pictureH = ini.getInt(sec, "PictureHeight", 0);
            def.patternW = ini.getInt(sec, "PatternWidth", 0);
            def.patternH = ini.getInt(sec, "PatternHeight", 0);
            def.skipW = ini.getInt(sec, "SkipWidth", 0);
            def.skipH = ini.getInt(sec, "SkipHeight", 0);
            def.transparent = ini.getBool(sec, "Transparent", false);
            def.transparentColor = ini.getColor(sec, "TransparentColor", 0x00FF00);

            if (def.patternW <= 0 || def.patternH <= 0)
            {
                TraceLog(LOG_WARNING, "Skipping '%s': invalid pattern %dx%d", sec.c_str(), def.patternW, def.patternH);
                continue;
            }

            // Имя файла = имя секции.
            //   фон (bg*) -> .png (сконвертированный из оригинального JPG)
            //   остальное -> .bmp
            const std::string base = toLower(sec);
            const bool isBackground = (base.rfind("bg", 0) == 0);
            const char *ext = isBackground ? ".png" : ".bmp";

            auto it = fileIndex.find(base + ext);
            if (it == fileIndex.end())
            {
                if (isBackground)
                    TraceLog(LOG_WARNING,
                             "No converted PNG for '%s' in '%s'. "
                             "Run: python tools/convert_jpg_to_png.py <data_dir>",
                             sec.c_str(), graphicsDir.c_str());
                else
                    TraceLog(LOG_WARNING, "Image not found: '%s%s' in '%s'",
                             sec.c_str(), ext, graphicsDir.c_str());
                continue;
            }

            def.file = it->second.filename().string();

            const std::string normalizedPath = normalizePath(it->second.string().c_str());

            Image img = LoadImage(normalizedPath.c_str());
            if (!img.data)
            {
                TraceLog(LOG_WARNING, "Failed to load image: %s", normalizedPath.c_str());
                continue;
            }

            if (def.transparent)
            {
                applyChromaKey(img, unpackColor(def.transparentColor));
            }

            SpriteSheetGPU gpu;
            gpu.texture = LoadTextureFromImage(img);
            
            SetTextureFilter(gpu.texture, isBackground ? TEXTURE_FILTER_BILINEAR
                                                       : TEXTURE_FILTER_POINT);

            gpu.patternW = def.patternW;
            gpu.patternH = def.patternH;
            gpu.skipW = def.skipW;
            gpu.skipH = def.skipH;

            const int cellW = gpu.patternW + gpu.skipW;
            const int cellH = gpu.patternH + gpu.skipH;
            gpu.cols = (cellW > 0) ? (gpu.texture.width / cellW) : 1;
            gpu.rows = (cellH > 0) ? (gpu.texture.height / cellH) : 1;
            gpu.frameCount = gpu.cols * gpu.rows;

            computeVisibleBounds(gpu, img);

            UnloadImage(img);

            // Секция уже была из COMMON — LEVEL-версия перезаписывает.
            auto existing = m_sheets.find(sec);
            if (existing != m_sheets.end())
            {
                UnloadTexture(existing->second.texture);
                existing->second = gpu;
            }
            else
            {
                m_sheets.emplace(sec, gpu);
            }
            ++loaded;

            TraceLog(LOG_INFO, "Loaded '%s' -> %s: %d frames (%dx%d)",
                     sec.c_str(), def.file.c_str(),
                     gpu.frameCount, gpu.patternW, gpu.patternH);
        }

        return loaded > 0;
    }

    void SpriteSheetManager::unload()
    {
        for (auto &[name, gpu] : m_sheets)
        {
            if (gpu.texture.id != 0)
            {
                UnloadTexture(gpu.texture);
                gpu.texture = {};
            }
        }
        m_sheets.clear();
    }

    const SpriteSheetGPU *SpriteSheetManager::get(const std::string &name) const
    {
        auto it = m_sheets.find(name);
        return (it != m_sheets.end()) ? &it->second : nullptr;
    }

    bool SpriteSheetManager::has(const std::string &name) const
    {
        return m_sheets.find(name) != m_sheets.end();
    }

    std::vector<std::string> SpriteSheetManager::names() const
    {
        std::vector<std::string> r;
        r.reserve(m_sheets.size());
        for (const auto &[name, _] : m_sheets)
            r.push_back(name);
        std::sort(r.begin(), r.end());
        return r;
    }

} // namespace vovochka