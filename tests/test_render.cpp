// Тесты текстовых представлений графа: Graphviz (DOT) и ASCII-дерево.

#include <algorithm>
#include <string>
#include <vector>

#include "graph.h"
#include "local_repo.h"
#include "render.h"
#include "test.h"

namespace {

Graph graphFrom(const PackageMap& packages, const std::string& root = "A") {
    LocalRepository source(packages, root);
    return buildGraph(root, source);
}

std::size_t countOf(const std::string& text, const std::string& part) {
    std::size_t count = 0;
    for (std::size_t pos = text.find(part); pos != std::string::npos; pos = text.find(part, pos + 1)) {
        ++count;
    }
    return count;
}

using Lines = std::vector<std::string>;

}  // namespace

TEST(dot_structure) {
    std::string dot = toDot(graphFrom({{"A", {"B", "C"}}, {"B", {"C"}}}));
    CHECK_EQ(dot.rfind("digraph dependencies {\n", 0), std::size_t(0));
    CHECK_CONTAINS(dot, "    \"A\" -> \"B\";\n");
    CHECK_CONTAINS(dot, "    \"B\" -> \"C\";\n");
    CHECK_EQ(dot.substr(dot.size() - 2), std::string("}\n"));
}

TEST(dot_each_edge_once) {
    std::string dot = toDot(graphFrom({{"A", {"B", "C"}}, {"B", {"D"}}, {"C", {"D"}}}));
    CHECK_EQ(countOf(dot, "-> "), std::size_t(4));
}

TEST(dot_root_highlighted) {
    std::string dot = toDot(graphFrom({{"A", {"B"}}}));
    CHECK_CONTAINS(dot, "\"A\" [label=\"A\", fillcolor=\"#cfe0f7\", penwidth=2];");
    CHECK_CONTAINS(dot, "\"B\" [label=\"B\"];");
}

TEST(dot_cycle_edge_red) {
    std::string dot = toDot(graphFrom({{"A", {"B"}}, {"B", {"A"}}}));
    CHECK_CONTAINS(dot, "\"B\" -> \"A\" [color=\"#d62728\", style=dashed];");
    CHECK_CONTAINS(dot, "\"A\" -> \"B\";");
}

TEST(dot_version_on_second_line) {
    Graph graph = graphFrom({{"A", {}}});
    graph.labels["A"] = "Pkg \"x\" 1.0.0";
    CHECK_CONTAINS(toDot(graph), "label=\"Pkg\\n\\\"x\\\" 1.0.0\"");
}

TEST(tree_simple) {
    CHECK_EQ(asciiTree(graphFrom({{"A", {"B", "C"}}, {"B", {"D"}}})), (Lines{"A", "+-- B", "|   \\-- D", "\\-- C"}));
}

TEST(tree_repeated_subtree) {
    Lines tree = asciiTree(graphFrom({{"A", {"B", "C"}}, {"B", {"D"}}, {"C", {"B"}}, {"D", {"E"}}}));
    CHECK_EQ(tree, (Lines{"A", "+-- B", "|   \\-- D", "|       \\-- E", "\\-- C", "    \\-- B (see above)"}));
}

TEST(tree_repeated_leaf_without_mark) {
    Lines tree = asciiTree(graphFrom({{"A", {"B", "C"}}, {"B", {"D"}}, {"C", {"D"}}}));
    CHECK_EQ(tree, (Lines{"A", "+-- B", "|   \\-- D", "\\-- C", "    \\-- D"}));
}

TEST(tree_cycle_mark) {
    CHECK_EQ(asciiTree(graphFrom({{"A", {"B"}}, {"B", {"A"}}})), (Lines{"A", "\\-- B", "    \\-- A (cycle)"}));
}

TEST(tree_self_loop) {
    CHECK_EQ(asciiTree(graphFrom({{"A", {"A", "B"}}})), (Lines{"A", "+-- A (cycle)", "\\-- B"}));
}

TEST(tree_only_ascii) {
    for (const std::string& line : asciiTree(graphFrom({{"A", {"B", "C"}}, {"C", {"D"}}}))) {
        CHECK(std::all_of(line.begin(), line.end(), [](char c) { return c >= 32 && c < 127; }));
    }
}
