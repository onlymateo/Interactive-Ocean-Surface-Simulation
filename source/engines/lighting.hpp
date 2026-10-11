#ifndef LIGHTING_HPP
#define LIGHTING_HPP

    #include <d3d11.h>
    #include <wrl/client.h>
    #include "engines/controller.hpp"
    #include "utils/config.hpp"

    class Lighting {
        public:
            Lighting(ID3D11Device* device, const Config& config);

            void setSun(float azimuthDegrees, float elevationDegrees);

            void update(ID3D11DeviceContext* context, const CameraState& camera);

        private:
            SunConfig sun_;
            Microsoft::WRL::ComPtr<ID3D11Buffer> buffer_;
    };

#endif // LIGHTING_HPP
