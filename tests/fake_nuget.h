#pragma once

// Подмена сети для тестов: ответы NuGet берутся из файлов tests/data/nuget.

#include <fstream>
#include <map>
#include <sstream>
#include <string>
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
