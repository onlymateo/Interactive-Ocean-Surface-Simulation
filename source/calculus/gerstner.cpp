#include "calculus/calculus.hpp"


float gerstnerWaveHeight(float x, float y, float time, GerstnerConfig g) {
    float z = 0.0;

    for (int i = 0; i < g.k.size(); i++) {
        z = z + (g.A[i] * sin(g.k[i] * (g.D[i][0] * x + g.D[i][1] * y) - g.w[i] * time));
    }

    return (z);
} 
