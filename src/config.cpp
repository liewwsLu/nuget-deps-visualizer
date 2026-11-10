#include "config.h"

#include <cctype>
#include <filesystem>
#include <set>

namespace {

const std::size_t MAX_PACKAGE_ID_LENGTH = 100;
const std::string OPTION_PREFIX = "--";
const std::string URL_CHARS = ":/._-~%?=&+#@,;";

/// Убрать пробелы в начале и в конце строки.
std::string trim(const std::string& text) {
    const std::string spaces = " \t\r\n";
    std::size_t begin = text.find_first_not_of(spaces);
    if (begin == std::string::npos) {
        return "";
    }
    std::size_t end = text.find_last_not_of(spaces);
    return text.substr(begin, end - begin + 1);
}

bool startsWith(const std::string& text, const std::string& prefix) {
    return text.compare(0, prefix.size(), prefix) == 0;
}

/// Имя тестового пакета: только большие латинские буквы.
bool isTestName(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    for (char c : name) {
        if (c < 'A' || c > 'Z') {
            return false;
        }
    }
    return true;
}

bool isIdChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

/// Имя пакета NuGet: части из букв, цифр и «_», разделённые «.» или «-».
bool isNuGetId(const std::string& name) {
    bool partStarted = false;
    for (char c : name) {
        if (isIdChar(c)) {
            partStarted = true;
        } else if ((c == '.' || c == '-') && partStarted) {
            partStarted = false;
        } else {
            return false;
        }
    }
    return partStarted;
}

/// В URL нет пробелов, кавычек и других символов, опасных для командной строки.
bool isSafeUrl(const std::string& url) {
    for (char c : url) {
        bool allowed = std::isalnum(static_cast<unsigned char>(c)) || URL_CHARS.find(c) != std::string::npos;
        if (!allowed) {
            return false;
        }
    }
    return true;
}

void validatePackage(const std::string& name, bool testMode) {
    if (name.empty()) {
        throw ConfigError("package name is empty");
    }
    if (testMode) {
        if (!isTestName(name)) {
            throw ConfigError("in test mode package name must be capital latin letters: " + name);
        }
        return;
    }
    if (name.size() > MAX_PACKAGE_ID_LENGTH) {
        throw ConfigError("package name is too long: " + name);
    }
    if (!isNuGetId(name)) {
        throw ConfigError("invalid package name: " + name);
    }
}

void validateRepository(const std::string& repository, bool testMode) {
    if (repository.empty()) {
        throw ConfigError("repository is empty");
    }
    if (testMode) {
        if (repository == DEFAULT_REPOSITORY) {
            throw ConfigError("test mode requires --repo with a path to a test file");
        }
        if (!std::filesystem::is_regular_file(repository)) {
            throw ConfigError("test repository file not found: " + repository);
        }
        return;
    }
    bool http = startsWith(repository, "http://") || startsWith(repository, "https://");
    std::size_t schemeEnd = repository.find("://");
    bool hasHost = schemeEnd != std::string::npos && repository.size() > schemeEnd + 3;
    if (!http || !hasHost) {
        throw ConfigError("repository must be an http(s) URL: " + repository);
    }
    if (!isSafeUrl(repository)) {
        throw ConfigError("repository URL contains invalid characters: " + repository);
    }
}

void validateOutput(const std::string& output) {
    const std::string extension = ".svg";
    std::string lower = output;
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    bool endsWithSvg = lower.size() > extension.size() &&
                       lower.compare(lower.size() - extension.size(), extension.size(), extension) == 0;
    if (!endsWithSvg) {
        throw ConfigError("output file must have .svg extension: " + output);
    }
    std::filesystem::path folder = std::filesystem::path(output).parent_path();
    if (!folder.empty() && !std::filesystem::is_directory(folder)) {
        throw ConfigError("output folder does not exist: " + folder.string());
    }
}

/// Значение параметра: следующий аргумент командной строки.
std::string takeValue(const std::vector<std::string>& args, std::size_t& index, const std::string& option) {
    if (index + 1 >= args.size() || startsWith(args[index + 1], OPTION_PREFIX)) {
        throw ConfigError("missing value for option " + option);
    }
    ++index;
    return args[index];
}

/// Применить параметр-флаг (без значения).
bool applyFlag(Config& config, const std::string& name) {
    if (name == "--test-mode") {
        config.testMode = true;
    } else if (name == "--ascii-tree") {
        config.asciiTree = true;
    } else if (name == "--load-order") {
        config.loadOrder = true;
    } else {
        return false;
    }
    return true;
}

/// Указатель на поле Config для параметра со значением или nullptr.
std::string* valueField(Config& config, const std::string& name) {
    if (name == "--package") {
        return &config.package;
    }
    if (name == "--repo") {
        return &config.repository;
    }
    if (name == "--output") {
        return &config.output;
    }
    return nullptr;
}

}  // namespace

Config parseConfig(const std::vector<std::string>& args) {
    Config config;
    std::set<std::string> seen;
    for (std::size_t i = 0; i < args.size(); ++i) {
        std::string name = args[i];
        std::string inlineValue;
        std::size_t equals = name.find('=');
        bool hasInline = startsWith(name, OPTION_PREFIX) && equals != std::string::npos;
        if (hasInline) {
            inlineValue = name.substr(equals + 1);
            name = name.substr(0, equals);
        }
        if (!startsWith(name, OPTION_PREFIX)) {
            throw ConfigError("unexpected argument: " + args[i]);
        }
        if (!seen.insert(name).second) {
            throw ConfigError("option " + name + " is given twice");
        }
        if (applyFlag(config, name)) {
            if (hasInline) {
                throw ConfigError("option " + name + " does not take a value");
            }
            continue;
        }
        std::string* field = valueField(config, name);
        if (field == nullptr) {
            throw ConfigError("unknown option: " + name);
        }
        *field = hasInline ? inlineValue : takeValue(args, i, name);
    }
    if (seen.count("--package") == 0) {
        throw ConfigError("option --package is required");
    }
    config.package = trim(config.package);
    config.repository = trim(config.repository);
    config.output = trim(config.output);
    validatePackage(config.package, config.testMode);
    validateRepository(config.repository, config.testMode);
    validateOutput(config.output);
    return config;
}

std::vector<std::string> formatConfig(const Config& config) {
    auto flag = [](bool value) { return std::string(value ? "true" : "false"); };
    return {
        "package=" + config.package,
        "repository=" + config.repository,
        "test_mode=" + flag(config.testMode),
        "output=" + config.output,
        "ascii_tree=" + flag(config.asciiTree),
        "load_order=" + flag(config.loadOrder),
    };
}

std::string usage() {
    return "usage: depviz --package NAME [--repo URL_OR_PATH] [--test-mode] [--output FILE.svg] [--ascii-tree] "
           "[--load-order]";
}
