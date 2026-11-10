// Тесты построения графа обходом в глубину без рекурсии.

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "graph.h"
#include "test.h"

namespace {

/// Источник данных из словаря пакет -> список зависимостей; запоминает запросы.
class DictSource : public DependencySource {
public:
    explicit DictSource(std::map<std::string, std::vector<std::string>> packages) : packages_(std::move(packages)) {}

    std::vector<std::string> calls;

    std::vector<std::string> dependencies(const std::string& name) override {
        calls.push_back(name);
        auto found = packages_.find(name);
        return found == packages_.end() ? std::vector<std::string>{} : found->second;
    }

    std::string label(const std::string& name) override {
        return "<" + name + ">";
    }

private:
    std::map<std::string, std::vector<std::string>> packages_;
};

using Cycles = std::vector<std::vector<std::string>>;

}  // namespace

TEST(graph_transitive_dependencies) {
    DictSource source({{"A", {"B"}}, {"B", {"C"}}});
    Graph graph = buildGraph("A", source);
    CHECK_EQ(graph.nodes, (std::vector<std::string>{"A", "B", "C"}));
    CHECK_EQ(graph.children("B"), std::vector<std::string>{"C"});
    CHECK(graph.children("C").empty());
    CHECK(graph.cycles.empty());
}

TEST(graph_shared_dependency_visited_once) {
    DictSource source({{"A", {"B", "C"}}, {"B", {"D"}}, {"C", {"D"}}});
    Graph graph = buildGraph("A", source);
    CHECK_EQ(std::count(source.calls.begin(), source.calls.end(), "D"), 1);
    CHECK_EQ(graph.nodes.size(), std::size_t(4));
}

TEST(graph_depth_first_order) {
    DictSource source({{"A", {"B", "C"}}, {"B", {"D"}}, {"C", {"E"}}});
    buildGraph("A", source);
    CHECK_EQ(source.calls, (std::vector<std::string>{"A", "B", "D", "C", "E"}));
}

TEST(graph_cycle) {
    DictSource source({{"A", {"B"}}, {"B", {"C"}}, {"C", {"A"}}});
    Graph graph = buildGraph("A", source);
    CHECK(graph.cycles == (Cycles{{"A", "B", "C", "A"}}));
    CHECK_EQ(graph.nodes.size(), std::size_t(3));
}

TEST(graph_self_loop) {
    DictSource source({{"A", {"A", "B"}}});
    Graph graph = buildGraph("A", source);
    CHECK(graph.cycles == (Cycles{{"A", "A"}}));
}

TEST(graph_two_cycles) {
    DictSource source({{"A", {"B", "D"}}, {"B", {"C"}}, {"C", {"B"}}, {"D", {"E"}}, {"E", {"D"}}});
    Graph graph = buildGraph("A", source);
    CHECK(graph.cycles == (Cycles{{"B", "C", "B"}, {"D", "E", "D"}}));
}

TEST(graph_no_stack_overflow_on_long_chain) {
    const int length = 200000;
    std::map<std::string, std::vector<std::string>> packages;
    for (int i = 0; i < length; ++i) {
        packages[std::to_string(i)] = {std::to_string(i + 1)};
    }
    DictSource source(packages);
    Graph graph = buildGraph("0", source);
    CHECK_EQ(graph.nodes.size(), std::size_t(length + 1));
}

TEST(graph_labels) {
    DictSource source({{"A", {"B"}}});
    Graph graph = buildGraph("A", source);
    CHECK_EQ(graph.labels["A"], std::string("<A>"));
    CHECK_EQ(graph.labels["B"], std::string("<B>"));
}
