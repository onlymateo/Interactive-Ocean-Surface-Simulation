#ifndef CALCULUS_HPP
#define CALCULUS_HPP

    #include <string>
    #include <cmath>
    #include <vector>


    using HeightFunction = float (*)(float x, float y, float time);

    HeightFunction findWaveMethod(const std::string& name);

    float gerstnerWaveHeight(float x, float y, float time);
    float perlinWaveHeight(float x, float y, float time);
    float fftWaveHeight(float x, float y, float time);

#endif // CALCULUS_HPP
