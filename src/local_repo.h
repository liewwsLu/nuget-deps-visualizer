#pragma once

// Тестовый репозиторий: граф пакетов, описанный в текстовом файле.
//
// Формат файла — по строке на пакет:
//
//     A: B C      пакет A зависит от B и C
//     B: D
//     D:          у D нет зависимостей
//     # комментарий
//
// Имена пакетов — большие латинские буквы. Пакет, который упомянут как
// зависимость, но не описан отдельной строкой, считается пакетом без зависимостей.

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "graph.h"

/// Ошибка в файле тестового репозитория.
struct LocalRepositoryError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Пакет -> список его прямых зависимостей.
using PackageMap = std::map<std::string, std::vector<std::string>>;

/// Разобрать текст тестового репозитория.
PackageMap parseLocalRepository(const std::string& text);

/// Прочитать и разобрать файл тестового репозитория.
PackageMap loadLocalRepository(const std::string& path);

/// Источник данных для buildGraph на основе тестового репозитория.
class LocalRepository : public DependencySource {
public:
    /// Корневой пакет должен быть описан в репозитории, иначе LocalRepositoryError.
    LocalRepository(PackageMap packages, const std::string& root);

    std::vector<std::string> dependencies(const std::string& name) override;

    /// У тестовых пакетов нет версий: подпись — просто имя.
    std::string label(const std::string& name) override;

private:
    PackageMap packages_;
};
