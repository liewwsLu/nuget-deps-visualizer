#include "app.h"

#include "config.h"

int runApp(const std::vector<std::string>& args, std::ostream& out, std::ostream& err) {
    try {
        Config config = parseConfig(args);
        for (const std::string& line : formatConfig(config)) {
            out << line << "\n";
        }
    } catch (const ConfigError& error) {
        err << "error: " << error.what() << "\n" << usage() << "\n";
        return 1;
    }
    return 0;
}
