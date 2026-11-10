// Тесты запуска приложения целиком (сеть подменяется).

#include "fake_nuget.h"
#include "helpers.h"
#include "test.h"

TEST(app_test_mode_graph) {
    RunResult result = runWith({"--package", "A", "--test-mode", "--repo", dataPath("cycle.txt")});
    CHECK_EQ(result.code, 0);
    CHECK_CONTAINS(result.out, std::string("Dependency graph of A (4 packages):\n"
                                     "  A -> B\n"
                                     "  B -> C\n"
                                     "  C -> A, D\n"
                                     "  D -> (none)\n"
                                     "Cycles:\n"
                                     "  A -> B -> C -> A\n"));
}

TEST(app_test_mode_without_cycles) {
    RunResult result = runWith({"--package", "A", "--test-mode", "--repo", dataPath("simple.txt")});
    CHECK_EQ(result.code, 0);
    CHECK_CONTAINS(result.out, "Dependency graph of A (4 packages):\n");
    CHECK_CONTAINS(result.out, "Cycles: none\n");
}

TEST(app_nuget_graph) {
    FakeFetcher fetcher = serverFetcher({{"Root.Pkg", {{"1.0.0", {{"Dep.One", "2.0"}}}}},
                                         {"Dep.One", {{"2.0.0", {}}, {"2.1.0", {}}}}});
    RunResult result = runWith({"--package", "Root.Pkg"}, fetcher);
    CHECK_EQ(result.code, 0);
    CHECK_CONTAINS(result.out, std::string("Dependency graph of Root.Pkg 1.0.0 (2 packages):\n"
                                     "  Root.Pkg 1.0.0 -> Dep.One\n"
                                     "  Dep.One 2.0.0 -> (none)\n"
                                     "Cycles: none\n"));
}

TEST(app_serilog_console) {
    FakeFetcher fetcher = serilogConsoleFetcher();
    fetcher.add(BASE + "serilog/index.json", R"({"versions": ["3.1.1", "4.0.0", "4.2.0"]})");
    fetcher.add(BASE + "serilog/4.0.0/serilog.nuspec", makeNuspec("Serilog", "4.0.0", {}));
    RunResult result = runWith({"--package", "Serilog.Sinks.Console"}, fetcher);
    CHECK_EQ(result.code, 0);
    CHECK_CONTAINS(result.out, "  Serilog.Sinks.Console 6.0.0 -> Serilog\n  Serilog 4.0.0 -> (none)\n");
}

TEST(app_load_order) {
    RunResult result = runWith({"--package", "A", "--test-mode", "--load-order", "--repo", dataPath("simple.txt")});
    CHECK_EQ(result.code, 0);
    CHECK_CONTAINS(result.out, std::string("Load order of A (dependencies first):\n"
                                     "  1. D\n"
                                     "  2. B\n"
                                     "  3. C\n"
                                     "  4. A\n"));
}

TEST(app_load_order_with_cycle) {
    RunResult result = runWith({"--package", "A", "--test-mode", "--load-order", "--repo", dataPath("cycle.txt")});
    CHECK_EQ(result.code, 0);
    CHECK_CONTAINS(result.out, "  1. D\n  2. C\n  3. B\n  4. A\n");
    CHECK_CONTAINS(result.out, "Note: cycle A -> B -> C -> A - its edge C -> A was ignored\n");
}

TEST(app_load_order_nuget_versions) {
    FakeFetcher fetcher = serverFetcher({{"Root.Pkg", {{"1.0.0", {{"Dep.One", "2.0"}}}}},
                                         {"Dep.One", {{"2.0.0", {}}, {"2.1.0", {}}}}});
    RunResult result = runWith({"--package", "Root.Pkg", "--load-order"}, fetcher);
    CHECK_EQ(result.code, 0);
    CHECK_CONTAINS(result.out, std::string("Load order of Root.Pkg 1.0.0 (dependencies first):\n"
                                     "  1. Dep.One 2.0.0\n"
                                     "  2. Root.Pkg 1.0.0\n"));
}

TEST(app_visualization_files) {
    TempDir folder;
    std::string output = (folder.path / "deps.svg").string();
    RunResult result =
        runWith({"--package", "A", "--test-mode", "--repo", dataPath("deep.txt"), "--output", output});
    CHECK_EQ(result.code, 0);
    CHECK(std::filesystem::is_regular_file(output));
    CHECK(std::filesystem::is_regular_file(folder.path / "deps.dot"));
    CHECK_CONTAINS(result.out, "Graphviz description saved to " + (folder.path / "deps.dot").string() + "\n");
    CHECK_CONTAINS(result.out, "SVG image saved to " + output + " (");
}

TEST(app_ascii_tree_option) {
    RunResult result = runWith({"--package", "A", "--test-mode", "--ascii-tree", "--repo", dataPath("cycle.txt")});
    CHECK_EQ(result.code, 0);
    CHECK_CONTAINS(result.out, "\nA\n\\-- B\n    \\-- C\n        +-- A (cycle)\n        \\-- D\n");
}

TEST(app_no_ascii_tree_by_default) {
    RunResult result = runWith({"--package", "A", "--test-mode", "--repo", dataPath("cycle.txt")});
    CHECK(result.out.find("(cycle)") == std::string::npos);
}

TEST(app_output_write_error) {
    TempDir folder;
    std::filesystem::create_directory(folder.path / "busy.dot");
    RunResult result = runWith({"--package", "A", "--test-mode", "--repo", dataPath("simple.txt"), "--output",
                                (folder.path / "busy.svg").string()});
    CHECK_EQ(result.code, 1);
    CHECK_CONTAINS(result.err, "error: cannot write");
}

TEST(app_test_mode_unknown_package) {
    RunResult result = runWith({"--package", "Z", "--test-mode", "--repo", dataPath("simple.txt")});
    CHECK_EQ(result.code, 1);
    CHECK_CONTAINS(result.err, "error: package not found in test repository: Z");
}

TEST(app_test_mode_bad_file) {
    RunResult result = runWith({"--package", "A", "--test-mode", "--repo", dataPath("bad_name.txt")});
    CHECK_EQ(result.code, 1);
    CHECK_CONTAINS(result.err, "error: line 3: invalid package name 'b'");
}

TEST(app_config_error) {
    RunResult result = runWith({"--package", "Serilog", "--output", "g.png"});
    CHECK_EQ(result.code, 1);
    CHECK_CONTAINS(result.err, "error: ");
    CHECK_CONTAINS(result.err, "usage: ");
    CHECK_CONTAINS(result.out, std::string(""));
}

TEST(app_network_error) {
    FakeFetcher fetcher;
    fetcher.offline = true;
    RunResult result = runWith({"--package", "Serilog"}, fetcher);
    CHECK_EQ(result.code, 1);
    CHECK_CONTAINS(result.err, "error: cannot connect");
}

TEST(app_unknown_package) {
    FakeFetcher fetcher = serilogConsoleFetcher();
    RunResult result = runWith({"--package", "No.Such"}, fetcher);
    CHECK_EQ(result.code, 1);
    CHECK_CONTAINS(result.err, "error: package not found: No.Such");
}
