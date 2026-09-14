#pragma once

#include <string>

namespace vovochka
{
    class SaveSystem
    {
    public:
        static SaveSystem &instance();

        bool load(const std::string &path);
        void loadVideoDurations(const std::string &dataRoot);
        void save() const;

        int levelScore(int lvl) const;
        int videoAvailSec(int video) const;
        int videoFullSec(int video) const;
        void submitLevelScore(int lvl, int score, int difficulty);

    private:
        SaveSystem() = default;
        int m_levelScores[12] = {0};
        int m_videoAvailSec[12] = {0};
        int m_videoFullSec[12] = {0};
        std::string m_path;
    };
}