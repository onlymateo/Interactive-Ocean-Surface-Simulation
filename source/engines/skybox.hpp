#ifndef SKYBOX_HPP
#define SKYBOX_HPP

    #include <d3d11.h>
    #include <wrl/client.h>
    #include "engines/controller.hpp"
    #include "utils/config.hpp"

    class Skybox {
        public:
            Skybox(ID3D11Device* device, const Config& config);

            void draw(ID3D11DeviceContext* context, const CameraState& camera);

        private:
            SkyConfig sky_;
            float tanHalfWidth_;
            float tanHalfHeight_;
            Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
            Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
            Microsoft::WRL::ComPtr<ID3D11Buffer> buffer_;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilState> noDepth_;
    };

#endif // SKYBOX_HPP
