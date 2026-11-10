#pragma once

// Вспомогательные функции для тестов: запуск приложения и временные файлы.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "app.h"
#include "fake_nuget.h"

/// Результат запуска приложения: код завершения, вывод и ошибки.
struct RunResult {
    int code;
    std::string out;
    std::string err;
};

/// Временная папка для файлов теста; удаляется вместе с объектом.
struct TempDir {
    std::filesystem::path path;

    TempDir() {
        static int counter = 0;
        path = std::filesystem::temp_directory_path() /
               ("depviz_test_" + std::to_string(++counter) + "_" +
                std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::filesystem::create_directories(path);
    }

    ~TempDir() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    /// Создать файл с текстом и вернуть путь к нему.
    std::string write(const std::string& name, const std::string& text) const {
        std::filesystem::path file = path / name;
        std::ofstream(file) << text;
        return file.string();
    }
};

/// Запустить приложение с параметрами args; сеть подменяется объектом fetcher.
/// Если --output не задан, изображение сохраняется во временную папку.
inline RunResult runWith(std::vector<std::string> args, Fetcher& fetcher) {
    TempDir folder;
    bool hasOutput = false;
    for (const std::string& arg : args) {
        hasOutput = hasOutput || arg.rfind("--output", 0) == 0;
    }
    if (!hasOutput) {
        args.push_back("--output");
        args.push_back((folder.path / "g.svg").string());
    }
    std::ostringstream out;
    std::ostringstream err;
    int code = runApp(args, fetcher, out, err);
    return {code, out.str(), err.str()};
}

/// Запустить приложение с пустым сервером (на любой запрос — 404).
inline RunResult runWith(const std::vector<std::string>& args) {
    FakeFetcher fetcher;
    return runWith(args, fetcher);
}

/// Путь к файлу из tests/data (тесты запускаются из корня проекта).
inline std::string dataPath(const std::string& name) {
    return "tests/data/" + name;
}
