#pragma once

#include <string>
#include <tuple>
#include <vector>

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
