#ifndef UTILS_CONFIG_HPP
#define UTILS_CONFIG_HPP

#include <string>

// Settings read from the .config file. The defaults are used for any key that is missing.
struct Config {
    std::string engine = "opengl";  // which window backend to use
    int width = 800;                // window width in pixels
    int height = 600;               // window height in pixels
    std::string method = "gerstner"; // wave method: "gerstner", "perlin" or "fft"
};

// Read a "key=value" file. Lines starting with '#' are comments.
Config loadConfig(const std::string& path);

#endif // UTILS_CONFIG_HPP
