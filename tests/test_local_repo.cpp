// Тесты тестового репозитория (файл с описанием графа пакетов).

#include "graph.h"
#include "helpers.h"
#include "local_repo.h"
#include "test.h"

namespace {

PackageMap load(const std::string& name) {
    return loadLocalRepository(dataPath(name));
}

using Cycles = std::vector<std::vector<std::string>>;

}  // namespace

TEST(local_parse_simple) {
    PackageMap packages = load("simple.txt");
    CHECK_EQ(packages.size(), std::size_t(4));
    CHECK_EQ(packages["A"], (std::vector<std::string>{"B", "C"}));
    CHECK(packages["D"].empty());
}

TEST(local_comments_and_blank_lines) {
    PackageMap packages = parseLocalRepository("# header\n\nA: B  # tail comment\r\n   \nB:\n");
    CHECK_EQ(packages["A"], std::vector<std::string>{"B"});
    CHECK_EQ(packages.size(), std::size_t(2));
}

TEST(local_lowercase_name) {
    try {
        load("bad_name.txt");
        CHECK(false);
    } catch (const LocalRepositoryError& error) {
        CHECK_CONTAINS(error.what(), "line 3: invalid package name 'b'");
    }
}

TEST(local_invalid_dependency_name) {
    CHECK_THROWS(parseLocalRepository("A: B c1\n"), LocalRepositoryError);
}

TEST(local_no_colon) {
    try {
        load("bad_format.txt");
        CHECK(false);
    } catch (const LocalRepositoryError& error) {
        CHECK_CONTAINS(error.what(), "line 2: expected 'NAME: DEPENDENCIES'");
    }
}

TEST(local_duplicate) {
    CHECK_THROWS(parseLocalRepository("A: B\nA: C\n"), LocalRepositoryError);
}

TEST(local_missing_file) {
    CHECK_THROWS(load("no_such_file.txt"), LocalRepositoryError);
}

TEST(local_unknown_root) {
    CHECK_THROWS(LocalRepository({{"A", {}}}, "Z"), LocalRepositoryError);
}

TEST(local_undescribed_dependency_is_leaf) {
    LocalRepository source({{"A", {"B"}}}, "A");
    Graph graph = buildGraph("A", source);
    CHECK(graph.children("B").empty());
    CHECK_EQ(graph.labels["B"], std::string("B"));
}

TEST(local_cycle_file) {
    LocalRepository source(load("cycle.txt"), "A");
    Graph graph = buildGraph("A", source);
    CHECK(graph.cycles == (Cycles{{"A", "B", "C", "A"}}));
}

TEST(local_deep_file) {
    LocalRepository source(load("deep.txt"), "A");
    Graph graph = buildGraph("A", source);
    CHECK_EQ(graph.nodes, (std::vector<std::string>{"A", "B", "D", "G", "E", "H", "C", "F"}));
    CHECK(graph.cycles.empty());
}

TEST(local_subgraph_from_middle) {
    LocalRepository source(load("deep.txt"), "E");
    Graph graph = buildGraph("E", source);
    CHECK_EQ(graph.nodes, (std::vector<std::string>{"E", "G", "H"}));
}
