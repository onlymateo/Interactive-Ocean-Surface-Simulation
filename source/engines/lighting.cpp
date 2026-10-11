#include "engines/lighting.hpp"

#include <cmath>
#include <cstring>

namespace {

// Same layout as the "Lighting" buffer in the shaders (3 numbers + 1 number = 16 bytes).
struct LightingData {
    float sunDirection[3]; // unit vector pointing towards the sun
    float sunIntensity;
    float sunColor[3];
    float ambient;
    float cameraPosition[3];
    float shininess;
};

}

Lighting::Lighting(ID3D11Device* device, const Config& config) : sun_(config.sun) {
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(LightingData);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&desc, nullptr, &buffer_);
}

void Lighting::setSun(float azimuthDegrees, float elevationDegrees) {
    sun_.azimuthDegrees = azimuthDegrees;
    sun_.elevationDegrees = elevationDegrees;
}

void Lighting::update(ID3D11DeviceContext* context, const CameraState& camera) {
    const float toRadians = 3.14159265358979f / 180.0f;
    float azimuth = sun_.azimuthDegrees * toRadians;
    float elevation = sun_.elevationDegrees * toRadians;

    LightingData data;
    data.sunDirection[0] = std::cos(elevation) * std::sin(azimuth); // x: right
    data.sunDirection[1] = std::cos(elevation) * std::cos(azimuth); // y: forward
    data.sunDirection[2] = std::sin(elevation);                     // z: up
    data.sunIntensity = sun_.intensity;
    data.sunColor[0] = sun_.color.r;
    data.sunColor[1] = sun_.color.g;
    data.sunColor[2] = sun_.color.b;
    data.ambient = sun_.ambient;
    data.cameraPosition[0] = camera.x;
    data.cameraPosition[1] = camera.y;
    data.cameraPosition[2] = camera.z;
    data.shininess = sun_.shininess;

    D3D11_MAPPED_SUBRESOURCE mapped;
    context->Map(buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    std::memcpy(mapped.pData, &data, sizeof(data));
    context->Unmap(buffer_.Get(), 0);

    context->PSSetConstantBuffers(1, 1, buffer_.GetAddressOf());
}
