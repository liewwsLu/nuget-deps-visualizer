#pragma once

// Текстовые представления графа: язык Graphviz (DOT) и ASCII-дерево.

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "graph.h"

const std::string ROOT_COLOR = "#cfe0f7";
const std::string NODE_COLOR = "#f2f5fa";
const std::string CYCLE_COLOR = "#d62728";

/// Связи (пакет, зависимость), которые замыкают найденные циклы.
std::set<std::pair<std::string, std::string>> cycleEdges(const Graph& graph);

/// Описание графа на языке диаграмм Graphviz (DOT).
///
/// Узел — пакет (имя и версия на отдельных строках), стрелка — от пакета
/// к его зависимости. Анализируемый пакет выделен цветом, связи,
/// замыкающие цикл, нарисованы красным пунктиром.
std::string toDot(const Graph& graph);

/// Зависимости в виде ASCII-дерева (по строке на элемент), без рекурсии.
///
/// Поддерево пакета, который уже был показан, не повторяется — у него
/// пометка «(see above)». Пакет, замыкающий цикл, помечен «(cycle)».
std::vector<std::string> asciiTree(const Graph& graph);
