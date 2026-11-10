#pragma once

// Подмена сети для тестов: ответы NuGet берутся из файлов tests/data/nuget.

#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "http.h"

const std::string INDEX_URL = "https://api.nuget.org/v3/index.json";
const std::string BASE = "https://api.nuget.org/v3-flatcontainer/";

/// Прочитать файл из tests/data/nuget.
inline std::string readNugetData(const std::string& name) {
    std::ifstream file("tests/data/nuget/" + name, std::ios::binary);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

/// Сервер с заранее заданными ответами: известный URL — 200, остальные — 404.
class FakeFetcher : public Fetcher {
public:
    std::map<std::string, HttpResponse> responses;
    std::vector<std::string> requested;
    bool offline = false;  ///< true — «нет связи с сервером»

    /// Добавить ответ 200 с текстом body.
    void add(const std::string& url, const std::string& body) {
        responses[url] = {200, body};
    }

    HttpResponse get(const std::string& url) override {
        requested.push_back(url);
        if (offline) {
            throw RepositoryError("cannot connect to " + url);
        }
        auto found = responses.find(url);
        if (found == responses.end()) {
            return {404, "Not Found"};
        }
        return found->second;
    }
};

/// Сервер с пакетом Serilog.Sinks.Console (индекс, список версий, .nuspec 6.0.0).
inline FakeFetcher serilogConsoleFetcher() {
    const std::string name = "serilog.sinks.console";
    FakeFetcher fetcher;
    fetcher.add(INDEX_URL, readNugetData("service_index.json"));
    fetcher.add(BASE + name + "/index.json", readNugetData(name + ".index.json"));
    fetcher.add(BASE + name + "/6.0.0/" + name + ".nuspec", readNugetData(name + ".6.0.0.nuspec"));
    return fetcher;
}

/// Пакеты тестового сервера: имя -> версия -> список зависимостей (имя, диапазон).
using FakeServer = std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, std::string>>>>;

inline std::string toLowerCase(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

/// Собрать текст .nuspec с одной группой .NETStandard2.0.
inline std::string makeNuspec(const std::string& id, const std::string& version,
                              const std::vector<std::pair<std::string, std::string>>& dependencies) {
    std::string items;
    for (const auto& [name, range] : dependencies) {
        items += "<dependency id=\"" + name + "\" version=\"" + range + "\" />";
    }
    return "<package xmlns=\"http://schemas.microsoft.com/packaging/2013/05/nuspec.xsd\"><metadata><id>" + id +
           "</id><version>" + version + "</version><dependencies><group targetFramework=\".NETStandard2.0\">" +
           items + "</group></dependencies></metadata></package>";
}

/// Сервер с заданными пакетами: индекс сервиса, списки версий и .nuspec каждой версии.
inline FakeFetcher serverFetcher(const FakeServer& packages) {
    FakeFetcher fetcher;
    fetcher.add(INDEX_URL, readNugetData("service_index.json"));
    for (const auto& [name, versions] : packages) {
        std::string key = toLowerCase(name);
        std::string list;
        for (const auto& entry : versions) {
            list += (list.empty() ? "\"" : ", \"") + entry.first + "\"";
        }
        fetcher.add(BASE + key + "/index.json", "{\"versions\": [" + list + "]}");
        for (const auto& [version, dependencies] : versions) {
            fetcher.add(BASE + key + "/" + version + "/" + key + ".nuspec", makeNuspec(name, version, dependencies));
        }
    }
    return fetcher;
}
