#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "render.h"

namespace {

const int CHAR_WIDTH = 7;
const int BOX_PADDING = 20;
const int BOX_HEIGHT = 30;
const int GAP_X = 20;
const int GAP_Y = 60;
const int MARGIN = 20;
const std::string EDGE_COLOR = "#555555";

#ifdef _WIN32
const char PATH_SEPARATOR = ';';
const char* const NULL_DEVICE = "NUL";
#else
const char PATH_SEPARATOR = ':';
const char* const NULL_DEVICE = "/dev/null";
#endif

/// Прямоугольник пакета на рисунке.
struct Box {
    int x;
    int y;
    int width;
};

/// Заменить символы, особые для XML.
std::string escapeXml(const std::string& text) {
    std::string result;
    for (char c : text) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            default: result += c;
        }
    }
    return result;
}

std::string labelOf(const Graph& graph, const std::string& name) {
    auto found = graph.labels.find(name);
    return found == graph.labels.end() ? name : found->second;
}

/// Координаты прямоугольников: пакеты одного уровня — в одном ряду.
std::map<std::string, Box> layout(const Graph& graph) {
    std::map<int, int> rowEnd;
    std::map<std::string, Box> boxes;
    for (const auto& [name, row] : levels(graph)) {
        int x = rowEnd.count(row) != 0 ? rowEnd[row] : MARGIN;
        int width = static_cast<int>(labelOf(graph, name).size()) * CHAR_WIDTH + BOX_PADDING;
        boxes[name] = {x, MARGIN + row * (BOX_HEIGHT + GAP_Y), width};
        rowEnd[row] = x + width + GAP_X;
    }
    return boxes;
}

/// Путь стрелки: вниз — прямая, в своём ряду — дуга снизу, вверх (обратная
/// связь цикла) — дуга справа.
std::string edgePath(const Box& from, const Box& to) {
    int center1 = from.x + from.width / 2;
    int center2 = to.x + to.width / 2;
    auto str = [](int value) { return std::to_string(value); };
    if (to.y > from.y) {
        return "M " + str(center1) + " " + str(from.y + BOX_HEIGHT) + " L " + str(center2) + " " + str(to.y);
    }
    if (to.y == from.y) {
        int bottom = from.y + BOX_HEIGHT;
        return "M " + str(center1) + " " + str(bottom) + " Q " + str((center1 + center2) / 2) + " " +
               str(bottom + GAP_Y / 2) + " " + str(center2) + " " + str(bottom);
    }
    int middle = BOX_HEIGHT / 2;
    int right1 = from.x + from.width;
    int right2 = to.x + to.width;
    int bend = std::max(right1, right2) + GAP_Y;
    return "M " + str(right1) + " " + str(from.y + middle) + " C " + str(bend) + " " + str(from.y + middle) + " " +
           str(bend) + " " + str(to.y + middle) + " " + str(right2) + " " + str(to.y + middle);
}

/// Записать текст в файл.
void writeFile(const std::string& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary);
    if (!file || !(file << text)) {
        throw RenderError("cannot write " + path);
    }
}

/// Команда для запуска dot; на Windows вся строка берётся ещё в одни кавычки,
/// потому что cmd.exe убирает первую и последнюю кавычку.
std::string graphvizCommand(const std::string& dotCommand, const std::string& dotPath, const std::string& svgPath) {
    std::string command = "\"" + dotCommand + "\" -Tsvg \"" + dotPath + "\" -o \"" + svgPath + "\" 2>" + NULL_DEVICE;
#ifdef _WIN32
    command = "\"" + command + "\"";
#endif
    return command;
}

}  // namespace

std::vector<std::pair<std::string, int>> levels(const Graph& graph) {
    std::vector<std::pair<std::string, int>> result = {{graph.root, 0}};
    std::map<std::string, int> level = {{graph.root, 0}};
    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto [name, depth] = result[i];
        for (const std::string& child : graph.children(name)) {
            if (level.count(child) == 0) {
                level[child] = depth + 1;
                result.push_back({child, depth + 1});
            }
        }
    }
    return result;
}

std::string builtInSvg(const Graph& graph) {
    std::map<std::string, Box> boxes = layout(graph);
    int width = 0;
    int height = 0;
    for (const auto& entry : boxes) {
        width = std::max(width, entry.second.x + entry.second.width);
        height = std::max(height, entry.second.y);
    }
    width += MARGIN + GAP_Y;
    height += BOX_HEIGHT + GAP_Y;
    std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" + std::to_string(width) + "\" height=\"" +
                      std::to_string(height) + "\">\n";
    svg += "<defs><marker id=\"arrow\" markerWidth=\"10\" markerHeight=\"7\" refX=\"10\" refY=\"3.5\" "
           "orient=\"auto\"><polygon points=\"0 0, 10 3.5, 0 7\" fill=\"" + EDGE_COLOR + "\"/></marker></defs>\n";
    auto closing = cycleEdges(graph);
    for (const std::string& name : graph.nodes) {
        for (const std::string& child : graph.children(name)) {
            bool cycle = closing.count({name, child}) != 0;
            svg += "<path d=\"" + edgePath(boxes.at(name), boxes.at(child)) + "\" fill=\"none\" stroke=\"" +
                   (cycle ? CYCLE_COLOR + "\" stroke-dasharray=\"5,4" : EDGE_COLOR) +
                   "\" marker-end=\"url(#arrow)\"/>\n";
        }
    }
    for (const auto& [name, depth] : levels(graph)) {
        const Box& box = boxes.at(name);
        std::string fill = name == graph.root ? ROOT_COLOR : NODE_COLOR;
        svg += "<rect x=\"" + std::to_string(box.x) + "\" y=\"" + std::to_string(box.y) + "\" width=\"" +
               std::to_string(box.width) + "\" height=\"" + std::to_string(BOX_HEIGHT) + "\" rx=\"6\" fill=\"" + fill +
               "\" stroke=\"#333333\"/>\n";
        svg += "<text x=\"" + std::to_string(box.x + box.width / 2) + "\" y=\"" +
               std::to_string(box.y + BOX_HEIGHT / 2 + 4) +
               "\" text-anchor=\"middle\" font-family=\"Helvetica\" font-size=\"12\">" +
               escapeXml(labelOf(graph, name)) + "</text>\n";
    }
    return svg + "</svg>\n";
}

std::string findProgram(const std::string& name) {
    const char* path = std::getenv("PATH");
    if (path == nullptr) {
        return "";
    }
    std::vector<std::string> names = {name};
#ifdef _WIN32
    names.insert(names.begin(), name + ".exe");
#endif
    std::string folders = path;
    std::size_t start = 0;
    while (start <= folders.size()) {
        std::size_t end = folders.find(PATH_SEPARATOR, start);
        std::string folder = folders.substr(start, end == std::string::npos ? std::string::npos : end - start);
        for (const std::string& candidate : names) {
            std::error_code error;
            std::filesystem::path file = std::filesystem::path(folder) / candidate;
            if (!folder.empty() && std::filesystem::is_regular_file(file, error)) {
                return file.string();
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return "";
}

std::string saveSvg(const Graph& graph, const std::string& dotPath, const std::string& svgPath,
                    const std::string& dotCommand) {
    if (dotCommand.empty()) {
        writeFile(svgPath, builtInSvg(graph));
        return BUILT_IN;
    }
    int code = std::system(graphvizCommand(dotCommand, dotPath, svgPath).c_str());
    if (code != 0) {
        throw RenderError("graphviz dot failed for " + dotPath + " (exit code " + std::to_string(code) + ")");
    }
    return GRAPHVIZ;
}
