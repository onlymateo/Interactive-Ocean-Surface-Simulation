#include <iostream>
#include "calculus/calculus.hpp"
#include "engines/directx/directx.hpp"
#include "utils/config.hpp"

int main() {
    Config config = loadConfig("./.config");

    HeightFunction wave = findWaveMethod(config.method);
    if (!wave) {
        std::cerr << "Error: Unknown method '" << config.method << "' in .config (expected gerstner, perlin or fft)." << std::endl;
        return 1;
    }

    return runDirectX(config, wave);
}
