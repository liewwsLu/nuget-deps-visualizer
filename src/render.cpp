#include "render.h"

#include <algorithm>

namespace {

const std::string BRANCH = "+-- ";
const std::string LAST_BRANCH = "\\-- ";
const std::string INDENT = "|   ";
const std::string LAST_INDENT = "    ";

/// Записать строку в кавычках по правилам DOT.
std::string quote(const std::string& text) {
    std::string result = "\"";
    for (char c : text) {
        if (c == '\\' || c == '"') {
            result += '\\';
            result += c;
        } else if (c == '\n') {
            result += "\\n";
        } else {
            result += c;
        }
    }
    return result + "\"";
}

/// Подпись пакета; первый пробел (между именем и версией) — перевод строки.
std::string twoLineLabel(const Graph& graph, const std::string& name) {
    auto found = graph.labels.find(name);
    std::string label = found == graph.labels.end() ? name : found->second;
    std::size_t space = label.find(' ');
    if (space != std::string::npos) {
        label[space] = '\n';
    }
    return label;
}

std::string labelOf(const Graph& graph, const std::string& name) {
    auto found = graph.labels.find(name);
    return found == graph.labels.end() ? name : found->second;
}

/// Элемент стека при построении дерева.
struct TreeItem {
    std::string name;
    std::string prefix;              ///< отступ перед веткой
    bool last;                       ///< последняя зависимость у родителя
    std::vector<std::string> path;   ///< пакеты от корня до родителя
};

/// Положить на стек зависимости пакета (в обратном порядке, чтобы первая вышла первой).
void pushChildren(const Graph& graph, const std::string& name, const std::string& prefix,
                  const std::vector<std::string>& parentPath, std::vector<TreeItem>& stack) {
    const std::vector<std::string>& children = graph.children(name);
    std::vector<std::string> path = parentPath;
    path.push_back(name);
    for (std::size_t i = children.size(); i-- > 0;) {
        stack.push_back({children[i], prefix, i + 1 == children.size(), path});
    }
}

}  // namespace

std::set<std::pair<std::string, std::string>> cycleEdges(const Graph& graph) {
    std::set<std::pair<std::string, std::string>> result;
    for (const std::vector<std::string>& cycle : graph.cycles) {
        result.insert({cycle[cycle.size() - 2], cycle.back()});
    }
    return result;
}

std::string toDot(const Graph& graph) {
    std::string text = "digraph dependencies {\n";
    text += "    rankdir=TB;\n";
    text += "    node [shape=box, style=\"rounded,filled\", fillcolor=\"" + NODE_COLOR +
            "\", fontname=\"Helvetica\"];\n";
    for (const std::string& name : graph.nodes) {
        std::string extra;
        if (name == graph.root) {
            extra = ", fillcolor=\"" + ROOT_COLOR + "\", penwidth=2";
        }
        text += "    " + quote(name) + " [label=" + quote(twoLineLabel(graph, name)) + extra + "];\n";
    }
    auto closing = cycleEdges(graph);
    for (const std::string& name : graph.nodes) {
        for (const std::string& child : graph.children(name)) {
            std::string style;
            if (closing.count({name, child}) != 0) {
                style = " [color=\"" + CYCLE_COLOR + "\", style=dashed]";
            }
            text += "    " + quote(name) + " -> " + quote(child) + style + ";\n";
        }
    }
    return text + "}\n";
}

std::vector<std::string> asciiTree(const Graph& graph) {
    std::vector<std::string> lines = {labelOf(graph, graph.root)};
    std::set<std::string> expanded = {graph.root};
    std::vector<TreeItem> stack;
    pushChildren(graph, graph.root, "", {}, stack);
    while (!stack.empty()) {
        TreeItem item = stack.back();
        stack.pop_back();
        std::string mark;
        if (std::find(item.path.begin(), item.path.end(), item.name) != item.path.end()) {
            mark = " (cycle)";
        } else if (expanded.count(item.name) != 0 && !graph.children(item.name).empty()) {
            mark = " (see above)";
        }
        lines.push_back(item.prefix + (item.last ? LAST_BRANCH : BRANCH) + labelOf(graph, item.name) + mark);
        if (!mark.empty() || expanded.count(item.name) != 0) {
            continue;
        }
        expanded.insert(item.name);
        pushChildren(graph, item.name, item.prefix + (item.last ? LAST_INDENT : INDENT), item.path, stack);
    }
    return lines;
}
