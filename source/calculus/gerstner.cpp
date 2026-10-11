#include "calculus/calculus.hpp"


Vertex gerstnerWaveHeight(const Vertex& vertex, float time, const GerstnerConfig& g) {
    Vertex result = vertex;
    float phase = 0.0;

    for (int i = 0; i < g.k.size(); i++) {
        phase = g.k[i] * (g.D[i][0] * vertex.x + g.D[i][1] * vertex.y) - g.w[i] * time;
        result.x = result.x + (g.s[i] / g.k[i]) * g.D[i][0] * cos(phase);
        result.y = result.y + (g.s[i] / g.k[i]) * g.D[i][1] * cos(phase);
        result.z = result.z + (g.A[i] * sin(phase));
    }

    return (result);
}
