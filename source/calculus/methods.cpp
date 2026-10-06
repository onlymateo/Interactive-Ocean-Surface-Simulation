#include "calculus/calculus.hpp"

namespace {

    struct WaveMethod {
        const char* name;
        HeightFunction height;
    };

    const WaveMethod METHODS[] = {
        {"gerstner", gerstnerWaveHeight},
        {"perlin", perlinWaveHeight},
        {"fft", fftWaveHeight},
    };

}

HeightFunction findWaveMethod(const std::string& name) {
    for (const WaveMethod& method : METHODS) {
        if (name == method.name) {
            return method.height;
        }
    }
    return nullptr;
}
