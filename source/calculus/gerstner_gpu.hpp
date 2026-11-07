#ifndef GERSTNER_GPU_HPP
#define GERSTNER_GPU_HPP

    #include <d3d11.h>
    #include <wrl/client.h>
    #include "utils/config.hpp"

    // Envoie les paramètres des vagues de Gerstner au GPU. Le calcul se fait dans le vertex shader.
    class GerstnerGpu {
        public:
            static constexpr int MAX_WAVES = 16; // doit être égal à MAX_GERSTNER_WAVES dans le shader

            bool init(ID3D11Device* device, const GerstnerConfig& g);
            void update(ID3D11DeviceContext* context, float time); // met à jour le temps
            void bindForVertexShader(ID3D11DeviceContext* context); // cbuffer b3

        private:
            // Copie exacte du cbuffer "Gerstner" du vertex shader (528 octets)
            struct Params {
                unsigned int enabled;
                unsigned int waveCount;
                float time;
                float pad;
                float wave[MAX_WAVES][4]; // k, A, w, s
                float dir[MAX_WAVES][4];  // Dx, Dy, 0, 0
            };

            Params params_{};
            Microsoft::WRL::ComPtr<ID3D11Buffer> buffer_;
    };

#endif // GERSTNER_GPU_HPP