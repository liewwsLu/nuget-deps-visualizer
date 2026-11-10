// Тесты порядка загрузки зависимостей.

#include <map>
#include <string>
#include <vector>

#include "graph.h"
#include "helpers.h"
#include "local_repo.h"
#include "test.h"

namespace {

using Order = std::vector<std::string>;

/// Построить граф по описанию пакетов.
Graph graphFrom(const PackageMap& packages, const std::string& root = "A") {
    LocalRepository source(packages, root);
    return buildGraph(root, source);
}

/// Каждая зависимость (кроме связи, замыкающей цикл) стоит в порядке раньше пакета.
void checkDependenciesFirst(const Graph& graph, const Order& order) {
    std::map<std::string, std::size_t> position;
    for (std::size_t i = 0; i < order.size(); ++i) {
        position[order[i]] = i;
    }
    for (const std::string& name : graph.nodes) {
        for (const std::string& child : graph.children(name)) {
            bool closesCycle = false;
            for (const auto& cycle : graph.cycles) {
                closesCycle = closesCycle || (cycle[cycle.size() - 2] == name && cycle.back() == child);
            }
            CHECK(closesCycle || position.at(child) < position.at(name));
        }
    }
}

}  // namespace

TEST(order_chain) {
    CHECK_EQ(loadOrder(graphFrom({{"A", {"B"}}, {"B", {"C"}}, {"C", {}}})), (Order{"C", "B", "A"}));
}

TEST(order_root_is_last) {
    Order order = loadOrder(graphFrom({{"A", {"B", "C"}}, {"B", {}}, {"C", {}}}));
    CHECK_EQ(order.back(), std::string("A"));
}

TEST(order_shared_dependency_once) {
    CHECK_EQ(loadOrder(graphFrom({{"A", {"B", "C"}}, {"B", {"D"}}, {"C", {"D"}}})), (Order{"D", "B", "C", "A"}));
}

TEST(order_deep_file) {
    Graph graph = graphFrom(loadLocalRepository(dataPath("deep.txt")));
    Order order = loadOrder(graph);
    CHECK_EQ(order, (Order{"G", "D", "H", "E", "B", "F", "C", "A"}));
    checkDependenciesFirst(graph, order);
}

TEST(order_cycle) {
    Graph graph = graphFrom({{"A", {"B"}}, {"B", {"C"}}, {"C", {"A", "D"}}});
    Order order = loadOrder(graph);
    CHECK_EQ(order, (Order{"D", "C", "B", "A"}));
    checkDependenciesFirst(graph, order);
}

TEST(order_two_cycles) {
    Graph graph = graphFrom(loadLocalRepository(dataPath("two_cycles.txt")));
    Order order = loadOrder(graph);
    CHECK_EQ(order.size(), std::size_t(6));
    checkDependenciesFirst(graph, order);
}

TEST(order_single_package) {
    CHECK_EQ(loadOrder(graphFrom({{"A", {}}})), Order{"A"});
}

TEST(order_long_chain_without_recursion) {
    PackageMap packages;
    const int length = 100000;
    Graph graph;
    graph.root = "0";
    for (int i = 0; i < length; ++i) {
        graph.nodes.push_back(std::to_string(i));
        graph.edges[std::to_string(i)] = {std::to_string(i + 1)};
    }
    Order order = loadOrder(graph);
    CHECK_EQ(order.size(), std::size_t(length + 1));
    CHECK_EQ(order.front(), std::to_string(length));
}
