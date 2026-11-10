// Тесты разбора и проверки параметров командной строки.

#include "config.h"
#include "helpers.h"
#include "test.h"

namespace {

/// Проверить, что разбор args даёт ConfigError с текстом text.
void checkError(const std::vector<std::string>& args, const std::string& text) {
    try {
        parseConfig(args);
    } catch (const ConfigError& error) {
        CHECK_CONTAINS(error.what(), text);
        return;
    }
    throw test::Failure("no ConfigError, expected: " + text);
}

}  // namespace

TEST(config_defaults) {
    Config config = parseConfig({"--package", "Serilog"});
    CHECK_EQ(config.package, std::string("Serilog"));
    CHECK_EQ(config.repository, DEFAULT_REPOSITORY);
    CHECK(!config.testMode);
    CHECK_EQ(config.output, DEFAULT_OUTPUT);
    CHECK(!config.asciiTree);
}

TEST(config_all_options) {
    TempDir dir;
    std::string repo = dir.write("repo.txt", "A:\n");
    Config config = parseConfig({"--package", "A", "--repo", repo, "--test-mode", "--output", "a.svg", "--ascii-tree"});
    CHECK_EQ(config.repository, repo);
    CHECK(config.testMode);
    CHECK_EQ(config.output, std::string("a.svg"));
    CHECK(config.asciiTree);
}

TEST(config_option_with_equals) {
    Config config = parseConfig({"--package=Serilog", "--output=deps.svg"});
    CHECK_EQ(config.package, std::string("Serilog"));
    CHECK_EQ(config.output, std::string("deps.svg"));
}

TEST(config_dotted_package_name) {
    Config config = parseConfig({"--package", "Microsoft.Extensions.Http"});
    CHECK_EQ(config.package, std::string("Microsoft.Extensions.Http"));
}

TEST(config_format_key_value) {
    Config config = parseConfig({"--package", "Serilog", "--ascii-tree"});
    std::vector<std::string> expected = {
        "package=Serilog", "repository=" + DEFAULT_REPOSITORY, "test_mode=false",
        "output=graph.svg", "ascii_tree=true", "load_order=false",
    };
    CHECK_EQ(formatConfig(config), expected);
}

TEST(config_load_order_flag) {
    CHECK(!parseConfig({"--package", "Serilog"}).loadOrder);
    CHECK(parseConfig({"--package", "Serilog", "--load-order"}).loadOrder);
}

TEST(config_error_missing_package) {
    checkError({}, "--package is required");
}

TEST(config_error_empty_package) {
    checkError({"--package", " "}, "empty");
}

TEST(config_error_invalid_package) {
    checkError({"--package", "bad!name"}, "invalid package name");
    checkError({"--package", "Serilog..Sinks"}, "invalid package name");
}

TEST(config_error_too_long_package) {
    checkError({"--package", std::string(101, 'A')}, "too long");
}

TEST(config_error_missing_value) {
    checkError({"--package"}, "missing value");
    checkError({"--package", "Serilog", "--repo"}, "missing value");
    checkError({"--package", "--ascii-tree"}, "missing value");
}

TEST(config_error_bad_repository_url) {
    checkError({"--package", "Serilog", "--repo", "ftp://x"}, "http(s) URL");
    checkError({"--package", "Serilog", "--repo", "https://"}, "http(s) URL");
}

TEST(config_error_unsafe_repository_url) {
    checkError({"--package", "Serilog", "--repo", "https://x.org/a\"b"}, "invalid characters");
}

TEST(config_error_test_mode_without_repo) {
    checkError({"--package", "A", "--test-mode"}, "requires --repo");
}

TEST(config_error_test_mode_missing_file) {
    checkError({"--package", "A", "--test-mode", "--repo", "nope.txt"}, "not found");
}

TEST(config_error_test_mode_lowercase_name) {
    TempDir dir;
    checkError({"--package", "abc", "--test-mode", "--repo", dir.write("r.txt", "")}, "capital latin");
}

TEST(config_error_output_extension) {
    checkError({"--package", "Serilog", "--output", "g.png"}, ".svg");
}

TEST(config_error_output_folder) {
    checkError({"--package", "Serilog", "--output", "nope/dir/g.svg"}, "folder does not exist");
}

TEST(config_error_flag_with_value) {
    checkError({"--package", "Serilog", "--ascii-tree=yes"}, "does not take a value");
    checkError({"--package", "Serilog", "--load-order=1"}, "does not take a value");
}

TEST(config_error_unknown_option) {
    checkError({"--package", "Serilog", "--color", "red"}, "unknown option: --color");
}

TEST(config_error_repeated_option) {
    checkError({"--package", "A", "--package", "B"}, "given twice");
}

TEST(config_error_unexpected_argument) {
    checkError({"Serilog"}, "unexpected argument");
}
