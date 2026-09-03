#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace vovochka
{

    // ����������� self-contained INI-������.
    // ������������:
    //   - ������ [Section]
    //   - ����� Key=Value (� '=' ��� ������������)
    //   - ����������� ';' � '#' � ������ ������
    //   - trim �������� ������ ������ � ��������
    //   - ������������������ �������� ��� ������� �������� � ��������� ������ cl*
    class IniReader
    {
    public:
        bool loadFile(const std::string &path);
        bool loadString(const std::string &text);

        std::optional<std::string> get(const std::string &section,
                                       const std::string &key) const;

        int getInt(const std::string &s, const std::string &k, int def) const;
        bool getBool(const std::string &s, const std::string &k, bool def) const;
        uint32_t getColor(const std::string &s, const std::string &k, uint32_t def) const;
        float getFloat(const std::string &s, const std::string &k, float def) const;

        std::vector<std::string> sections() const;
        bool hasSection(const std::string &s) const;
        bool hasKey(const std::string &s, const std::string &k) const;

    private:
        using Section = std::unordered_map<std::string, std::string>;
        std::unordered_map<std::string, Section> m_data;
    };

} // namespace vovochka