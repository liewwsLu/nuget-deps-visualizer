#pragma once

#include <map>
#include <string>
#include <vector>

/// Источник данных о пакетах: репозиторий NuGet или тестовый файл.
class DependencySource {
public:
    virtual ~DependencySource() = default;

    /// Прямые зависимости пакета (имена других пакетов).
    virtual std::vector<std::string> dependencies(const std::string& name) = 0;

    /// Подпись пакета для вывода (имя и версия).
    virtual std::string label(const std::string& name) = 0;
};

/// Граф зависимостей.
struct Graph {
    std::string root;
    std::vector<std::string> nodes;                         ///< пакеты в порядке обнаружения
    std::map<std::string, std::vector<std::string>> edges;  ///< прямые зависимости каждого пакета
    std::map<std::string, std::string> labels;              ///< подпись каждого пакета
    std::vector<std::vector<std::string>> cycles;           ///< найденные циклы: A -> B -> A

    /// Прямые зависимости пакета (пустой список, если пакета нет в графе).
    const std::vector<std::string>& children(const std::string& name) const;
};

/// Построить граф всех зависимостей пакета root обходом в глубину без рекурсии.
///
/// Вместо рекурсии используется явный стек пар (пакет, номер следующей
/// зависимости). Пакет в состоянии «в обработке» лежит на стеке; если
/// встречаем его снова — найден цикл, и дальше по этой связи не идём.
Graph buildGraph(const std::string& root, DependencySource& source);
