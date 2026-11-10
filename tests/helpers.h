#pragma once

// Вспомогательные функции для тестов: запуск приложения и временные файлы.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "app.h"

/// Результат запуска приложения: код завершения, вывод и ошибки.
struct RunResult {
    int code;
    std::string out;
    std::string err;
};

/// Запустить приложение с параметрами args.
inline RunResult runWith(const std::vector<std::string>& args) {
    std::ostringstream out;
    std::ostringstream err;
    int code = runApp(args, out, err);
    return {code, out.str(), err.str()};
}

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

/// Путь к файлу из tests/data (тесты запускаются из корня проекта).
inline std::string dataPath(const std::string& name) {
    return "tests/data/" + name;
}
