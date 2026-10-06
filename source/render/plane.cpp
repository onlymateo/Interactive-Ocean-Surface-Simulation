#include "render/plane.hpp"

#include <GLFW/glfw3.h>

Plane::Plane(float size, int cells, HeightFunction heightAt)
    : size_(size), cells_(cells), heightAt_(heightAt) {
    buildMesh();
}

// Create the vertices and triangles once. Heights are filled in later by update().
void Plane::buildMesh() {
    // A grid of cells x cells squares has (cells + 1) x (cells + 1) corner vertices.
    const int verticesPerRow = cells_ + 1;
    const float cellSize = size_ / cells_;

    for (int row = 0; row < verticesPerRow; ++row) {
        for (int col = 0; col < verticesPerRow; ++col) {
            // Position within the grid, from 0 to 1. Used to pick the colour.
            float u = static_cast<float>(col) / cells_;
            float v = static_cast<float>(row) / cells_;

            vertices_.push_back({
                -size_ / 2 + col * cellSize, // x, centred on the origin
                -size_ / 2 + row * cellSize, // y, centred on the origin
                0.0f,                        // z, set by update()
                0.1f + 0.2f * u,             // colour: red
                0.3f + 0.3f * v,             // colour: green
                0.6f + 0.4f * (1.0f - u),    // colour: blue
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

            indices_.insert(indices_.end(), {
                topLeft, bottomLeft, topRight,
                topRight, bottomLeft, bottomRight,
            });
        }
    }
}

void Plane::update(float time) {
    for (Vertex& vertex : vertices_) {
        vertex.z = heightAt_(vertex.x, vertex.y, time);
    }
}

void Plane::draw() const {
    const GLsizei indexCount = static_cast<GLsizei>(indices_.size());
    const GLsizei stride = sizeof(Vertex); // bytes from one vertex to the next

    // Coloured triangles
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, &vertices_[0].x);

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(3, GL_FLOAT, stride, &vertices_[0].r);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indices_.data());

    // Same triangles again, as black outlines, so each triangle is visible
    glDisableClientState(GL_COLOR_ARRAY);
    glColor3f(0.0f, 0.0f, 0.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indices_.data());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDisableClientState(GL_VERTEX_ARRAY);
}
