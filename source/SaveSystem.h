#pragma once

#include <string>

namespace vovochka
{
    class SaveSystem
    {
    public:
        static SaveSystem &instance();

        bool load(const std::string &path);
        void save() const;

        int levelScore(int lvl) const;             // 1..12, 0 если уровень не пройден
        void submitLevelScore(int lvl, int score); // пишем, если больше прежнего, и сразу в файл

    private:
        SaveSystem() = default;
        int m_levelScores[12] = {0};
        std::string m_path;
    };
}