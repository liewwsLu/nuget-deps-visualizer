#include "nuspec.h"

#include <cctype>

#include "xml.h"

namespace {

const std::string ANY_FRAMEWORK;
const std::vector<std::string> PREFERRED_FRAMEWORKS = {"netstandard2.0", "netstandard2.1", ANY_FRAMEWORK};

std::string trim(const std::string& text) {
    std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

Dependency makeDependency(const XmlElement& element) {
    std::string id = trim(element.attribute("id"));
    if (id.empty()) {
        throw NuspecError("dependency without id");
    }
    return {id, trim(element.attribute("version"))};
}

std::vector<Dependency> dependenciesOf(const XmlElement& parent) {
    std::vector<Dependency> result;
    for (const XmlElement* element : parent.childrenNamed("dependency")) {
        result.push_back(makeDependency(*element));
    }
    return result;
}

}  // namespace

std::vector<DependencyGroup> parseDependencyGroups(const std::string& xmlText) {
    XmlElement root;
    try {
        root = parseXml(xmlText);
    } catch (const XmlError& error) {
        throw NuspecError(std::string("invalid nuspec XML: ") + error.what());
    }
    std::vector<DependencyGroup> groups;
    const XmlElement* dependencies = root.findDeep("dependencies");
    if (dependencies == nullptr) {
        return groups;
    }
    for (const XmlElement* group : dependencies->childrenNamed("group")) {
        groups.push_back({group->attribute("targetFramework", ANY_FRAMEWORK), dependenciesOf(*group)});
    }
    std::vector<Dependency> plain = dependenciesOf(*dependencies);
    if (!plain.empty()) {
        groups.push_back({ANY_FRAMEWORK, plain});
    }
    return groups;
}

std::string normalizeFramework(const std::string& name) {
    std::string result = trim(name);
    while (!result.empty() && result[0] == '.') {
        result.erase(0, 1);
    }
    for (char& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

DependencyGroup selectGroup(const std::vector<DependencyGroup>& groups) {
    if (groups.empty()) {
        return {ANY_FRAMEWORK, {}};
    }
    for (const std::string& preferred : PREFERRED_FRAMEWORKS) {
        for (const DependencyGroup& group : groups) {
            if (normalizeFramework(group.framework) == preferred) {
                return group;
            }
        }
    }
    return groups.front();
}
