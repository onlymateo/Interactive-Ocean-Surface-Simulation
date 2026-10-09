#include "utils/config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Config loadConfig(const std::string& path) {
    Config config;

    std::ifstream file(path);
    json root = json::parse(file, nullptr, false);
    if (root.is_discarded()) {
        std::cerr << "Warning: cannot read " << path << ", using default settings." << std::endl;
        return config;
    }

    const json window = root.value("window", json::object());
    config.width = window.value("width", config.width);
    config.height = window.value("height", config.height);

    const json plane = root.value("plane", json::object());
    config.cellsnumber = plane.value("cellsnumber", config.cellsnumber);
    config.planeSize = plane.value("planesize", config.planeSize);
    const json color = plane.value("color", json::object());
    config.color.r = color.value("r", config.color.r);
    config.color.g = color.value("g", config.color.g);
    config.color.b = color.value("b", config.color.b);
    config.baseZpos = plane.value("basezpos", config.baseZpos);

    const json camera = root.value("camera", json::object());
    config.nearPlane = camera.value("nearplane", config.nearPlane);
    config.farPlane = camera.value("farplane", config.farPlane);
    config.halfHeightAtNear = camera.value("halfheightatnear", config.halfHeightAtNear);
    config.cameraDistance = camera.value("cameradistance", config.cameraDistance);
    config.tiltDegrees = camera.value("tiltdegrees", config.tiltDegrees);

    config.calculusfrequency = root.value("calculusfrequency", config.calculusfrequency);
    config.selectedmethod = root.value("selectedmethod", config.selectedmethod);

    for (char& c : config.selectedmethod) {
        c = std::tolower(c);
    }
    bool found = false;
    for (const std::string& name : selectablemethods) {
        if (name == config.selectedmethod) {
            found = true;
        }
    }
    if (!found) {
        std::cerr << "Error: unknown method '" << config.selectedmethod << "'" << std::endl;
        std::exit(1);
    }

    for (const json& method : root.value("methods", json::array())) {
        if (method.value("name", "") == "gerstner") {
            config.g.k = method.value("k", config.g.k);
            config.g.A = method.value("A", config.g.A);
            config.g.w = method.value("w", config.g.w);
            config.g.D = method.value("D", config.g.D);
        }
    }
    return config;
}
