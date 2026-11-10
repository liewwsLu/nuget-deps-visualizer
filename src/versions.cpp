#include "versions.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {

const std::size_t VERSION_PARTS = 4;

/// Убрать пробелы и метаданные сборки (+...).
std::string clean(const std::string& version) {
    std::string result;
    for (char c : version) {
        if (c == '+') {
            break;
        }
        if (!std::isspace(static_cast<unsigned char>(c))) {
            result += c;
        }
    }
    return result;
}

/// Числовая часть версии и метка пререлиза.
std::pair<std::string, std::string> split(const std::string& version) {
    std::string text = clean(version);
    std::size_t dash = text.find('-');
    if (dash == std::string::npos) {
        return {text, ""};
    }
    return {text.substr(0, dash), text.substr(dash + 1)};
}

bool isNumber(const std::string& text) {
    return !text.empty() && std::all_of(text.begin(), text.end(),
                                        [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
}

}  // namespace

VersionKey versionKey(const std::string& version) {
    auto [main, label] = split(version);
    std::vector<long long> numbers;
    std::size_t start = 0;
    while (start <= main.size()) {
        std::size_t dot = main.find('.', start);
        std::string part = main.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        numbers.push_back(isNumber(part) ? std::stoll(part) : 0);
        if (dot == std::string::npos) {
            break;
        }
        start = dot + 1;
    }
    while (numbers.size() < VERSION_PARTS) {
        numbers.push_back(0);
    }
    for (char& c : label) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return {numbers, label.empty(), label};
}

bool isPrerelease(const std::string& version) {
    return !split(version).second.empty();
}

std::string latestStable(const std::vector<std::string>& versions) {
    std::vector<std::string> pool;
    for (const std::string& version : versions) {
        if (!isPrerelease(version)) {
            pool.push_back(version);
        }
    }
    if (pool.empty()) {
        pool = versions;
    }
    if (pool.empty()) {
        throw std::invalid_argument("no versions available");
    }
    return *std::max_element(pool.begin(), pool.end(), [](const std::string& a, const std::string& b) {
        return versionKey(a) < versionKey(b);
    });
}

std::string describeRange(const std::string& range) {
    std::string text = clean(range);
    if (text.empty()) {
        return "any version";
    }
    if (text[0] == '[' || text[0] == '(') {
        return range;
    }
    return ">= " + text;
}
