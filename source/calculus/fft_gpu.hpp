#ifndef FFT_GPU_HPP
#define FFT_GPU_HPP

    #include <d3d11.h>
    #include <wrl/client.h>
    #include "utils/config.hpp"

    // Calcule la FFT de l'océan sur le GPU. Les résultats restent dans des buffers GPU.
    class FFTGpu {
        public:
            bool init(ID3D11Device* device, FFTConfig f);
            // init se lance une fois au démarrage. device est « la carte graphique » : c'est lui qui crée les buffers et les shaders. 
            // FFTConfig f est passé par valeur (une copie), car initFFTSpectrum modifie son argument et que config est const dans runDirectX. 
            // Elle renvoie false en cas d'échec.

            // Évolution temporelle + FFT inverse 2D, pour trois champs (hauteur, dx, dz).
            void compute(ID3D11DeviceContext* context, float time);
            // context est la file de commandes envoyées au GPU (Dispatch, Map, …). compute lance toute la FFT pour l'instant time.

            // Branche les 3 résultats sur les slots t0, t1, t2 du vertex shader.
            void bindForVertexShader(ID3D11DeviceContext* context);

        private:
            struct Buffer {
                Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
            };
            // Un tableau GPU se décompose en trois objets. Le buffer est la mémoire, et les deux vues sont les « fenêtres » par lesquelles un shader l'ouvre.

            void setParams(ID3D11DeviceContext* context, float time, unsigned int horizontal);
            int N_ = 0;
            float size_ = 0.0f;
            Buffer h0_, ht_, dx_, dz_;
            Microsoft::WRL::ComPtr<ID3D11Buffer> params_;
            Microsoft::WRL::ComPtr<ID3D11ComputeShader> timeEvoShader_, fftShader_;
            // - N_, size_ : copies de f.N et f.size.
            // - h0_ : le spectre initial (jamais modifié après l'envoi).
            // - ht_, dx_, dz_ : les 3 champs (hauteur, déplacement x, déplacement z). Ils sont recalculés puis transformés par FFT.
            // - params_ : le petit buffer de constantes (temps, taille, direction).
            // - timeEvoShader_, fftShader_ : les deux programmes GPU compilés.
    };

#endif // FFT_GPU_HPP