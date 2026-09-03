#pragma once

#include "MapData.h"
#include "SpriteSheet.h"
#include <memory>
#include "raylib.h"

namespace vovochka
{

    class LevelLoader;

    class LevelRenderer
    {
    public:
        LevelRenderer();
        ~LevelRenderer();

        bool loadLevelAssets(const LevelLoader &loader, int levelNum);
        void unload();

        void draw(const LevelMap &map, bool debugObjects = true) const;

        const SpriteSheetGPU *sheet(const std::string &name) const;

        int tileW() const { return m_tileW; }
        int tileH() const { return m_tileH; }

    private:
        Texture2D m_bg{};
        std::unique_ptr<SpriteSheetManager> m_sheets;

        int m_tileW = 80;
        int m_tileH = 80;

        void drawTile(const MapTile &t, float px, float py) const;
    };

} // namespace vovochka