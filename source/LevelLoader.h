#pragma once

#include "MapData.h"
#include "SpriteSheet.h"
#include <string>

namespace vovochka
{

    // Параметры из Levels.cfg — [Difficulty1..3], выбираются перед игрой
    struct DifficultyParams
    {
        int scoreLevel = 35;
        int bombCost = 1;
        int playerLifeMax = 8;
        int enemyCountMax = 3;
        int condomCount = 88;
        int playerStrengthCan = 4;
        float playerSpeed = 0.080f;
        float enemySpeed = 0.015f;
        float enemyGirlSpeed = 0.025f;
        int enemyGirlLifeMax = 1;
    };

    // Читает карты (.map) и спрайт-листы (GameSprites.dat) из папки data.
    // Иерархия папок (из оригинала):
    //   data/MAP/LevelN.map
    //   data/COMMON/LEVELGRAPHICS/GameSprites.dat   <- общие спрайты
    //   data/LEVEL<N>/GRAPHICS/GameSprites.dat      <- уровень-специфичные
    //   data/LEVEL<N>/GRAPHICS/*.bmp|jpg            <- картинки спрайтов
    class LevelLoader
    {
    public:
        explicit LevelLoader(std::string dataRoot);

        // Путь к файлу карты: data/MAP/LevelN.map
        std::string mapPath(int level) const;

        bool loadDifficulty(int n, DifficultyParams &out) const; // n = 1..3

        // Парсит бинарный .map-файл. Порядок тайлов — x * height + y (столбцы).
        bool loadMap(int level, LevelMap &out) const;

        // Загружает спрайт-листы из двух INI:
        //   1) COMMON/LEVELGRAPHICS/GameSprites.dat
        //   2) LEVEL<N>/GRAPHICS/GameSprites.dat (может переопределять COMMON)
        bool loadLevelAssets(int level, SpriteSheetManager &outSheets) const;

        // Пути к папкам графики (для загрузки BMP по Picture= из INI)
        std::string levelGraphicsPath(int level, const std::string &rel = "") const;
        std::string commonGraphicsPath(const std::string &rel = "") const;

        const std::string &dataRoot() const { return m_root; }

    private:
        std::string m_root;
    };

} // namespace vovochka