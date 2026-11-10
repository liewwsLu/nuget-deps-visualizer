#pragma once

#include <map>
#include <string>
#include <vector>

#include "graph.h"
#include "http.h"
#include "nuspec.h"

/// Прямые зависимости пакета: выбранная версия, платформа и список зависимостей.
struct DirectDependencies {
    std::string version;
    std::string framework;
    std::vector<Dependency> dependencies;
};

/// Репозиторий NuGet, заданный URL (протокол NuGet V3).
///
/// URL может указывать на индекс сервиса (.../v3/index.json) или прямо
/// на хранилище пакетов (.../v3-flatcontainer/).
class NuGetRepository {
public:
    NuGetRepository(std::string url, Fetcher& fetcher);

    /// Адрес хранилища пакетов (ресурс PackageBaseAddress/3.0.0), всегда с «/» в конце.
    std::string baseAddress();

    /// Список всех версий пакета.
    std::vector<std::string> versions(const std::string& packageId);

    /// Текст файла .nuspec заданной версии пакета.
    std::string nuspec(const std::string& packageId, const std::string& version);

    /// Прямые зависимости пакета. Если версия не задана — берётся последняя стабильная.
    DirectDependencies directDependencies(const std::string& packageId, const std::string& version = "");

private:
    std::string url_;
    Fetcher& fetcher_;
    std::string base_;

    /// Скачать текст по URL. Если сервер ответил 404 — found = false и пустая строка.
    std::string fetchText(const std::string& url, bool& found);
};

/// Источник данных для buildGraph на основе репозитория NuGet.
///
/// Версия корневого пакета — последняя стабильная. Версия зависимости —
/// наименьшая, подходящая под диапазон из .nuspec (правило NuGet «lowest
/// applicable version»). Если пакет встречается несколько раз, используется
/// диапазон, встреченный первым. Имена пакетов NuGet не зависят от регистра,
/// поэтому сравниваются маленькими буквами, а выводятся так, как встретились первыми.
class NuGetSource : public DependencySource {
public:
    NuGetSource(NuGetRepository& repository, const std::string& root);

    std::vector<std::string> dependencies(const std::string& name) override;

    /// Подпись пакета: имя и выбранная версия.
    std::string label(const std::string& name) override;

    /// Задать версию пакета заранее, без выбора по правилам (используется в тестах).
    void useVersion(const std::string& name, const std::string& version);

private:
    NuGetRepository& repository_;
    std::map<std::string, std::string> names_;     ///< имя маленькими буквами -> имя для вывода
    std::map<std::string, std::string> ranges_;    ///< имя маленькими буквами -> диапазон версий
    std::map<std::string, std::string> versions_;  ///< имя маленькими буквами -> выбранная версия

    /// Определить версию пакета.
    std::string resolve(const std::string& name);
};
