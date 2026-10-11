#include "engines/skybox.hpp"

#include <d3dcompiler.h>
#include <cstring>
#include <iostream>

using Microsoft::WRL::ComPtr;

namespace {

    const char* SKY_SHADER = R"(
    cbuffer Lighting : register(b1) {
        float3 sunDirection;
        float sunIntensity;
        float3 sunColor;
        float ambient;
        float3 cameraPosition;
        float shininess;
    };

    cbuffer Sky : register(b2) {
        row_major float4x4 cameraToWorld;
        float2 tanHalfSize;
        float2 padding;
        float4 zenithColor;
        float4 horizonColor;
    };

    struct SkyOut {
        float4 position : SV_POSITION;
        float2 screen : TEXCOORD0; // -1 to 1 on the screen
    };

    SkyOut vsSky(uint id : SV_VertexID) {
        float2 corner = float2((id << 1) & 2, id & 2); // (0,0) (2,0) (0,2)
        SkyOut output;
        output.screen = corner * 2.0 - 1.0;
        output.position = float4(output.screen, 1.0, 1.0);
        return output;
    }

    float4 psSky(SkyOut input) : SV_TARGET {
        // Direction of the pixel: the camera looks towards -z, then we turn it into the world
        float3 direction = float3(input.screen * tanHalfSize, -1.0);
        direction = normalize(mul(cameraToWorld, float4(direction, 0.0)).xyz);

        // Blue at the top, light at the horizon (z is up)
        float height = saturate(direction.z);
        float3 color = lerp(horizonColor.rgb, zenithColor.rgb, sqrt(height));

        // The sun: a small bright disc, with a soft glow around it
        float towardsSun = saturate(dot(direction, normalize(sunDirection)));
        float disc = pow(towardsSun, 2000.0);
        float glow = pow(towardsSun, 8.0) * 0.3;
        color += sunColor * sunIntensity * (disc + glow);

        return float4(color, 1.0);
    }
    )";

    struct SkyData {
        float cameraToWorld[16];
        float tanHalfSize[2];
        float padding[2];
        float zenith[4];
        float horizon[4];
    };

    ComPtr<ID3DBlob> compile(const char* entry, const char* target) {
        ComPtr<ID3DBlob> code, errors;
        D3DCompile(SKY_SHADER, std::strlen(SKY_SHADER), nullptr, nullptr, nullptr,
                entry, target, 0, 0, &code, &errors);
        if (errors) {
            std::cerr << static_cast<const char*>(errors->GetBufferPointer()) << std::endl;
        }
        return code;
    }

}

Skybox::Skybox(ID3D11Device* device, const Config& config) : sky_(config.sky) {
    float aspect = static_cast<float>(config.width) / static_cast<float>(config.height);
    tanHalfHeight_ = static_cast<float>(config.halfHeightAtNear / config.nearPlane);
    tanHalfWidth_ = tanHalfHeight_ * aspect;

    ComPtr<ID3DBlob> vsCode = compile("vsSky", "vs_4_0");
    ComPtr<ID3DBlob> psCode = compile("psSky", "ps_4_0");
    if (!vsCode || !psCode) {
        return;
    }
    device->CreateVertexShader(vsCode->GetBufferPointer(), vsCode->GetBufferSize(), nullptr, &vertexShader_);
    device->CreatePixelShader(psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &pixelShader_);

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(SkyData);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&desc, nullptr, &buffer_);

    D3D11_DEPTH_STENCIL_DESC depthDesc{};
    depthDesc.DepthEnable = FALSE;
    device->CreateDepthStencilState(&depthDesc, &noDepth_);
}

void Skybox::draw(ID3D11DeviceContext* context, const CameraState& camera) {
    if (!vertexShader_ || !pixelShader_) {
        return;
    }

    float rotation[3][3];
    cameraRotation(camera, rotation);

    SkyData data{};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            data.cameraToWorld[row * 4 + col] = rotation[col][row];
        }
    }
    data.cameraToWorld[15] = 1.0f;
    data.tanHalfSize[0] = tanHalfWidth_;
    data.tanHalfSize[1] = tanHalfHeight_;
    data.zenith[0] = sky_.zenith.r;
    data.zenith[1] = sky_.zenith.g;
    data.zenith[2] = sky_.zenith.b;
    data.horizon[0] = sky_.horizon.r;
    data.horizon[1] = sky_.horizon.g;
    data.horizon[2] = sky_.horizon.b;

    D3D11_MAPPED_SUBRESOURCE mapped;
    context->Map(buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, &data, sizeof(data));
    context->Unmap(buffer_.Get(), 0);

    context->IASetInputLayout(nullptr);
    context->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context->PSSetShader(pixelShader_.Get(), nullptr, 0);
    context->PSSetConstantBuffers(2, 1, buffer_.GetAddressOf());
    context->OMSetDepthStencilState(noDepth_.Get(), 0);

    context->Draw(3, 0);

    context->OMSetDepthStencilState(nullptr, 0);
}
