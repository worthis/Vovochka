#pragma once

#include "raylib.h"
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace vovochka
{
    // Удаление пробелов, символов табуляции и переноса строки в начале и в конце строки
    std::string trim(const std::string &s);

    // Преобразование строки в нижний регистр
    std::string toLower(std::string s);

    // Регистронезависимый поиск подпапки по частям пути
    std::filesystem::path resolveDirCI(const std::filesystem::path &base,
                                       std::initializer_list<const char *> parts);

    // Нормализация пути (замена backslash на slash)
    std::string normalizePath(const std::filesystem::path &p);

    // Построение индекса файлов папки: lower-case имя -> реальный путь
    std::unordered_map<std::string, std::filesystem::path> buildFileIndex(const std::filesystem::path &dir);

    // Применение chroma-key к изображению (замена близких к key пикселей на прозрачные)
    void applyChromaKey(Image &img, Color key, int tolerance = 8);

    // Распаковка RGB-цвета (0xRRGGBB) в Color
    Color unpackColor(uint32_t rgb);

} // namespace vovochka