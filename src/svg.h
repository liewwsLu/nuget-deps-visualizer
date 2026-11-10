#pragma once

// Сохранение изображения графа в файл SVG.
//
// Основной способ — программа dot из пакета Graphviz: она получает описание
// графа на языке DOT и рисует его. Если Graphviz не установлен, используется
// простой встроенный рисовальщик: пакеты располагаются по уровням (уровень —
// расстояние от анализируемого пакета).

#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "graph.h"

/// Не удалось нарисовать или сохранить граф.
struct RenderError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

const std::string GRAPHVIZ = "graphviz dot";
const std::string BUILT_IN = "built-in renderer";

/// Уровень каждого пакета — длина кратчайшего пути от корня (обход в ширину).
/// Пакеты перечислены в порядке обхода.
std::vector<std::pair<std::string, int>> levels(const Graph& graph);

/// Нарисовать граф встроенным рисовальщиком и вернуть текст SVG.
std::string builtInSvg(const Graph& graph);

/// Полный путь к программе в папках из переменной PATH или пустая строка.
std::string findProgram(const std::string& name);

/// Сохранить изображение графа в svgPath; вернуть, чем оно нарисовано.
/// dotPath — файл с описанием графа на языке DOT. dotCommand — путь к
/// программе dot; если пустой, используется встроенный рисовальщик.
std::string saveSvg(const Graph& graph, const std::string& dotPath, const std::string& svgPath,
                    const std::string& dotCommand);
