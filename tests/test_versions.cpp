// Тесты сравнения версий.

#include <stdexcept>

#include "test.h"
#include "versions.h"

TEST(versions_numeric_order) {
    CHECK(versionKey("1.2.0") < versionKey("1.10.0"));
    CHECK(versionKey("2.0.0") < versionKey("10.0.0"));
}

TEST(versions_short_equals_long) {
    CHECK(versionKey("4.0") == versionKey("4.0.0.0"));
}

TEST(versions_prerelease_before_release) {
    CHECK(versionKey("2.0.0-beta") < versionKey("2.0.0"));
    CHECK(versionKey("2.0.0-alpha") < versionKey("2.0.0-beta"));
    CHECK(versionKey("1.9.9") < versionKey("2.0.0-beta"));
}

TEST(versions_build_metadata_ignored) {
    CHECK(versionKey("1.0.0+abc") == versionKey("1.0.0"));
    CHECK_EQ(isPrerelease("1.0.0+build-5"), false);
}

TEST(versions_is_prerelease) {
    CHECK_EQ(isPrerelease("6.0.1-dev-00953"), true);
    CHECK_EQ(isPrerelease("6.0.0"), false);
}

TEST(versions_latest_stable) {
    CHECK_EQ(latestStable({"5.0.0", "6.0.0", "6.0.1-dev-00953", "5.1.0"}), std::string("6.0.0"));
    CHECK_EQ(latestStable({"1.2.0", "1.10.0", "1.9.0"}), std::string("1.10.0"));
}

TEST(versions_only_prereleases) {
    CHECK_EQ(latestStable({"1.0.0-alpha", "1.0.0-beta"}), std::string("1.0.0-beta"));
}

TEST(versions_no_versions) {
    CHECK_THROWS(latestStable({}), std::invalid_argument);
}

TEST(versions_describe_range) {
    CHECK_EQ(describeRange("4.0.0"), std::string(">= 4.0.0"));
    CHECK_EQ(describeRange("[1.0.0, 2.0.0)"), std::string("[1.0.0, 2.0.0)"));
    CHECK_EQ(describeRange(""), std::string("any version"));
}

// Диапазоны версий и выбор наименьшей подходящей версии.

const std::vector<std::string> VERSIONS = {"1.0.0", "1.5.0", "2.0.0", "2.1.0-beta", "3.0.0"};

TEST(range_minimum_only) {
    CHECK_EQ(lowestMatching("1.5", VERSIONS), std::string("1.5.0"));
}

TEST(range_minimum_not_published) {
    CHECK_EQ(lowestMatching("1.2", VERSIONS), std::string("1.5.0"));
}

TEST(range_exclusive_bounds) {
    CHECK_EQ(lowestMatching("(1.0.0, 3.0.0)", VERSIONS), std::string("1.5.0"));
    CHECK_EQ(lowestMatching("(1.0.0,)", VERSIONS), std::string("1.5.0"));
}

TEST(range_inclusive_upper_bound) {
    CHECK(parseRange("[1.0, 2.0]").contains("2.0.0"));
    CHECK(!parseRange("[1.0, 2.0)").contains("2.0.0"));
}

TEST(range_exact) {
    CHECK_EQ(lowestMatching("[2.0.0]", VERSIONS), std::string("2.0.0"));
}

TEST(range_upper_bound_only) {
    CHECK_EQ(lowestMatching("(, 2.0.0]", VERSIONS), std::string("1.0.0"));
}

TEST(range_any_version) {
    CHECK_EQ(lowestMatching("", VERSIONS), std::string("1.0.0"));
}

TEST(range_skips_prerelease) {
    CHECK_EQ(lowestMatching("2.0.1", VERSIONS), std::string("3.0.0"));
}

TEST(range_prerelease_lower_bound) {
    CHECK_EQ(lowestMatching("2.1.0-alpha", VERSIONS), std::string("2.1.0-beta"));
}

TEST(range_no_match) {
    CHECK_EQ(lowestMatching("[5.0, 6.0)", VERSIONS), std::string(""));
}

TEST(range_invalid) {
    CHECK_THROWS(parseRange("[1.0, 2.0"), VersionError);
}
