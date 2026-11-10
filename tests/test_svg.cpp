// Тесты сохранения изображения графа в SVG.

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "graph.h"
#include "helpers.h"
#include "local_repo.h"
#include "render.h"
#include "svg.h"
#include "test.h"
#include "xml.h"

namespace {

/// Граф с общей зависимостью и циклом B -> D -> B.
Graph makeGraph() {
    LocalRepository source({{"A", {"B", "C"}}, {"B", {"D"}}, {"C", {"D"}}, {"D", {"B"}}}, "A");
    return buildGraph("A", source);
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

/// Все элементы с заданным именем на любой глубине.
std::vector<const XmlElement*> allNamed(const XmlElement& root, const std::string& name) {
    std::vector<const XmlElement*> result;
    std::vector<const XmlElement*> stack = {&root};
    while (!stack.empty()) {
        const XmlElement* element = stack.back();
        stack.pop_back();
        if (element->name == name) {
            result.push_back(element);
        }
        for (const XmlElement& child : element->children) {
            stack.push_back(&child);
        }
    }
    return result;
}

}  // namespace

TEST(svg_levels) {
    std::vector<std::pair<std::string, int>> expected = {{"A", 0}, {"B", 1}, {"C", 1}, {"D", 2}};
    CHECK(levels(makeGraph()) == expected);
}

TEST(svg_valid_with_all_packages) {
    XmlElement root = parseXml(builtInSvg(makeGraph()));
    CHECK_EQ(root.name, std::string("svg"));
    std::vector<std::string> texts;
    for (const XmlElement* text : allNamed(root, "text")) {
        texts.push_back(text->text);
    }
    std::sort(texts.begin(), texts.end());
    CHECK_EQ(texts, (std::vector<std::string>{"A", "B", "C", "D"}));
}

TEST(svg_one_arrow_per_edge) {
    XmlElement root = parseXml(builtInSvg(makeGraph()));
    CHECK_EQ(allNamed(root, "path").size(), std::size_t(5));
}

TEST(svg_cycle_edge_dashed) {
    std::string svg = builtInSvg(makeGraph());
    CHECK_CONTAINS(svg, "stroke=\"#d62728\" stroke-dasharray=\"5,4\"");
}

TEST(svg_labels_escaped) {
    Graph graph = makeGraph();
    graph.labels["A"] = "A<1> & \"B\"";
    XmlElement root = parseXml(builtInSvg(graph));
    CHECK_CONTAINS(builtInSvg(graph), "A&lt;1&gt; &amp; &quot;B&quot;");
    CHECK_EQ(root.name, std::string("svg"));
}

TEST(svg_save_without_graphviz) {
    TempDir folder;
    std::string path = (folder.path / "g.svg").string();
    CHECK_EQ(saveSvg(makeGraph(), "", path, ""), BUILT_IN);
    CHECK_CONTAINS(readFile(path), "<svg");
}

TEST(svg_write_error) {
    CHECK_THROWS(saveSvg(makeGraph(), "", "/no/such/dir/g.svg", ""), RenderError);
}

TEST(svg_find_program) {
    CHECK_EQ(findProgram("surely-no-such-program-xyz"), std::string(""));
}

// Тесты с Graphviz выполняются, только если программа dot установлена.

TEST(svg_graphviz) {
    std::string dot = findProgram("dot");
    if (dot.empty()) {
        return;
    }
    TempDir folder;
    std::string dotPath = folder.write("g.dot", toDot(makeGraph()));
    std::string svgPath = (folder.path / "g.svg").string();
    CHECK_EQ(saveSvg(makeGraph(), dotPath, svgPath, dot), GRAPHVIZ);
    CHECK_CONTAINS(readFile(svgPath), "<svg");
}

TEST(svg_graphviz_error) {
    std::string dot = findProgram("dot");
    if (dot.empty()) {
        return;
    }
    TempDir folder;
    std::string dotPath = folder.write("g.dot", "digraph {");
    CHECK_THROWS(saveSvg(makeGraph(), dotPath, (folder.path / "g.svg").string(), dot), RenderError);
}
