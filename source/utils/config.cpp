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

    if (values.count("width")) {
        config.width = std::stoi(values["width"]);
    }
    if (values.count("height")) {
        config.height = std::stoi(values["height"]);
    }
    if (values.count("method")) {
        config.method = values["method"];
    }
    if (values.count("cellsnumber")) {
        config.cellsnumber = std::stoi(values["cellsnumber"]);
    }
    if (values.count("planesize")) {
        config.planeSize = std::stof(values["planesize"]);
    }
    if (values.count("nearplane")) {
        config.nearPlane = std::stod(values["nearplane"]);
    }
    if (values.count("farplane")) {
        config.farPlane = std::stod(values["farplane"]);
    }
    if (values.count("halfheightatnear")) {
        config.halfHeightAtNear = std::stod(values["halfheightatnear"]);
    }
    if (values.count("cameradistance")) {
        config.cameraDistance = std::stof(values["cameradistance"]);
    }
    if (values.count("tiltdegrees")) {
        config.tiltDegrees = std::stof(values["tiltdegrees"]);
    }
    return config;
}
