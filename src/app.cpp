#include "app.h"

#include "config.h"
#include "nuget.h"
#include "nuspec.h"
#include "versions.h"

namespace {

/// Получить и вывести прямые зависимости пакета (этап 2).
void printDirectDependencies(const Config& config, Fetcher& fetcher, std::ostream& out) {
    NuGetRepository repository(config.repository, fetcher);
    DirectDependencies direct = repository.directDependencies(config.package);
    out << "Package: " << config.package << " " << direct.version << "\n";
    out << "Framework: " << (direct.framework.empty() ? "any" : direct.framework) << "\n";
    out << "Direct dependencies:\n";
    if (direct.dependencies.empty()) {
        out << "  (none)\n";
    }
    for (const Dependency& dependency : direct.dependencies) {
        out << "  " << dependency.packageId << " " << describeRange(dependency.versionRange) << "\n";
    }
}

}  // namespace

int runApp(const std::vector<std::string>& args, Fetcher& fetcher, std::ostream& out, std::ostream& err) {
    try {
        Config config = parseConfig(args);
        if (config.testMode) {
            throw ConfigError("test repository mode is not supported yet");
        }
        printDirectDependencies(config, fetcher, out);
    } catch (const ConfigError& error) {
        err << "error: " << error.what() << "\n" << usage() << "\n";
        return 1;
    } catch (const RepositoryError& error) {
        err << "error: " << error.what() << "\n";
        return 1;
    } catch (const NuspecError& error) {
        err << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
