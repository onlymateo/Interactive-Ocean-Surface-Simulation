#include "utils/config.hpp"

#include <fstream>
#include <map>

namespace {

std::map<std::string, std::string> readKeyValues(const std::string& path) {
    std::map<std::string, std::string> values;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::size_t sep = line.find('=');
        if (sep == std::string::npos) {
            continue;
        }
        values[line.substr(0, sep)] = line.substr(sep + 1);
    }
    return values;
}

}

Config loadConfig(const std::string& path) {
    std::map<std::string, std::string> values = readKeyValues(path);
    Config config;

    if (values.count("engine")) {
        config.engine = values["engine"];
    }
    if (values.count("width")) {
        config.width = std::stoi(values["width"]);
    }
    if (values.count("height")) {
        config.height = std::stoi(values["height"]);
    }
    if (values.count("method")) {
        config.method = values["method"];
    }
    return config;
}
