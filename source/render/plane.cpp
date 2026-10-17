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

    for (int row = 0; row < config.cellsnumber; ++row) {
        for (int col = 0; col < config.cellsnumber; ++col) {
            unsigned int topLeft = row * verticesPerRow + col;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = topLeft + verticesPerRow;
            unsigned int bottomRight = bottomLeft + 1;

            indices_.insert(indices_.end(), {
                topLeft, bottomLeft, bottomRight,
                topLeft, bottomRight, topRight,
            });
        }
    }

    base_ = vertices_;
}

void Plane::moveTo(float centerX, float centerY) {
    for (Vertex& vertex : base_) {
        vertex.x += centerX - centerX_;
        vertex.y += centerY - centerY_;
    }
    for (Vertex& vertex : vertices_) {
        vertex.x += centerX - centerX_;
        vertex.y += centerY - centerY_;
    }
    centerX_ = centerX;
    centerY_ = centerY;
}

void Plane::update(float time) {
    if (config.selectedmethod == "gerstner") {
        for (std::size_t i = 0; i < base_.size(); i++) {
            vertices_[i] = gerstnerWaveHeight(base_[i], time, config.g);
        }
    }
    if (config.selectedmethod == "fft") {
        const int verticesPerRow = config.cellsnumber + 1;
        const int N = config.f.N;
        // draw the initial spectrum once
        if (static_cast<int>(config.f.h0.size()) != N) {
            initFFTSpectrum(config.f);
        }

        for (int m = 0; m < N; m++) {
            for (int n = 0; n < N; n++) {
                fftTimeEvo(n, m, time, config.f);
            }
        }

        ifft2d(config.f.ht);
        ifft2d(config.f.ht_dx);
        ifft2d(config.f.ht_dz);

        for (int i = 0; i < verticesPerRow * verticesPerRow; i++) {
            int m = i / verticesPerRow; // row
            int n = i % verticesPerRow; // column


            // the last row/column wraps onto the first one (the surface tiles)
            int gm = m % N;
            int gn = n % N;

            // k is centred on N/2 but the inverse FFT expects k = 0 at index 0,
            // this shift shows up as a (-1)^(n+m) checkerboard that has to be undone
            double sign = ((gn + gm) % 2 == 0) ? 1.0 : -1.0;

            double h_spatial = sign * config.f.ht[gm][gn].real() * N * N;
            double dx_spatial = sign * config.f.ht_dx[gm][gn].real() * N * N;
            double dz_spatial = sign * config.f.ht_dz[gm][gn].real() * N * N;

            vertices_[i] = base_[i];
            vertices_[i].x += config.f.lambda * dx_spatial;
            vertices_[i].y += config.f.lambda * dz_spatial;
            vertices_[i].z += h_spatial;
        }
    }
}
