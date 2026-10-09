#include "render/plane.hpp"

Plane::Plane(Config config)
    : config(config) {
    buildMesh();
}

void Plane::buildMesh() {
    const int verticesPerRow = config.cellsnumber + 1;
    const float cellSize = config.planeSize / config.cellsnumber;

    for (int row = 0; row < verticesPerRow; ++row) {
        for (int col = 0; col < verticesPerRow; ++col) {
            vertices_.push_back({
                -config.planeSize / 2 + col * cellSize, // x, centred on the origin
                -config.planeSize / 2 + row * cellSize, // y, centred on the origin
                config.baseZpos,
                config.color.r,
                config.color.g,
                config.color.b,
            });
        }
    }

    // Each square is two triangles. Indices point into vertices_.
    for (int row = 0; row < config.cellsnumber; ++row) {
        for (int col = 0; col < config.cellsnumber; ++col) {
            unsigned int topLeft = row * verticesPerRow + col;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = topLeft + verticesPerRow;
            unsigned int bottomRight = bottomLeft + 1;
            unsigned int center = static_cast<unsigned int>(vertices_.size());

            vertices_.push_back({
                (vertices_[topLeft].x + vertices_[bottomRight].x) / 2.0f,
                (vertices_[topLeft].y + vertices_[bottomRight].y) / 2.0f,
                config.baseZpos,
                config.color.r,
                config.color.g,
                config.color.b,
            });

            indices_.insert(indices_.end(), {
                topLeft, bottomLeft, center,
                bottomLeft, bottomRight, center,
                bottomRight, topRight, center,
                topRight, topLeft, center,
            });
        }
    }
}

void Plane::update(float time) {
    for (Vertex& vertex : vertices_) {
        if (config.selectedmethod == "gerstner") {
            vertex.z = gerstnerWaveHeight(vertex.x, vertex.y, time, config.g);
        }
    }
}
