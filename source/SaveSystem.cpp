#include "SaveSystem.h"
#include <algorithm>
#include <fstream>
#include "third_party/json.hpp"
#include "raylib.h"

using json = nlohmann::json;

namespace vovochka
{
    SaveSystem &SaveSystem::instance()
    {
        static SaveSystem s;
        return s;
    }

    bool SaveSystem::load(const std::string &path)
    {
        m_path = path;

        std::ifstream f(path);
        if (!f.is_open())
            return false;

        json j;
        f >> j;

        if (j.contains("levelScores") && j["levelScores"].is_array())
        {
            const auto &arr = j["levelScores"];
            for (int i = 0; i < 12 && i < (int)arr.size(); ++i)
                if (arr[i].is_number_integer())
                    m_levelScores[i] = std::clamp(arr[i].get<int>(), 0, 100);
        }

        TraceLog(LOG_INFO, "Save loaded from %s", path.c_str());
        return true;
    }

    void SaveSystem::save() const
    {
        if (m_path.empty())
            return;

        json j;
        json arr = json::array();
        for (int i = 0; i < 12; ++i)
            arr.push_back(m_levelScores[i]);
        j["levelScores"] = arr;

        std::ofstream f(m_path);
        if (f.is_open())
        {
            f << j.dump(4);
            TraceLog(LOG_INFO, "Save written to %s", m_path.c_str());
        }
        else
        {
            TraceLog(LOG_WARNING, "Failed to write save: %s", m_path.c_str());
        }
    }

    int SaveSystem::levelScore(int lvl) const
    {
        return (lvl >= 1 && lvl <= 12) ? m_levelScores[lvl - 1] : 0;
    }

    void SaveSystem::submitLevelScore(int lvl, int score)
    {
        if (lvl < 1 || lvl > 12)
            return;

        const int sc = std::clamp(score, 0, 100);
        if (sc > m_levelScores[lvl - 1]) // сохраняем только улучшение
        {
            m_levelScores[lvl - 1] = sc;
            save(); // сразу в файл
        }
    }
}