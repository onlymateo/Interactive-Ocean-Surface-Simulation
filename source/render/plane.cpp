#include "render/plane.hpp"

Plane::Plane(float size, int cells, HeightFunction heightAt)
    : size_(size), cells_(cells), heightAt_(heightAt) {
    buildMesh();
}

// Create the vertices and triangles once. Heights are filled in later by update().
void Plane::buildMesh() {
    const int verticesPerRow = cells_ + 1;
    const float cellSize = size_ / cells_;

    for (int row = 0; row < verticesPerRow; ++row) {
        for (int col = 0; col < verticesPerRow; ++col) {
            vertices_.push_back({
                -size_ / 2 + col * cellSize, // x, centred on the origin
                -size_ / 2 + row * cellSize, // y, centred on the origin
                0.0f,                        // z, set by update()
                0.1f,                        // colour: red
                0.4f,                        // colour: green
                0.8f,                        // colour: blue
            });
        }
    }

    // Each square is two triangles. Indices point into vertices_.
    for (int row = 0; row < cells_; ++row) {
        for (int col = 0; col < cells_; ++col) {
            unsigned int topLeft = row * verticesPerRow + col;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = topLeft + verticesPerRow;
            unsigned int bottomRight = bottomLeft + 1;
            unsigned int center = static_cast<unsigned int>(vertices_.size());

            vertices_.push_back({
                (vertices_[topLeft].x + vertices_[bottomRight].x) / 2.0f,
                (vertices_[topLeft].y + vertices_[bottomRight].y) / 2.0f,
                0.0f,
                0.1f,
                0.4f,
                0.8f,
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
        vertex.z = heightAt_(vertex.x, vertex.y, time);
    }
}
