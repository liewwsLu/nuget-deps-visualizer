#pragma once

#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

/// Неверная запись диапазона версий.
struct VersionError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Ключ для сравнения версий: числа (4 части), признак релиза, метка пререлиза.
/// Сравнение ключей даёт порядок 1.2 < 1.10 < 2.0-beta < 2.0.
using VersionKey = std::tuple<std::vector<long long>, bool, std::string>;

/// Ключ сравнения для записи версии.
VersionKey versionKey(const std::string& version);

/// Пререлиз — версия с меткой после дефиса (6.0.1-dev-00953).
bool isPrerelease(const std::string& version);

/// Самая новая стабильная версия; если стабильных нет — самая новая вообще.
/// Пустой список — std::invalid_argument.
std::string latestStable(const std::vector<std::string>& versions);

/// Диапазон версий в понятном виде: «4.0.0» -> «>= 4.0.0».
std::string describeRange(const std::string& range);

/// Диапазон версий NuGet: «1.0» (1.0 или новее), «[1.0, 2.0)», «(1.0,)», «[1.0]», «(, 2.0]».
/// Пустая граница означает «граница не задана».
struct VersionRange {
    std::string low;
    bool lowInclusive = true;
    std::string high;
    bool highInclusive = true;

    /// Проверить, что версия попадает в диапазон.
    bool contains(const std::string& version) const;
};

/// Разобрать запись диапазона из .nuspec. Незакрытая скобка — VersionError.
VersionRange parseRange(const std::string& text);

/// Наименьшая подходящая версия — так выбирает версию NuGet («lowest applicable version»).
/// Пререлизы учитываются, только если нижняя граница — пререлиз.
/// Если подходящей версии нет — пустая строка.
std::string lowestMatching(const std::string& rangeText, const std::vector<std::string>& versions);
