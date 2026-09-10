#include "Utils.h"
#include <algorithm>
#include <cctype>
#include "raylib.h"

namespace vovochka
{
    std::string trim(const std::string &s)
    {
        size_t b = s.find_first_not_of(" \t\r\n");
        size_t e = s.find_last_not_of(" \t\r\n");
        return (b == std::string::npos) ? std::string() : s.substr(b, e - b + 1);
    }

    std::string toLower(std::string s)
    {
        for (auto &c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    std::filesystem::path resolveDirCI(const std::filesystem::path &base,
                                       std::initializer_list<const char *> parts)
    {
        namespace fs = std::filesystem;
        fs::path cur = base;
        for (const char *part : parts)
        {
            bool found = false;
            std::error_code ec;
            for (auto &e : fs::directory_iterator(cur, ec))
            {
                if (ec)
                    break;
                if (e.is_directory() && toLower(e.path().filename().string()) == toLower(part))
                {
                    cur = e.path();
                    found = true;
                    break;
                }
            }
            if (!found)
                return {};
        }
        return cur;
    }

    std::string normalizePath(const std::filesystem::path &p)
    {
        std::string s = p.string();
        std::replace(s.begin(), s.end(), '\\', '/');
        return s;
    }

    std::unordered_map<std::string, std::filesystem::path> buildFileIndex(const std::filesystem::path &dir)
    {
        namespace fs = std::filesystem;
        std::unordered_map<std::string, fs::path> idx;
        std::error_code ec;
        if (!fs::is_directory(dir, ec))
            return idx;

        for (const auto &e : fs::directory_iterator(dir, ec))
        {
            if (ec)
                break;
            std::error_code ec2;
            if (!e.is_regular_file(ec2))
                continue;
            idx.emplace(toLower(e.path().filename().string()), e.path());
        }
        return idx;
    }

    void applyChromaKey(Image &img, Color key, int tolerance)
    {
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        auto *p = static_cast<unsigned char *>(img.data);
        const int n = img.width * img.height;

        for (int i = 0; i < n; ++i)
        {
            unsigned char *c = p + i * 4;
            const int dr = (int)c[0] - (int)key.r;
            const int dg = (int)c[1] - (int)key.g;
            const int db = (int)c[2] - (int)key.b;

            if (std::abs(dr) <= tolerance &&
                std::abs(dg) <= tolerance &&
                std::abs(db) <= tolerance)
            {
                c[3] = 0;
            }
        }
    }

    Color unpackColor(uint32_t rgb)
    {
        return Color{static_cast<unsigned char>((rgb >> 16) & 0xFF),
                     static_cast<unsigned char>((rgb >> 8) & 0xFF),
                     static_cast<unsigned char>(rgb & 0xFF),
                     255};
    }

} // namespace vovochka