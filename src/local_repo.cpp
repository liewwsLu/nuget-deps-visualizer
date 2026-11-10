#include "local_repo.h"

#include <fstream>
#include <sstream>
#include <utility>

namespace {

std::string trim(const std::string& text) {
    std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

/// Имя пакета — одна или несколько больших латинских букв.
bool isValidName(const std::string& name) {
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

void checkName(const std::string& name, int lineNumber) {
    if (!isValidName(name)) {
        throw LocalRepositoryError("line " + std::to_string(lineNumber) + ": invalid package name '" + name + "'");
    }
}

/// Разобрать строку «A: B C».
std::pair<std::string, std::vector<std::string>> parseLine(const std::string& line, int lineNumber) {
    std::size_t colon = line.find(':');
    if (colon == std::string::npos) {
        throw LocalRepositoryError("line " + std::to_string(lineNumber) + ": expected 'NAME: DEPENDENCIES'");
    }
    std::string name = trim(line.substr(0, colon));
    checkName(name, lineNumber);
    std::vector<std::string> dependencies;
    std::istringstream rest(line.substr(colon + 1));
    std::string dependency;
    while (rest >> dependency) {
        checkName(dependency, lineNumber);
        dependencies.push_back(dependency);
    }
    return {name, dependencies};
}

}  // namespace

PackageMap parseLocalRepository(const std::string& text) {
    PackageMap packages;
    std::istringstream input(text);
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = trim(line.substr(0, line.find('#')));
        if (line.empty()) {
            continue;
        }
        auto [name, dependencies] = parseLine(line, lineNumber);
        if (packages.count(name) != 0) {
            throw LocalRepositoryError("line " + std::to_string(lineNumber) + ": package " + name +
                                       " described twice");
        }
        packages[name] = dependencies;
    }
    return packages;
}

PackageMap loadLocalRepository(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw LocalRepositoryError("cannot read " + path);
    }
    std::ostringstream text;
    text << file.rdbuf();
    return parseLocalRepository(text.str());
}

LocalRepository::LocalRepository(PackageMap packages, const std::string& root) : packages_(std::move(packages)) {
    if (packages_.count(root) == 0) {
        throw LocalRepositoryError("package not found in test repository: " + root);
    }
}

std::vector<std::string> LocalRepository::dependencies(const std::string& name) {
    auto found = packages_.find(name);
    return found == packages_.end() ? std::vector<std::string>{} : found->second;
}

std::string LocalRepository::label(const std::string& name) {
    return name;
}
