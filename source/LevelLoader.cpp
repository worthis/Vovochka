#include "LevelLoader.h"
#include "IniReader.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include "raylib.h"

namespace vovochka
{

    LevelLoader::LevelLoader(std::string dataRoot) : m_root(std::move(dataRoot)) {}

    std::string LevelLoader::mapPath(int n) const
    {
        std::ostringstream s;
        s << m_root << "/MAP/Level" << n << ".map";
        return s.str();
    }

    std::string LevelLoader::levelGraphicsPath(int n, const std::string &rel) const
    {
        std::ostringstream s;
        s << m_root << "/LEVEL" << n << "/GRAPHICS";
        if (!rel.empty())
            s << "/" << rel;
        return s.str();
    }

    std::string LevelLoader::commonGraphicsPath(const std::string &rel) const
    {
        std::ostringstream s;
        s << m_root << "/COMMON/LEVELGRAPHICS";
        if (!rel.empty())
            s << "/" << rel;
        return s.str();
    }

    static std::string readAll(const std::string &path)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f)
            return {};
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    template <class T>
    static T readLE(const char *p)
    {
        T v{};
        std::memcpy(&v, p, sizeof(T));
        return v;
    }

    bool LevelLoader::loadMap(int n, LevelMap &out) const
    {
        const std::string path = mapPath(n);
        const std::string buf = readAll(path);
        if (buf.size() < 8)
        {
            TraceLog(LOG_ERROR, "Map file too small: %s", path.c_str());
            return false;
        }

        out.width = readLE<int32_t>(&buf[0]);
        out.height = readLE<int32_t>(&buf[4]);
        if (out.width <= 0 || out.height <= 0)
        {
            TraceLog(LOG_ERROR, "Invalid map dimensions: %dx%d", out.width, out.height);
            return false;
        }

        const size_t tileCount = static_cast<size_t>(out.width) * out.height;
        const size_t tilesEnd = 8 + tileCount * 3;
        if (buf.size() < tilesEnd + 4)
        {
            TraceLog(LOG_ERROR, "Map truncated before object block: %s", path.c_str());
            return false;
        }

        // Тайлы: [set][index][attr], порядок x * height + y
        out.tiles.resize(tileCount);
        for (size_t i = 0; i < tileCount; ++i)
        {
            out.tiles[i].set = static_cast<uint8_t>(buf[8 + i * 3 + 0]);
            out.tiles[i].index = static_cast<uint8_t>(buf[8 + i * 3 + 1]);
            out.tiles[i].attr = static_cast<uint8_t>(buf[8 + i * 3 + 2]);
        }

        // Блок объектов: count i32, затем count * 9 байт [x i32][y i32][type u8]
        const int32_t count = readLE<int32_t>(&buf[tilesEnd]);
        size_t off = tilesEnd + 4;

        out.objects.clear();
        out.objects.reserve(count > 0 ? static_cast<size_t>(count) : 0u);
        for (int32_t i = 0; i < count; ++i)
        {
            if (off + 9 > buf.size())
            {
                TraceLog(LOG_WARNING, "Map: object list truncated at %d/%d", i, count);
                break;
            }
            MapObject o;
            o.x = readLE<int32_t>(&buf[off]);
            o.y = readLE<int32_t>(&buf[off + 4]);
            o.type = static_cast<MapObjectType>(buf[off + 8]);
            out.objects.push_back(o);
            off += 9;
        }

        TraceLog(LOG_INFO, "Loaded %s: %dx%d, %zu tiles, %zu objects",
                 path.c_str(), out.width, out.height,
                 out.tiles.size(), out.objects.size());
        return true;
    }

    bool LevelLoader::loadLevelAssets(int n, SpriteSheetManager &sheets) const
    {
        // 1) Общие спрайты (PlayerGo, Enemy2Go, Condom, LevelExit, Energy, ...)
        const std::string commonIni = commonGraphicsPath("GameSprites.dat");
        const std::string commonDir = commonGraphicsPath();
        if (!sheets.loadFromIni(commonIni, commonDir))
        {
            TraceLog(LOG_WARNING, "COMMON GameSprites.dat not loaded: %s", commonIni.c_str());
        }

        // 2) Уровень-специфичные спрайты (bg1, Set2, Enemy1*, Girl1*)
        //    Могут переопределять записи с теми же именами секций из COMMON.
        const std::string levelIni = levelGraphicsPath(n, "GameSprites.dat");
        const std::string levelDir = levelGraphicsPath(n);
        if (!sheets.loadFromIni(levelIni, levelDir))
        {
            TraceLog(LOG_ERROR, "LEVEL%d GameSprites.dat failed: %s", n, levelIni.c_str());
            return false;
        }

        return true;
    }

    bool LevelLoader::loadDifficulty(int n, DifficultyParams &out) const
    {
        IniReader ini;
        const std::string path = m_root + "/COMMON/GAMECONFIGINF/Levels.cfg";
        if (!ini.loadFile(path))
        {
            TraceLog(LOG_WARNING, "Levels.cfg not loaded: %s", path.c_str());
            return false;
        }

        const std::string sec = "Difficulty" + std::to_string(n);
        if (!ini.hasSection(sec))
        {
            TraceLog(LOG_WARNING, "No section [%s] in Levels.cfg", sec.c_str());
            return false;
        }

        out.scoreLevel = ini.getInt(sec, "ScoreLevel", out.scoreLevel);
        out.bombCost = ini.getInt(sec, "BombCost", out.bombCost);
        out.playerLifeMax = ini.getInt(sec, "PlayerLifeMax", out.playerLifeMax);
        out.enemyCountMax = ini.getInt(sec, "EnemyCountMax", out.enemyCountMax);
        out.condomCount = ini.getInt(sec, "CondomCount", out.condomCount);
        out.playerStrengthCan = ini.getInt(sec, "PlayerStrengthCan", out.playerStrengthCan);
        out.playerSpeed = ini.getFloat(sec, "PlayerSpeed", out.playerSpeed);
        out.enemySpeed = ini.getFloat(sec, "EnemySpeed", out.enemySpeed);
        out.enemyGirlSpeed = ini.getFloat(sec, "EnemyGirlSpeed", out.enemyGirlSpeed);
        out.enemyGirlLifeMax = ini.getInt(sec, "EnemyGirlLifeMax", out.enemyGirlLifeMax);

        TraceLog(LOG_INFO, "Difficulty%d: condoms=%d enemies=%d score=%d speeds=%.3f/%.3f/%.3f",
                 n, out.condomCount, out.enemyCountMax, out.scoreLevel,
                 out.playerSpeed, out.enemySpeed, out.enemyGirlSpeed);

        return true;
    }

} // namespace vovochka