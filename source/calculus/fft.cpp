#include "calculus/calculus.hpp"

// Cooley-tukey algoryhtm
void fft1d(std::vector<Complex>& x, bool inverse = false) {
    int N = x.size();
    if (N <= 1) return;

    for (int i = 0, j = 0; i < N; i++) {
        if (i < j) std::swap(x[i], x[j]);
        int bit = N >> 1;
        for (; j & bit; bit >>= 1) j &= ~bit;
        j |= bit;
    }

    for (int len = 2; len <= N; len <<= 1) {
        double ang = 2 * PI / len * (inverse ? 1 : -1);
        Complex wlen(std::cos(ang), std::sin(ang));
        for (int i = 0; i < N; i += len) {
            Complex w(1, 0);
            for (int j = 0; j < len / 2; j++) {
                Complex u = x[i + j];
                Complex v = x[i + j + len / 2] * w;
                x[i + j] = u + v;
                x[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    if (inverse) {
        for (auto& val : x) val /= N;
    }
}

void ifft2d(std::vector<std::vector<Complex>>& grid) {
    int N = grid.size();
    for (int i = 0; i < N; i++) {
        fft1d(grid[i], true);
    }
    std::vector<Complex> col(N);
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) col[i] = grid[i][j];
        fft1d(col, true);
        for (int i = 0; i < N; i++) grid[i][j] = col[i];
    }
}



// Draws the initial spectrum h0 once, so that the waves keep the same shape from one frame to the next
void initFFTSpectrum(FFTConfig& f) {
    const int N = f.N;
    std::vector<Complex> row(N, 0.0);
    f.ht.assign(N, row);
    f.ht_dx.assign(N, row);
    f.ht_dz.assign(N, row);
    f.h0.assign(N, row);

    f.rng.seed(f.seed);

    for (int m = 0; m < N; m++) {
        for (int n = 0; n < N; n++) {
            double kx = 2.0 * PI * (n - N / 2) / f.size;
            double kz = 2.0 * PI * (m - N / 2) / f.size;
            double k_mag = std::sqrt(kx * kx + kz * kz);
            if (k_mag < 1e-4) k_mag = 1e-4;

            double k_dot_w = kx * f.wx + kz * f.wz;

            // phillips spectrum equation
            double phillips = f.A * (std::exp(-1.0 / std::pow(k_mag * f.L, 2)) / (std::pow(k_mag, 4))) * std::pow(k_dot_w / k_mag, 2);

            // supress waves travelling against the wind
            if (k_dot_w < 0) phillips *= 0.1;
            phillips = phillips * (std::exp(-k_mag * 0.0001));

            double xi_r = f.dist(f.rng);
            double xi_i = f.dist(f.rng);
            f.h0[m][n] = 1.0 / std::sqrt(2.0) * Complex(xi_r, xi_i) * std::sqrt(std::max(0.0, phillips));
        }
    }
}

void fftTimeEvo(int n, int m, float time, FFTConfig& f) {
    double kx = 2.0 * PI * (n - f.N / 2) / f.size;
    double kz = 2.0 * PI * (m - f.N / 2) / f.size;
    double k_mag = std::sqrt(kx * kx + kz * kz);
    if (k_mag < 1e-4) k_mag = 1e-4;

    // time evolution, the second term uses h0 at -k
    Complex h0 = f.h0[m][n];
    Complex h0_minus_k = f.h0[(f.N - m) % f.N][(f.N - n) % f.N];
    double omega = std::sqrt(g * k_mag);
    Complex c1 = std::polar(1.0, omega * time);
    Complex c2 = std::polar(1.0, -omega * time);

    // ht
    Complex h_val = h0 * c1 + std::conj(h0_minus_k) * c2;
    f.ht[m][n] = h_val;
    Complex factor_x = Complex(0, -1) * (kx / k_mag);
    Complex factor_z = Complex(0, -1) * (kz / k_mag);

    f.ht_dx[m][n] = h_val * factor_x;
    f.ht_dz[m][n] = h_val * factor_z;
}
