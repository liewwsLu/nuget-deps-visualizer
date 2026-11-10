#include "app.h"

#include <exception>
#include <fstream>

#include "config.h"
#include "graph.h"
#include "local_repo.h"
#include "nuget.h"
#include "render.h"
#include "svg.h"

namespace {

std::string join(const std::vector<std::string>& items, const std::string& separator) {
    std::string result;
    for (std::size_t i = 0; i < items.size(); ++i) {
        result += (i ? separator : "") + items[i];
    }
    return result;
}

/// Построить граф по тестовому файлу или по репозиторию NuGet.
Graph makeGraph(const Config& config, Fetcher& fetcher) {
    if (config.testMode) {
        LocalRepository repository(loadLocalRepository(config.repository), config.package);
        return buildGraph(config.package, repository);
    }
    NuGetRepository repository(config.repository, fetcher);
    NuGetSource source(repository, config.package);
    return buildGraph(config.package, source);
}

/// Вывести граф: для каждого пакета список его прямых зависимостей, затем циклы (этап 3).
void printGraph(const Graph& graph, std::ostream& out) {
    out << "Dependency graph of " << graph.labels.at(graph.root) << " (" << graph.nodes.size() << " packages):\n";
    for (const std::string& name : graph.nodes) {
        const std::vector<std::string>& children = graph.children(name);
        out << "  " << graph.labels.at(name) << " -> " << (children.empty() ? "(none)" : join(children, ", "))
            << "\n";
    }
    if (graph.cycles.empty()) {
        out << "Cycles: none\n";
        return;
    }
    out << "Cycles:\n";
    for (const std::vector<std::string>& cycle : graph.cycles) {
        out << "  " << join(cycle, " -> ") << "\n";
    }
}

/// Вывести порядок загрузки зависимостей (этап 4).
void printLoadOrder(const Graph& graph, std::ostream& out) {
    out << "Load order of " << graph.labels.at(graph.root) << " (dependencies first):\n";
    std::vector<std::string> order = loadOrder(graph);
    for (std::size_t i = 0; i < order.size(); ++i) {
        out << "  " << i + 1 << ". " << graph.labels.at(order[i]) << "\n";
    }
    for (const std::vector<std::string>& cycle : graph.cycles) {
        out << "Note: cycle " << join(cycle, " -> ") << " - its edge " << cycle[cycle.size() - 2] << " -> "
            << cycle.back() << " was ignored\n";
    }
}

/// Сохранить описание графа на языке DOT и изображение SVG (этап 5).
void saveVisualization(const Graph& graph, const std::string& output, std::ostream& out) {
    std::string dotPath = output.substr(0, output.size() - std::string(".svg").size()) + ".dot";
    std::ofstream file(dotPath, std::ios::binary);
    if (!file || !(file << toDot(graph))) {
        throw RenderError("cannot write " + dotPath);
    }
    file.close();
    std::string renderer = saveSvg(graph, dotPath, output, findProgram("dot"));
    out << "Graphviz description saved to " << dotPath << "\n";
    out << "SVG image saved to " << output << " (" << renderer << ")\n";
}

}  // namespace

int runApp(const std::vector<std::string>& args, Fetcher& fetcher, std::ostream& out, std::ostream& err) {
    try {
        Config config = parseConfig(args);
        Graph graph = makeGraph(config, fetcher);
        if (config.loadOrder) {
            printLoadOrder(graph, out);
        } else {
            printGraph(graph, out);
        }
        saveVisualization(graph, config.output, out);
        if (config.asciiTree) {
            for (const std::string& line : asciiTree(graph)) {
                out << line << "\n";
            }
        }
    } catch (const ConfigError& error) {
        err << "error: " << error.what() << "\n" << usage() << "\n";
        return 1;
    } catch (const std::exception& error) {
        // Ошибки репозитория, тестового файла, .nuspec и диапазонов версий.
        err << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
