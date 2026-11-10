// Тесты работы с репозиторием NuGet (сеть подменяется FakeFetcher).

#include "fake_nuget.h"
#include "http.h"
#include "graph.h"
#include "nuget.h"
#include "test.h"

TEST(nuget_base_from_service_index) {
    FakeFetcher fetcher = serilogConsoleFetcher();
    NuGetRepository repository(INDEX_URL, fetcher);
    CHECK_EQ(repository.baseAddress(), BASE);
    repository.baseAddress();
    CHECK_EQ(fetcher.requested.size(), std::size_t(1));
}

TEST(nuget_direct_flat_container_url) {
    FakeFetcher fetcher;
    NuGetRepository repository("https://example.org/flat", fetcher);
    CHECK_EQ(repository.baseAddress(), std::string("https://example.org/flat/"));
    CHECK(fetcher.requested.empty());
}

TEST(nuget_index_without_resource) {
    FakeFetcher fetcher;
    fetcher.add(INDEX_URL, R"({"version": "3.0.0", "resources": []})");
    NuGetRepository repository(INDEX_URL, fetcher);
    CHECK_THROWS(repository.baseAddress(), RepositoryError);
}

TEST(nuget_invalid_json) {
    FakeFetcher fetcher;
    fetcher.add(INDEX_URL, "<html>Service unavailable</html>");
    NuGetRepository repository(INDEX_URL, fetcher);
    CHECK_THROWS(repository.baseAddress(), RepositoryError);
}

TEST(nuget_server_error_status) {
    FakeFetcher fetcher;
    fetcher.responses[INDEX_URL] = {500, "error"};
    NuGetRepository repository(INDEX_URL, fetcher);
    try {
        repository.baseAddress();
        CHECK(false);
    } catch (const RepositoryError& error) {
        CHECK_CONTAINS(error.what(), "HTTP 500");
    }
}

TEST(nuget_connection_error) {
    FakeFetcher fetcher;
    fetcher.offline = true;
    NuGetRepository repository(INDEX_URL, fetcher);
    CHECK_THROWS(repository.baseAddress(), RepositoryError);
}

TEST(nuget_utf8_bom_skipped) {
    FakeFetcher fetcher;
    fetcher.add(BASE + "a/index.json", "\xEF\xBB\xBF{\"versions\": [\"1.0.0\"]}");
    NuGetRepository repository(BASE, fetcher);
    CHECK_EQ(repository.versions("A"), std::vector<std::string>{"1.0.0"});
}

TEST(nuget_latest_stable_version) {
    FakeFetcher fetcher = serilogConsoleFetcher();
    NuGetRepository repository(INDEX_URL, fetcher);
    DirectDependencies direct = repository.directDependencies("Serilog.Sinks.Console");
    CHECK_EQ(direct.version, std::string("6.0.0"));
    CHECK_EQ(direct.framework, std::string(".NETStandard2.0"));
    CHECK_EQ(direct.dependencies.size(), std::size_t(1));
    CHECK_EQ(direct.dependencies[0].packageId, std::string("Serilog"));
}

TEST(nuget_urls_are_lowercase) {
    FakeFetcher fetcher = serilogConsoleFetcher();
    NuGetRepository repository(INDEX_URL, fetcher);
    repository.directDependencies("Serilog.Sinks.Console", "6.0.0");
    CHECK_EQ(fetcher.requested.back(),
             BASE + "serilog.sinks.console/6.0.0/serilog.sinks.console.nuspec");
}

TEST(nuget_unknown_package) {
    FakeFetcher fetcher = serilogConsoleFetcher();
    NuGetRepository repository(INDEX_URL, fetcher);
    try {
        repository.versions("No.Such.Package");
        CHECK(false);
    } catch (const RepositoryError& error) {
        CHECK_CONTAINS(error.what(), "package not found: No.Such.Package");
    }
}

TEST(nuget_unknown_version) {
    FakeFetcher fetcher = serilogConsoleFetcher();
    NuGetRepository repository(INDEX_URL, fetcher);
    try {
        repository.nuspec("Serilog.Sinks.Console", "9.9.9");
        CHECK(false);
    } catch (const RepositoryError& error) {
        CHECK_CONTAINS(error.what(), "version 9.9.9 of Serilog.Sinks.Console not found");
    }
}

TEST(http_parse_curl_output) {
    HttpResponse response = parseCurlOutput("{\"a\": 1}\n200");
    CHECK_EQ(response.status, 200);
    CHECK_EQ(response.body, std::string("{\"a\": 1}"));
    CHECK_EQ(parseCurlOutput("line1\nline2\n404").body, std::string("line1\nline2"));
    CHECK_EQ(parseCurlOutput("\n000").status, 0);
    CHECK_EQ(parseCurlOutput("").status, 0);
}

// Граф по данным репозитория NuGet.

const FakeServer SERVER = {
    {"App.Core", {{"1.0.0", {{"Lib.A", "1.0.0"}, {"Lib.B", "[2.0.0, 3.0.0)"}}}, {"2.0.0", {{"Lib.A", "1.5.0"}}}}},
    {"Lib.A", {{"1.0.0", {{"Lib.C", "1.0.0"}}}, {"1.5.0", {}}, {"2.0.0-beta", {}}}},
    {"Lib.B", {{"1.0.0", {}}, {"2.0.0", {{"lib.c", "1.0.0"}}}, {"2.5.0", {}}}},
    {"Lib.C", {{"1.0.0", {}}, {"1.1.0", {}}}},
};

TEST(source_root_latest_dependency_lowest) {
    FakeFetcher fetcher = serverFetcher(SERVER);
    NuGetRepository repository(INDEX_URL, fetcher);
    NuGetSource source(repository, "App.Core");
    Graph graph = buildGraph("App.Core", source);
    CHECK_EQ(graph.labels["App.Core"], std::string("App.Core 2.0.0"));
    CHECK_EQ(graph.labels["Lib.A"], std::string("Lib.A 1.5.0"));
}

TEST(source_transitive_and_ranges) {
    FakeFetcher fetcher = serverFetcher(SERVER);
    NuGetRepository repository(INDEX_URL, fetcher);
    NuGetSource source(repository, "App.Core");
    source.useVersion("App.Core", "1.0.0");
    Graph graph = buildGraph("App.Core", source);
    CHECK_EQ(graph.labels["Lib.B"], std::string("Lib.B 2.0.0"));
    CHECK_EQ(graph.children("Lib.A"), std::vector<std::string>{"Lib.C"});
}

TEST(source_names_ignore_case) {
    FakeFetcher fetcher = serverFetcher(SERVER);
    NuGetRepository repository(INDEX_URL, fetcher);
    NuGetSource source(repository, "App.Core");
    source.useVersion("App.Core", "1.0.0");
    Graph graph = buildGraph("App.Core", source);
    CHECK_EQ(graph.children("Lib.B"), std::vector<std::string>{"Lib.C"});
    CHECK_EQ(graph.nodes.size(), std::size_t(4));
}

TEST(source_no_suitable_version) {
    FakeFetcher fetcher = serverFetcher({{"X", {{"1.0.0", {{"Y", "[9.0]"}}}}}, {"Y", {{"1.0.0", {}}}}});
    NuGetRepository repository(INDEX_URL, fetcher);
    NuGetSource source(repository, "X");
    try {
        buildGraph("X", source);
        CHECK(false);
    } catch (const RepositoryError& error) {
        CHECK_CONTAINS(error.what(), "no suitable version of Y for [9.0]");
    }
}
