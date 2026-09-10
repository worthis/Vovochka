#pragma once
#include "raylib.h"
#include <string>
#include <vector>

namespace vovochka
{

    // Параметры шрифта из <font>.dat (ini-формат)
    struct FontDef
    {
        int pictureW = 0, pictureH = 0;
        int patternW = 0, patternH = 0;
        int skipW = 0, skipH = 0;
        bool transparent = false;
        Color transparentColor{0, 0, 0, 255};
    };

    class BitmapFont
    {
    public:
        bool load(const std::string &basePath); // ".../Font1" без расширения
        void unload();
        bool valid() const { return tex.id != 0 && !widths.empty(); }

        void draw(const std::string &utf8Text, float x, float y, Color tint = WHITE, float scale = 1.0f) const;
        int textWidth(const std::string &utf8Text, float scale = 1.0f) const;
        int lineHeight() const { return cellH; }

    private:
        Texture2D tex{};
        FontDef def{};
        int cellW = 0, cellH = 0;
        int cols = 32;
        int firstChar = 32;
        float spacing = 0.0f;
        std::vector<int> widths;

        bool glyphFor(unsigned char cp, int &sx, int &sy, int &sw) const;
        static std::vector<unsigned char> utf8ToCp1251(const std::string &s);

        // Парсинг <font>.dat
        static bool parseDat(const std::string &path, const std::string &section, FontDef &out);
        static Color parseDelphiColor(const std::string &s);
    };

} // namespace vovochka