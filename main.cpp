#include <iostream>
#include "calculus/calculus.hpp"
#include "engines/engine.hpp"
#include "utils/config.hpp"

// Read .config, then start the chosen engine with the chosen wave method.
int main() {
    Config config = loadConfig("./.config");

    const Engine* engine = findEngine(config.engine);
    if (!engine) {
        std::cerr << "Error: Unknown engine '" << config.engine << "' in .config (expected opengl)." << std::endl;
        return 1;
    }

    HeightFunction wave = findWaveMethod(config.method);
    if (!wave) {
        std::cerr << "Error: Unknown method '" << config.method << "' in .config (expected gerstner, perlin or fft)." << std::endl;
        return 1;
    }

    return engine->run(config.width, config.height, wave);
}
