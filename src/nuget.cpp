#include "nuget.h"

#include <cctype>
#include <utility>

#include "json.h"
#include "versions.h"

namespace {

const std::string BASE_ADDRESS_TYPE = "PackageBaseAddress/3.0.0";
const int HTTP_OK = 200;
const int HTTP_NOT_FOUND = 404;
const std::string UTF8_BOM = "\xEF\xBB\xBF";

std::string toLower(std::string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/// Адрес с ровно одной «/» в конце.
std::string withSlash(std::string url) {
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    return url + "/";
}

/// Разобрать JSON-ответ сервера.
JsonValue loadJson(const std::string& text, const std::string& url) {
    try {
        return parseJson(text);
    } catch (const JsonError&) {
        throw RepositoryError("invalid JSON from " + url);
    }
}

}  // namespace

NuGetRepository::NuGetRepository(std::string url, Fetcher& fetcher) : url_(std::move(url)), fetcher_(fetcher) {}

std::string NuGetRepository::fetchText(const std::string& url, bool& found) {
    HttpResponse response = fetcher_.get(url);
    found = response.status != HTTP_NOT_FOUND;
    if (!found) {
        return "";
    }
    if (response.status != HTTP_OK) {
        throw RepositoryError("HTTP " + std::to_string(response.status) + " for " + url);
    }
    std::string body = response.body;
    if (body.compare(0, UTF8_BOM.size(), UTF8_BOM) == 0) {
        body.erase(0, UTF8_BOM.size());
    }
    return body;
}

std::string NuGetRepository::baseAddress() {
    if (!base_.empty()) {
        return base_;
    }
    if (!endsWith(url_, ".json")) {
        base_ = withSlash(url_);
        return base_;
    }
    bool found = false;
    std::string text = fetchText(url_, found);
    if (!found) {
        throw RepositoryError("service index not found: " + url_);
    }
    JsonValue index = loadJson(text, url_);
    const JsonValue* resources = index.find("resources");
    if (resources != nullptr) {
        for (const JsonValue& resource : resources->items) {
            const JsonValue* type = resource.find("@type");
            const JsonValue* id = resource.find("@id");
            if (type != nullptr && id != nullptr && type->text == BASE_ADDRESS_TYPE) {
                base_ = withSlash(id->text);
                return base_;
            }
        }
    }
    throw RepositoryError("service index has no " + BASE_ADDRESS_TYPE + " resource");
}

std::vector<std::string> NuGetRepository::versions(const std::string& packageId) {
    std::string url = baseAddress() + toLower(packageId) + "/index.json";
    bool found = false;
    std::string text = fetchText(url, found);
    if (!found) {
        throw RepositoryError("package not found: " + packageId);
    }
    std::vector<std::string> result;
    JsonValue index = loadJson(text, url);
    const JsonValue* list = index.find("versions");
    if (list != nullptr) {
        for (const JsonValue& item : list->items) {
            result.push_back(item.text);
        }
    }
    return result;
}

std::string NuGetRepository::nuspec(const std::string& packageId, const std::string& version) {
    std::string name = toLower(packageId);
    std::string url = baseAddress() + name + "/" + toLower(version) + "/" + name + ".nuspec";
    bool found = false;
    std::string text = fetchText(url, found);
    if (!found) {
        throw RepositoryError("version " + version + " of " + packageId + " not found");
    }
    return text;
}

DirectDependencies NuGetRepository::directDependencies(const std::string& packageId, const std::string& version) {
    DirectDependencies result;
    result.version = version;
    if (result.version.empty()) {
        std::vector<std::string> available = versions(packageId);
        if (available.empty()) {
            throw RepositoryError("package has no versions: " + packageId);
        }
        result.version = latestStable(available);
    }
    DependencyGroup group = selectGroup(parseDependencyGroups(nuspec(packageId, result.version)));
    result.framework = group.framework;
    result.dependencies = group.dependencies;
    return result;
}

NuGetSource::NuGetSource(NuGetRepository& repository, const std::string& root) : repository_(repository) {
    names_[toLower(root)] = root;
}

void NuGetSource::useVersion(const std::string& name, const std::string& version) {
    versions_[toLower(name)] = version;
}

std::string NuGetSource::resolve(const std::string& name) {
    std::string key = toLower(name);
    auto known = versions_.find(key);
    if (known != versions_.end()) {
        return known->second;
    }
    std::vector<std::string> available = repository_.versions(name);
    auto range = ranges_.find(key);
    std::string version;
    if (range == ranges_.end()) {
        version = available.empty() ? "" : latestStable(available);
    } else {
        version = lowestMatching(range->second, available);
    }
    if (version.empty()) {
        std::string wanted = range == ranges_.end() || range->second.empty() ? "latest" : range->second;
        throw RepositoryError("no suitable version of " + name + " for " + wanted);
    }
    versions_[key] = version;
    return version;
}

std::vector<std::string> NuGetSource::dependencies(const std::string& name) {
    std::string version = resolve(name);
    std::vector<std::string> result;
    for (const Dependency& dependency : repository_.directDependencies(name, version).dependencies) {
        std::string key = toLower(dependency.packageId);
        ranges_.emplace(key, dependency.versionRange);
        names_.emplace(key, dependency.packageId);
        result.push_back(names_[key]);
    }
    return result;
}

std::string NuGetSource::label(const std::string& name) {
    return name + " " + versions_[toLower(name)];
}
