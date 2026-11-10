#pragma once

#include <string>
#include <vector>

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
