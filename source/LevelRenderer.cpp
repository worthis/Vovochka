#include "LevelRenderer.h"
#include "LevelLoader.h"
#include <string>
#include <memory>
#include "raylib.h"

namespace vovochka
{

    LevelRenderer::LevelRenderer() = default;
    LevelRenderer::~LevelRenderer() { unload(); }

    bool LevelRenderer::loadLevelAssets(const LevelLoader &loader, int n)
    {
        unload();

        m_sheets = std::make_unique<SpriteSheetManager>();
        if (!loader.loadLevelAssets(n, *m_sheets))
        {
            return false;
        }

        const SpriteSheetGPU *bg = m_sheets->get("bg1");
        if (bg)
        {
            m_bg = bg->texture;
        }

        // Размеры тайла берём из набора геометрии [Set2]
        // PatternWidth/PatternHeight в GameSprites.dat
        if (const SpriteSheetGPU *s = m_sheets->get("Set2"))
        {
            if (s->patternW > 0)
                m_tileW = s->patternW;
            if (s->patternH > 0)
                m_tileH = s->patternH;
        }

        return true;
    }

    void LevelRenderer::unload()
    {
        if (m_sheets)
        {
            m_sheets->unload();
            m_sheets.reset();
        }
        m_bg = {};
    }

    void LevelRenderer::drawTile(int set, int index, float px, float py) const
    {
        if (index == 9 || index == 0xFF)
            return;

        std::string sheetName = "Set" + std::to_string(set);
        const SpriteSheetGPU *sheet = m_sheets->get(sheetName);
        if (!sheet)
            return;

        Rectangle src = sheet->frame(index);
        Rectangle dst{px, py, static_cast<float>(sheet->patternW), static_cast<float>(sheet->patternH)};
        DrawTexturePro(sheet->texture, src, dst, {0, 0}, 0.0f, WHITE);
    }

    void LevelRenderer::drawTile(const MapTile &t, float px, float py) const
    {
        drawTile(t.set, t.index, px, py);
    }

    void LevelRenderer::drawFrontLayer(const LevelMap &map) const
    {
        const float tw = static_cast<float>(m_tileW);
        const float th = static_cast<float>(m_tileH);

        // Поверх каждого стыка «лестница×земля» (LadderBase) рисуем
        // тайл земли того же набора — игрок и враги проходят «сквозь землю».
        for (int x = 0; x < map.width; ++x)
            for (int y = 0; y < map.height; ++y)
            {
                if (map.kindAt(x, y) != TileKind::LadderBase)
                    continue;

                const MapTile &t = map.tileAt(x, y);
                drawTile(t.set, 0 /* Platform */, x * tw, y * th);
            }
    }

    void LevelRenderer::drawMap(const LevelMap &map) const
    {
        const float tw = static_cast<float>(m_tileW);
        const float th = static_cast<float>(m_tileH);

        for (int x = 0; x < map.width; ++x)
            for (int y = 0; y < map.height; ++y)
                drawTile(map.tileAt(x, y), x * tw, y * th);
    }

    void LevelRenderer::draw(const LevelMap &map, bool debugObjects) const
    {
        const float tw = static_cast<float>(m_tileW);
        const float th = static_cast<float>(m_tileH);

        if (m_bg.id != 0)
        {
            DrawTexturePro(m_bg,
                           {0, 0, (float)m_bg.width, (float)m_bg.height},
                           {0, 0, map.width * tw, map.height * th}, {0, 0}, 0, WHITE);
        }

        drawMap(map);

        if (!debugObjects)
            return;

        for (const MapObject &o : map.objects)
        {
            Color c = WHITE;
            if (o.type == MapObjectType::Player)
                c = GREEN;
            else if (o.type == MapObjectType::Girl)
                c = RED;
            DrawRectangleLines(o.x * m_tileW + 2, o.y * m_tileH + 2,
                               m_tileW - 4, m_tileH - 4, c);
        }
    }

    const SpriteSheetGPU *LevelRenderer::sheet(const std::string &name) const
    {
        return m_sheets ? m_sheets->get(name) : nullptr;
    }

} // namespace vovochka