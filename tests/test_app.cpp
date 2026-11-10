// Тесты запуска приложения целиком (сеть подменяется).

#include "fake_nuget.h"
#include "helpers.h"
#include "test.h"

TEST(app_prints_direct_dependencies) {
    FakeFetcher fetcher = serilogConsoleFetcher();
    RunResult result = runWith({"--package", "Serilog.Sinks.Console"}, fetcher);
    CHECK_EQ(result.code, 0);
    CHECK_EQ(result.out, std::string("Package: Serilog.Sinks.Console 6.0.0\n"
                                     "Framework: .NETStandard2.0\n"
                                     "Direct dependencies:\n"
                                     "  Serilog >= 4.0.0\n"));
    CHECK_EQ(result.err, std::string(""));
}

TEST(app_package_without_dependencies) {
    FakeFetcher fetcher;
    fetcher.add(BASE + "newtonsoft.json/index.json", R"({"versions": ["13.0.3"]})");
    fetcher.add(BASE + "newtonsoft.json/13.0.3/newtonsoft.json.nuspec", readNugetData("no_dependencies.nuspec"));
    RunResult result = runWith({"--package", "Newtonsoft.Json", "--repo", BASE}, fetcher);
    CHECK_EQ(result.code, 0);
    CHECK_CONTAINS(result.out, "Direct dependencies:\n  (none)\n");
}

TEST(app_config_error) {
    RunResult result = runWith({"--package", "Serilog", "--output", "g.png"});
    CHECK_EQ(result.code, 1);
    CHECK_CONTAINS(result.err, "error: ");
    CHECK_CONTAINS(result.err, "usage: ");
    CHECK_EQ(result.out, std::string(""));
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

TEST(app_test_mode_not_supported_yet) {
    RunResult result = runWith({"--package", "A", "--test-mode", "--repo", dataPath("simple.txt")});
    CHECK_EQ(result.code, 1);
    CHECK_CONTAINS(result.err, "test repository mode is not supported yet");
}
