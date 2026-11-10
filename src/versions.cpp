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

bool VersionRange::contains(const std::string& version) const {
    VersionKey key = versionKey(version);
    if (!low.empty()) {
        VersionKey lowKey = versionKey(low);
        if (lowInclusive ? key < lowKey : key <= lowKey) {
            return false;
        }
    }
    if (!high.empty()) {
        VersionKey highKey = versionKey(high);
        if (highInclusive ? key > highKey : key >= highKey) {
            return false;
        }
    }
    return true;
}

VersionRange parseRange(const std::string& text) {
    std::string range = clean(text);
    VersionRange result;
    if (range.empty()) {
        return result;
    }
    if (range[0] != '[' && range[0] != '(') {
        result.low = range;
        return result;
    }
    char last = range.back();
    if (range.size() < 2 || (last != ']' && last != ')')) {
        throw VersionError("invalid version range: " + text);
    }
    std::string inner = range.substr(1, range.size() - 2);
    std::size_t comma = inner.find(',');
    if (comma == std::string::npos) {
        result.low = result.high = inner;
        return result;
    }
    result.low = inner.substr(0, comma);
    result.high = inner.substr(comma + 1);
    result.lowInclusive = range[0] == '[';
    result.highInclusive = last == ']';
    return result;
}

std::string lowestMatching(const std::string& rangeText, const std::vector<std::string>& versions) {
    VersionRange range = parseRange(rangeText);
    bool allowPrerelease = !range.low.empty() && isPrerelease(range.low);
    std::string best;
    for (const std::string& version : versions) {
        if (!range.contains(version) || (isPrerelease(version) && !allowPrerelease)) {
            continue;
        }
        if (best.empty() || versionKey(version) < versionKey(best)) {
            best = version;
        }
    }
    return best;
}
