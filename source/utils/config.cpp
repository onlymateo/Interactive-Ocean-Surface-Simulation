#include "utils/config.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Reads {"r": 0.1, "g": 0.5, "b": 0.8} from parent[key]. Missing values stay as they are.
void readColor(const json& parent, const char* key, RGBcolor& color) {
    const json value = parent.value(key, json::object());
    color.r = value.value("r", color.r);
    color.g = value.value("g", color.g);
    color.b = value.value("b", color.b);
}

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
    config.chunksNumber = plane.value("chunksnumber", config.chunksNumber);
    config.showtriangles = plane.value("showtriangles", config.showtriangles);


    const json camera = root.value("camera", json::object());
    config.nearPlane = camera.value("nearplane", config.nearPlane);
    config.farPlane = camera.value("farplane", config.farPlane);
    config.halfHeightAtNear = camera.value("halfheightatnear", config.halfHeightAtNear);
    config.cameraDistance = camera.value("cameradistance", config.cameraDistance);
    config.tiltDegrees = camera.value("tiltdegrees", config.tiltDegrees);

    const json sun = root.value("sun", json::object());
    config.sun.azimuthDegrees = sun.value("azimuth", config.sun.azimuthDegrees);
    config.sun.elevationDegrees = sun.value("elevation", config.sun.elevationDegrees);
    readColor(sun, "color", config.sun.color);
    config.sun.intensity = sun.value("intensity", config.sun.intensity);
    config.sun.ambient = sun.value("ambient", config.sun.ambient);
    config.sun.shininess = sun.value("shininess", config.sun.shininess);

    const json sky = root.value("sky", json::object());
    readColor(sky, "zenith", config.sky.zenith);
    readColor(sky, "horizon", config.sky.horizon);

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
            config.g.s = method.value("s", config.g.s);

            // The steepness only makes sense with unit directions
            for (std::vector<float>& direction : config.g.D) {
                float length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1]);
                if (length > 0.0f) {
                    direction[0] /= length;
                    direction[1] /= length;
                }
            }
        }
    }
    return config;
}
