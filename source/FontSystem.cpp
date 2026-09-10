#include "FontSystem.h"
#include "Utils.h"
#include <cstdio>
#include <cctype>
#include <fstream>
#include <algorithm>

namespace vovochka
{

    // Delphi-цвета: clXXX или число $00BBGGRR / десятичное
    Color BitmapFont::parseDelphiColor(const std::string &s)
    {
        std::string v = toLower(trim(s));

        if (v.rfind("cl", 0) == 0)
        {
            static const std::pair<const char *, Color> names[] = {
                {"clblack", {0, 0, 0, 255}},
                {"clwhite", {255, 255, 255, 255}},
                {"clred", {255, 0, 0, 255}},
                {"clgreen", {0, 128, 0, 255}},
                {"clblue", {0, 0, 255, 255}},
                {"clyellow", {255, 255, 0, 255}},
                {"claqua", {0, 255, 255, 255}},
                {"clcyan", {0, 255, 255, 255}},
                {"clfuchsia", {255, 0, 255, 255}},
                {"clmagenta", {255, 0, 255, 255}},
                {"clgray", {128, 128, 128, 255}},
                {"clgrey", {128, 128, 128, 255}},
                {"clsilver", {192, 192, 192, 255}},
                {"clmaroon", {128, 0, 0, 255}},
                {"clolive", {128, 128, 0, 255}},
                {"cllime", {0, 255, 0, 255}},
                {"clteal", {0, 128, 128, 255}},
                {"clnavy", {0, 0, 128, 255}},
                {"clpurple", {128, 0, 128, 255}},
            };
            for (const auto &p : names)
                if (v == p.first)
                    return p.second;
            return {0, 0, 0, 255}; // неизвестное имя -> чёрный
        }

        // Числовой TColor: $00BBGGRR (BGR!) или десятичный
        long long n = 0;
        if (!v.empty() && v[0] == '$')
            n = std::stoll(v.substr(1), nullptr, 16);
        else if (!v.empty() && (std::isdigit((unsigned char)v[0]) || v[0] == '-'))
            n = std::stoll(v);
        else
            return {0, 0, 0, 255};

        int r = (int)(n & 0xFF);
        int g = (int)((n >> 8) & 0xFF);
        int b = (int)((n >> 16) & 0xFF);
        return {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
    }

    bool BitmapFont::parseDat(const std::string &path, const std::string &section, FontDef &out)
    {
        std::ifstream f(path);
        if (!f.is_open())
            return false;

        std::string want = toLower(section);
        std::string cur;
        bool inSection = false;
        bool anySection = false;

        std::string line;
        while (std::getline(f, line))
        {
            std::string s = trim(line);
            if (s.empty() || s[0] == ';' || s[0] == '#')
                continue;

            if (s[0] == '[')
            {
                size_t e = s.find(']');
                cur = toLower(trim(s.substr(1, e - 1)));
                anySection = true;
                inSection = (cur == want);
                continue;
            }

            // Если нужной секции нет — читаем первую попавшуюся
            if (!inSection && anySection && !want.empty() && cur != want)
            {
                // уже были в другой секции и нужная не началась — продолжаем искать
            }

            size_t eq = s.find('=');
            if (eq == std::string::npos)
                continue;

            std::string key = toLower(trim(s.substr(0, eq)));
            std::string val = trim(s.substr(eq + 1));

            if (inSection || !anySection || cur == want)
            {
                if (key == "picturewidth")
                    out.pictureW = std::stoi(val);
                else if (key == "pictureheight")
                    out.pictureH = std::stoi(val);
                else if (key == "patternwidth")
                    out.patternW = std::stoi(val);
                else if (key == "patternheight")
                    out.patternH = std::stoi(val);
                else if (key == "skipwidth")
                    out.skipW = std::stoi(val);
                else if (key == "skipheight")
                    out.skipH = std::stoi(val);
                else if (key == "transparent")
                    out.transparent = (std::stoi(val) != 0);
                else if (key == "transparentcolor")
                    out.transparentColor = parseDelphiColor(val);
            }
        }
        return true;
    }

    bool BitmapFont::load(const std::string &basePath)
    {
        // Имя секции = имя файла: ".../Font1" -> "Font1"
        std::string section = basePath;
        size_t slash = section.find_last_of("/\\");
        if (slash != std::string::npos)
            section = section.substr(slash + 1);

        def = FontDef{};
        parseDat(basePath + ".dat", section, def); // не фатально, если нет

        // --- .inf: ширины глифов ---
        widths.clear();
        {
            FILE *f = fopen((basePath + ".inf").c_str(), "rb");
            if (!f)
                return false;
            unsigned char buf[512];
            size_t n = fread(buf, 1, sizeof(buf), f);
            fclose(f);
            for (size_t i = 0; i < n; ++i)
                widths.push_back(buf[i]);
        }
        if (widths.empty() || widths.size() % cols != 0)
            return false;

        // --- .bmp: текстура глифов ---
        Image img = LoadImage((basePath + ".bmp").c_str());
        if (!img.data)
            return false;

        // Прозрачность — ИЗ .dat, без угадываний
        if (def.transparent)
            applyChromaKey(img, def.transparentColor);

        const int rows = (int)(widths.size() / cols);
        cellW = img.width / cols;
        cellH = img.height / rows;

        tex = LoadTextureFromImage(img);
        UnloadImage(img);

        return tex.id != 0;
    }

    void BitmapFont::unload()
    {
        if (tex.id != 0)
            UnloadTexture(tex);
        tex = {};
        widths.clear();
    }

    bool BitmapFont::glyphFor(unsigned char cp, int &sx, int &sy, int &sw) const
    {
        if (cp < firstChar)
            return false;

        int i = cp - firstChar;
        if (i >= (int)widths.size())
            return false;

        int col = i % cols;
        int row = i / cols;
        sx = col * cellW;
        sy = row * cellH;
        sw = widths[i];
        return sw > 0;
    }

    // UTF-8 -> Windows-1251 (нужно для кириллицы: глифы лежат в порядке кодов CP1251)
    std::vector<unsigned char> BitmapFont::utf8ToCp1251(const std::string &s)
    {
        std::vector<unsigned char> out;
        for (size_t i = 0; i < s.size(); ++i)
        {
            unsigned char c = (unsigned char)s[i];
            if (c < 0x80)
            {
                out.push_back(c);
                continue;
            }
            if (i + 1 < s.size())
            {
                unsigned char c2 = (unsigned char)s[i + 1];
                unsigned int cp = ((unsigned int)(c & 0x1F) << 6) | (c2 & 0x3F);
                ++i;

                unsigned char cp1251 = 0;
                if (cp >= 0x0410 && cp <= 0x044F) // А..я
                    cp1251 = (unsigned char)(0xC0 + (cp - 0x0410));
                else if (cp == 0x0401) // Ё
                    cp1251 = 0xA8;
                else if (cp == 0x0451) // ё
                    cp1251 = 0xB8;
                else if (cp == 0x2116) // №
                    cp1251 = 0xB9;

                if (cp1251)
                    out.push_back(cp1251);
            }
        }
        return out;
    }

    int BitmapFont::textWidth(const std::string &utf8Text, float scale) const
    {
        int w = 0;
        for (unsigned char cp : utf8ToCp1251(utf8Text))
        {
            if (cp >= firstChar && cp - firstChar < (int)widths.size())
                w += widths[cp - firstChar];
            w += (int)spacing;
        }
        return (int)(w * scale);
    }

    void BitmapFont::draw(const std::string &utf8Text, float x, float y, Color tint, float scale) const
    {
        if (!valid())
            return;

        float cx = x;
        for (unsigned char cp : utf8ToCp1251(utf8Text))
        {
            int sx, sy, sw;
            if (glyphFor(cp, sx, sy, sw))
            {
                Rectangle src{(float)sx, (float)sy, (float)sw, (float)cellH};
                Rectangle dst{cx, y, (float)sw * scale, (float)cellH * scale};
                DrawTexturePro(tex, src, dst, {0, 0}, 0.0f, tint);
            }
            int adv = (cp >= firstChar && cp - firstChar < (int)widths.size())
                          ? widths[cp - firstChar]
                          : cellW;
            cx += (adv + spacing) * scale;
        }
    }

}