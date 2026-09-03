#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <optional>

namespace vovochka
{

    // ---------------------------------------------------------------------------
    // Константы, восстановленные из оригинала
    // (глобалы 0x495d5c / 0x495fd8 = TileW / TileH)
    // ---------------------------------------------------------------------------
    inline constexpr int kTileWidth = 80;
    inline constexpr int kTileHeight = 80;

    // Индекс тайла в наборе, означающий "пусто" (прозрачный слот).
    inline constexpr std::uint8_t kEmptyTileIndex = 9;

    // ---------------------------------------------------------------------------
    // Тайл карты: 3 байта в .map — [set][index][attr]
    //   set   -> секция [SetN] в GameSprites.dat (в уровнях всегда 2)
    //   index -> номер тайла внутри набора (0..9 для Set2)
    //   attr  -> атрибут/флаги (в известных уровнях всегда 0xFF)
    // ---------------------------------------------------------------------------
    struct MapTile
    {
        std::uint8_t set = 2;
        std::uint8_t index = kEmptyTileIndex;
        std::uint8_t attr = 0xFF;
    };

    // Семантика индексов Set2 (4 типа тайлов геометрии + пусто).
    // Если на рендере крышки лестниц окажутся перепутаны — меняем
    // LadderBase/LadderTop местами (проверяется визуально).
    enum class TileKind : std::uint8_t
    {
        Empty,      // 9
        Platform,   // 0
        Ladder,     // 1
        LadderBase, // 2 — лестница начинается от платформы и идёт вверх
        LadderTop,  // 3 — верхний конец лестницы
        Unknown
    };

    constexpr TileKind tileKindFromIndex(std::uint8_t index) noexcept
    {
        switch (index)
        {
        case 0:
            return TileKind::Platform;
        case 1:
            return TileKind::Ladder;
        case 2:
            return TileKind::LadderBase;
        case 3:
            return TileKind::LadderTop;
        case 9:
            return TileKind::Empty;
        default:
            return TileKind::Unknown;
        }
    }

    inline char tileKindToChar(TileKind k) noexcept
    {
        switch (k)
        {
        case TileKind::Platform:
            return '#';
        case TileKind::Ladder:
            return '|';
        case TileKind::LadderBase:
            return 'L';
        case TileKind::LadderTop:
            return 'T';
        case TileKind::Empty:
            return '.';
        default:
            return '?';
        }
    }

    // ---------------------------------------------------------------------------
    // Запись спавна из .map: 9 байт — [x i32][y i32][type u8]
    // Координаты — в ТАЙЛАХ.
    // ---------------------------------------------------------------------------
    enum class MapObjectType : std::uint8_t
    {
        Unknown = 0,
        Player = 1, // стартовая позиция игрока (ровно 1 на уровень)
        Girl = 2    // девушки
    };

    struct MapObject
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
        MapObjectType type = MapObjectType::Unknown;
    };

    // ---------------------------------------------------------------------------
    // Карта уровня целиком.
    // Порядок тайлов в файле: index = x * height + y  (столбцы! проверено:
    // лестницы становятся вертикальными только при таком порядке).
    // ---------------------------------------------------------------------------
    struct LevelMap
    {
        std::int32_t width = 0;
        std::int32_t height = 0;

        std::vector<MapTile> tiles;
        std::vector<MapObject> objects;

        void clear()
        {
            width = height = 0;
            tiles.clear();
            objects.clear();
        }

        bool isValid() const noexcept { return width > 0 && height > 0; }

        bool inBounds(int x, int y) const noexcept
        {
            return x >= 0 && y >= 0 && x < width && y < height;
        }

        std::size_t tileIndex(int x, int y) const noexcept
        {
            return static_cast<std::size_t>(x) * static_cast<std::size_t>(height) + static_cast<std::size_t>(y);
        }

        const MapTile &tileAt(int x, int y) const noexcept
        {
            static const MapTile kEmpty{};
            return inBounds(x, y) ? tiles[tileIndex(x, y)] : kEmpty;
        }

        TileKind kindAt(int x, int y) const noexcept
        {
            return tileKindFromIndex(tileAt(x, y).index);
        }

        // Коллизии: твёрдая только платформа; лестницы проходимы.
        bool isSolidAt(int x, int y) const noexcept { return kindAt(x, y) == TileKind::Platform; }
        bool isLadderAt(int x, int y) const noexcept
        {
            const TileKind k = kindAt(x, y);
            return k == TileKind::Ladder || k == TileKind::LadderBase || k == TileKind::LadderTop;
        }

        // --- спавны -------------------------------------------------------------
        std::optional<MapObject> findObject(MapObjectType t) const
        {
            for (const auto &o : objects)
                if (o.type == t)
                    return o;
            return std::nullopt;
        }

        std::vector<MapObject> objectsOf(MapObjectType t) const
        {
            std::vector<MapObject> r;
            for (const auto &o : objects)
                if (o.type == t)
                    r.push_back(o);
            return r;
        }

        std::optional<MapObject> playerStart() const { return findObject(MapObjectType::Player); }
        std::vector<MapObject> girls() const { return objectsOf(MapObjectType::Girl); }
    };

    // ---------------------------------------------------------------------------
    // Тайлы <-> пиксели (мировые координаты)
    // ---------------------------------------------------------------------------
    inline constexpr int tileToPixelX(int tx) noexcept { return tx * kTileWidth; }
    inline constexpr int tileToPixelY(int ty) noexcept { return ty * kTileHeight; }
    inline constexpr int pixelToTileX(int px) noexcept { return px / kTileWidth; }
    inline constexpr int pixelToTileY(int py) noexcept { return py / kTileHeight; }

} // namespace vovochka