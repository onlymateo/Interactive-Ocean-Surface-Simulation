#include "calculus/gerstner_gpu.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

bool GerstnerGpu::init(ID3D11Device* device, const GerstnerConfig& g) {
    // le CPU parcourt g.k.size() vagues ; on prend le plus petit tableau par sécurité
    size_t count = std::min({g.k.size(), g.A.size(), g.w.size(), g.s.size(), g.D.size()});
    if (count > static_cast<size_t>(MAX_WAVES)) {
        std::cerr << "GPU Gerstner: " << count << " waves, only the first " << MAX_WAVES << " are used" << std::endl;
        count = MAX_WAVES;
    }

    params_ = {};
    params_.enabled = 1;
    params_.waveCount = static_cast<unsigned int>(count);
    for (size_t i = 0; i < count; i++) {
        params_.wave[i][0] = g.k[i];
        params_.wave[i][1] = g.A[i];
        params_.wave[i][2] = g.w[i];
        params_.wave[i][3] = g.s[i];
        if (g.D[i].size() >= 2) {
            params_.dir[i][0] = g.D[i][0];
            params_.dir[i][1] = g.D[i][1];
        }
    }

    // buffer dynamique : le CPU peut le réécrire à chaque mise à jour (pour le temps)
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(Params);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA data{&params_, 0, 0};
    return SUCCEEDED(device->CreateBuffer(&desc, &data, &buffer_));
}

void GerstnerGpu::update(ID3D11DeviceContext* context, float time) {
    params_.time = time;
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(context->Map(buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        std::memcpy(mapped.pData, &params_, sizeof(Params));
        context->Unmap(buffer_.Get(), 0);
    }
    // Map donne au CPU un pointeur (mapped.pData) vers la mémoire du buffer. WRITE_DISCARD dit « je réécris tout, jette l’ancien contenu ». 
    // C’est le mode normal pour un buffer dynamique, et c’est ce que vous faites déjà pour votre cameraBuffer.
    // On copie les 528 octets dans le buffer, puis Unmap rend la main au GPU. Entre Map et Unmap, le GPU ne peut pas utiliser le buffer.
}

void GerstnerGpu::bindForVertexShader(ID3D11DeviceContext* context) {
    context->VSSetConstantBuffers(3, 1, buffer_.GetAddressOf());
    // VS signifie vertex shader. On branche 1 buffer à partir du slot 3 (donc b3). 
    // GetAddressOf() donne l’adresse du pointeur dans le ComPtr, que D3D attend (un tableau de buffers de longueur 1).
}