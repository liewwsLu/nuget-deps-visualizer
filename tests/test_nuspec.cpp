// Тесты разбора файлов .nuspec и выбора группы зависимостей.

#include "fake_nuget.h"
#include "nuspec.h"
#include "test.h"

TEST(nuspec_groups_by_framework) {
    auto groups = parseDependencyGroups(readNugetData("serilog.sinks.console.6.0.0.nuspec"));
    CHECK_EQ(groups.size(), std::size_t(3));
    CHECK_EQ(groups[1].framework, std::string(".NETStandard2.0"));
    CHECK_EQ(groups[1].dependencies.size(), std::size_t(1));
    CHECK_EQ(groups[1].dependencies[0].packageId, std::string("Serilog"));
    CHECK_EQ(groups[1].dependencies[0].versionRange, std::string("4.0.0"));
}

TEST(nuspec_old_style_without_groups) {
    auto groups = parseDependencyGroups(readNugetData("old_style.nuspec"));
    CHECK_EQ(groups.size(), std::size_t(1));
    CHECK_EQ(groups[0].framework, std::string(""));
    CHECK_EQ(groups[0].dependencies[0].versionRange, std::string("[1.0.0, 2.0.0)"));
    CHECK_EQ(groups[0].dependencies[1].packageId, std::string("Second.Dependency"));
    CHECK_EQ(groups[0].dependencies[1].versionRange, std::string(""));
}

TEST(nuspec_empty_groups) {
    auto groups = parseDependencyGroups(readNugetData("no_dependencies.nuspec"));
    CHECK_EQ(groups.size(), std::size_t(2));
    CHECK(selectGroup(groups).dependencies.empty());
}

TEST(nuspec_no_dependencies_element) {
    CHECK(parseDependencyGroups("<package><metadata><id>A</id></metadata></package>").empty());
}

TEST(nuspec_broken_xml) {
    CHECK_THROWS(parseDependencyGroups("<package><metadata></package>"), NuspecError);
}

TEST(nuspec_dependency_without_id) {
    CHECK_THROWS(parseDependencyGroups("<package><dependencies><dependency version=\"1.0\"/></dependencies></package>"),
                 NuspecError);
}

TEST(nuspec_prefers_netstandard20) {
    DependencyGroup group = selectGroup({{"net8.0", {}}, {".NETStandard2.1", {}}, {".NETStandard2.0", {}}});
    CHECK_EQ(group.framework, std::string(".NETStandard2.0"));
    CHECK_EQ(selectGroup({{"net8.0", {}}, {"netstandard2.1", {}}}).framework, std::string("netstandard2.1"));
    CHECK_EQ(selectGroup({{"net8.0", {}}, {"", {}}}).framework, std::string(""));
}

TEST(nuspec_falls_back_to_first) {
    CHECK_EQ(selectGroup({{"net8.0", {}}, {"net6.0", {}}}).framework, std::string("net8.0"));
}

TEST(nuspec_select_from_empty) {
    DependencyGroup group = selectGroup({});
    CHECK_EQ(group.framework, std::string(""));
    CHECK(group.dependencies.empty());
}

TEST(nuspec_normalize_framework) {
    CHECK_EQ(normalizeFramework(".NETStandard2.0"), std::string("netstandard2.0"));
    CHECK_EQ(normalizeFramework(" net8.0 "), std::string("net8.0"));
}
