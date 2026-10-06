#ifndef RENDER_PLANE_HPP
#define RENDER_PLANE_HPP

#include <vector>
#include "calculus/calculus.hpp"

// A flat grid of triangles whose vertex heights are set every frame by a wave method.
class Plane {
public:
    Plane(float size, int cells, HeightFunction heightAt);

    // Move every vertex to the height given by the wave method at the given time.
    void update(float time);

    // Draw the filled triangles, then their edges on top.
    void draw() const;

private:
    struct Vertex {
        float x, y, z;
        float r, g, b;
    };

    void buildMesh();

    float size_;
    int cells_;
    HeightFunction heightAt_;
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
};

#endif // RENDER_PLANE_HPP
