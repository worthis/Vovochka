#include "IniReader.h"
#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <cstdint>

namespace vovochka
{

    bool IniReader::loadFile(const std::string &path)
    {
        std::ifstream f(path);
        if (!f)
            return false;
        std::stringstream ss;
        ss << f.rdbuf();
        return loadString(ss.str());
    }

    bool IniReader::loadString(const std::string &text)
    {
        m_data.clear();
        std::string cur;
        std::istringstream in(text);
        std::string line;
        while (std::getline(in, line))
        {
            line = trim(line);
            if (line.empty())
                continue;

            // BOM-strip � �����������
            if (line[0] == ';' || line[0] == '#')
                continue;

            if (line.front() == '[' && line.back() == ']')
            {
                cur = trim(line.substr(1, line.size() - 2));
                m_data.try_emplace(cur);
                continue;
            }

            auto eq = line.find('=');
            if (eq == std::string::npos)
                continue;
            std::string k = trim(line.substr(0, eq));
            std::string v = trim(line.substr(eq + 1));
            auto sc = v.find(" ;");
            if (sc != std::string::npos)
                v = trim(v.substr(0, sc));

            if (!k.empty())
                m_data[cur][k] = v;
        }
        return true;
    }

    std::optional<std::string> IniReader::get(const std::string &s, const std::string &k) const
    {
        auto it = m_data.find(s);
        if (it == m_data.end())
            return std::nullopt;
        auto it2 = it->second.find(k);
        if (it2 == it->second.end())
            return std::nullopt;
        return it2->second;
    }

    int IniReader::getInt(const std::string &s, const std::string &k, int def) const
    {
        auto v = get(s, k);
        if (!v)
            return def;
        try
        {
            std::string lv = toLower(*v);
            if (!lv.empty() && lv[0] == '$')
                return static_cast<int>(std::stol(lv.substr(1), nullptr, 16));
            if (lv.size() > 2 && lv[0] == '0' && lv[1] == 'x')
                return static_cast<int>(std::stol(lv.substr(2), nullptr, 16));
            return std::stoi(*v);
        }
        catch (...)
        {
            return def;
        }
    }

    bool IniReader::getBool(const std::string &s, const std::string &k, bool def) const
    {
        auto v = get(s, k);
        if (!v)
            return def;
        std::string lv = toLower(*v);
        if (lv == "1" || lv == "true" || lv == "yes" || lv == "on")
            return true;
        if (lv == "0" || lv == "false" || lv == "no" || lv == "off")
            return false;
        return def;
    }

    uint32_t IniReader::getColor(const std::string &s, const std::string &k, uint32_t def) const
    {
        auto v = get(s, k);
        if (!v)
            return def;
        std::string lv = toLower(*v);
        if (lv == "cllime")
            return 0x00FF00u;
        if (lv == "clfuchsia")
            return 0xFF00FFu;
        if (lv == "clmagenta")
            return 0xFF00FFu;
        if (lv == "clblack")
            return 0x000000u;
        if (lv == "clwhite")
            return 0xFFFFFFu;
        if (lv == "clred")
            return 0xFF0000u;
        if (lv == "clgreen")
            return 0x00FF00u;
        if (lv == "clblue")
            return 0x0000FFu;
        if (lv == "clyellow")
            return 0xFFFF00u;
        if (lv == "clcyan")
            return 0x00FFFFu;
        if (lv == "clgray")
            return 0x808080u;
        if (lv == "clsilver")
            return 0xC0C0C0u;
        // hex: $RRGGBB
        if (!lv.empty() && lv[0] == '$')
        {
            try
            {
                return static_cast<uint32_t>(std::stoul(lv.substr(1), nullptr, 16));
            }
            catch (...)
            {
                return def;
            }
        }
        return def;
    }

    float IniReader::getFloat(const std::string &s, const std::string &k, float def) const
    {
        auto v = get(s, k);
        if (!v)
            return def;
        try
        {
            return std::stof(*v);
        }
        catch (...)
        {
            return def;
        }
    }

    std::vector<std::string> IniReader::sections() const
    {
        std::vector<std::string> r;
        r.reserve(m_data.size());
        for (const auto &[k, _] : m_data)
            r.push_back(k);
        std::sort(r.begin(), r.end());
        return r;
    }

    bool IniReader::hasSection(const std::string &s) const
    {
        return m_data.find(s) != m_data.end();
    }

    bool IniReader::hasKey(const std::string &s, const std::string &k) const
    {
        auto it = m_data.find(s);
        return it != m_data.end() && it->second.find(k) != it->second.end();
    }

} // namespace vovochka