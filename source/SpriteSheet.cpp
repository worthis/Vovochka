#include "SpriteSheet.h"
#include "IniReader.h"

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

    namespace fs = std::filesystem;

    namespace
    {

        std::string toLowerCopy(std::string s)
        {
            for (auto &c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }

        std::string normalizePath(const fs::path &p)
        {
            std::string s = p.string();
            std::replace(s.begin(), s.end(), '\\', '/');
            return s;
        }

        // Индекс файлов папки: lower-case имя файла -> реальный путь.
        // Регистр на диске может отличаться от имени секции
        // (bg1 -> BG1.JPG, Set2 -> SET2.BMP, Girl1Wait -> Girl1Wait.bmp).
        std::unordered_map<std::string, fs::path> buildFileIndex(const fs::path &dir)
        {
            std::unordered_map<std::string, fs::path> idx;
            std::error_code ec;
            if (!fs::is_directory(dir, ec))
                return idx;

            for (const auto &e : fs::directory_iterator(dir, ec))
            {
                if (ec)
                    break;
                std::error_code ec2;
                if (!e.is_regular_file(ec2))
                    continue;
                idx.emplace(toLowerCopy(e.path().filename().string()), e.path());
            }
            return idx;
        }

    } // namespace

    SpriteSheetManager::~SpriteSheetManager() { unload(); }

    Color SpriteSheetManager::unpackColor(uint32_t rgb)
    {
        return Color{static_cast<unsigned char>((rgb >> 16) & 0xFF),
                     static_cast<unsigned char>((rgb >> 8) & 0xFF),
                     static_cast<unsigned char>(rgb & 0xFF),
                     255};
    }

    // Ручной chroma-key: все пиксели, близкие к key, делаем полностью прозрачными.
    // Не зависит от особенностей ImageColorReplace (точное совпадение + альфа).
    static void applyChromaKey(Image &img, Color key, int tolerance = 8)
    {
        // Принудительно RGBA8 — гарантированно есть альфа-канал
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        // Диагностика: цвет углового пикселя (у тайлсетов это фон-лайм)
        Color corner = GetImageColor(img, 0, 0);
        TraceLog(LOG_INFO, "ChromaKey: corner pixel = (%d, %d, %d, %d)",
                 corner.r, corner.g, corner.b, corner.a);

        auto *p = static_cast<unsigned char *>(img.data);
        const int n = img.width * img.height;
        int keyed = 0;

        for (int i = 0; i < n; ++i)
        {
            unsigned char *c = p + i * 4;
            const int dr = (int)c[0] - (int)key.r;
            const int dg = (int)c[1] - (int)key.g;
            const int db = (int)c[2] - (int)key.b;

            if (std::abs(dr) <= tolerance && std::abs(dg) <= tolerance && std::abs(db) <= tolerance)
            {
                c[3] = 0;
                ++keyed;
            }
        }

        TraceLog(LOG_INFO, "ChromaKey: %d / %d pixels made transparent", keyed, n);
    }

    bool SpriteSheetManager::loadFromIni(const std::string &iniPath,
                                         const std::string &graphicsDir)
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
                TraceLog(LOG_WARNING, "Skipping '%s': invalid pattern %dx%d",
                         sec.c_str(), def.patternW, def.patternH);
                continue;
            }

            // Имя файла = имя секции.
            //   фон (bg*) -> .png (сконвертированный из оригинального JPG)
            //   остальное -> .bmp
            const std::string base = toLowerCopy(sec);
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
                // ImageColorReplace(&img, unpackColor(def.transparentColor), BLANK);
                applyChromaKey(img, unpackColor(def.transparentColor));
            }

            SpriteSheetGPU gpu;
            gpu.texture = LoadTextureFromImage(img);
            gpu.patternW = def.patternW;
            gpu.patternH = def.patternH;
            gpu.skipW = def.skipW;
            gpu.skipH = def.skipH;

            const int cellW = gpu.patternW + gpu.skipW;
            const int cellH = gpu.patternH + gpu.skipH;
            gpu.cols = (cellW > 0) ? (gpu.texture.width / cellW) : 1;
            gpu.rows = (cellH > 0) ? (gpu.texture.height / cellH) : 1;
            gpu.frameCount = gpu.cols * gpu.rows;

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