#pragma once

#include <stdexcept>
#include <string>
#include <vector>

/// Ошибка в параметрах командной строки.
struct ConfigError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Репозиторий NuGet по умолчанию (индекс сервиса NuGet V3).
inline const std::string DEFAULT_REPOSITORY = "https://api.nuget.org/v3/index.json";
/// Имя файла с изображением графа по умолчанию.
inline const std::string DEFAULT_OUTPUT = "graph.svg";

/// Настройки, заданные пользователем.
struct Config {
    std::string package;                       ///< имя анализируемого пакета
    std::string repository = DEFAULT_REPOSITORY;  ///< URL репозитория или путь к тестовому файлу
    bool testMode = false;                     ///< режим тестового репозитория
    std::string output = DEFAULT_OUTPUT;       ///< имя файла с изображением графа
    bool asciiTree = false;                    ///< вывод ASCII-дерева
};

/// Разобрать и проверить параметры командной строки (без имени программы).
/// При любой ошибке бросает ConfigError.
Config parseConfig(const std::vector<std::string>& args);

/// Параметры в виде строк «ключ=значение».
std::vector<std::string> formatConfig(const Config& config);

/// Подсказка по запуску программы.
std::string usage();
