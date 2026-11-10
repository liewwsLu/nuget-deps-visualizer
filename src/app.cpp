#include "app.h"

#include <exception>

#include "config.h"
#include "graph.h"
#include "local_repo.h"
#include "nuget.h"

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

}  // namespace

int runApp(const std::vector<std::string>& args, Fetcher& fetcher, std::ostream& out, std::ostream& err) {
    try {
        Config config = parseConfig(args);
        printGraph(makeGraph(config, fetcher), out);
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
