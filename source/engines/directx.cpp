#include "engines/directx.hpp"
#include "engines/controller.hpp"
#include "engines/chunk.hpp"
#include "engines/lighting.hpp"
#include "engines/skybox.hpp"
#include "calculus/calculus.hpp"
#include "calculus/fft_gpu.hpp"
#include "calculus/gerstner_gpu.hpp"
#include "render/plane.hpp"


#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <complex>

using Microsoft::WRL::ComPtr;

namespace {

// Vertex shader: place the point on screen. Pixel shader: use the colour of the vertex.
const char* SHADER_SOURCE = R"(
    cbuffer Camera : register(b0) {
        row_major float4x4 viewProjection;
    };

    cbuffer Lighting : register(b1) {
        float3 sunDirection;
        float sunIntensity;
        float3 sunColor;
        float ambient;
        float3 cameraPosition;
        float shininess;
    };

    cbuffer GpuWave : register(b2) {
        uint useGpuWave;
        uint gridN;
        uint verticesPerChunk;
        uint verticesPerRow;
        float lambda;
        float3 wavePad;
    };
    // Les paramètres du déplacement, envoyés par le CPU :
    //    useGpuWave : 1 si le mode GPU est actif, 0 sinon. Cela permet de garder le même shader pour Gerstner et pour la FFT CPU.
    //    gridN : f.N.
    //    verticesPerChunk : le nombre de sommets d’un chunk ((cellsnumber+1)²).
    //    verticesPerRow : cellsnumber + 1.
    //    lambda : l’intensité du déplacement horizontal (« chopiness »).
    //    wavePad : du remplissage jusqu’à 32 octets.


    StructuredBuffer<float2> waveHeight : register(t0);
    StructuredBuffer<float2> waveDx : register(t1);
    StructuredBuffer<float2> waveDz : register(t2);
    // Les trois résultats de la FFT, en lecture seule. Ce sont les ht, dx, dz branchés par bindForVertexShader.


    #define MAX_GERSTNER_WAVES 16

    cbuffer Gerstner : register(b3) {
        uint useGerstner; // 1 si Gerstner GPU est actif, 0 sinon
        uint gerstnerWaveCount; // le nombre de vagues. C’est l’équivalent de g.k.size().
        float gerstnerTime; // le temps de l’animation (le time de gerstnerWaveHeight). C’est l’équivalent de g.k.size().
        float gerstnerPad; // du remplissage. Les 4 valeurs ci-dessus (uint, uint, float, float) font exactement 16 octets, la taille d’une ligne de cbuffer.
        float4 gerstnerWave[MAX_GERSTNER_WAVES];
        float4 gerstnerDir[MAX_GERSTNER_WAVES];
        // Deux tableaux de 16 float4. On regroupe les paramètres d’une vague dans un seul float4 :
        // gerstnerWave[i] = (k, A, w, s) : nombre d’onde, amplitude, pulsation, raideur.
        // gerstnerDir[i] = (Dx, Dy, 0, 0) : direction de la vague. Les deux dernières composantes sont inutilisées.
        // Pourquoi des float4 ? Dans un cbuffer, chaque élément d’un tableau est aligné sur 16 octets. 
        // Un float k[16] gaspillerait 12 octets par élément et ne correspondrait plus à la struct C++. Avec des float4, il n’y a aucun trou.
    };
    // Un cbuffer est un paquet de valeurs envoyées par le CPU, identiques pour tous les sommets. register(b3) est son emplacement. 
    // b0 est la caméra, b1 la lumière et b2 la FFT (ou le skybox), donc b3 est libre. C’est le numéro du VSSetConstantBuffers(3, ...) dans le C++.


    struct VertexOut {
        float4 position : SV_POSITION;
        float3 color : COLOR;
        float3 worldPosition : TEXCOORD0;
    };


    VertexOut vsMain(float3 position : POSITION, float3 color : COLOR, uint vertexId : SV_VertexID) {
        // SV_VertexID est le numéro du sommet en cours de traitement. Le GPU le fournit tout seul, il n’a pas besoin d’être dans votre layout. 
        // Avec DrawIndexed(..., baseVertex), ce numéro inclut le baseVertex, donc il vaut numéro du chunk × verticesPerChunk + indice local.


        if (useGpuWave != 0) {
            uint local = vertexId % verticesPerChunk; // Retire le décalage du chunk pour obtenir l’indice du sommet dans son chunk.

            uint row = (local / verticesPerRow) % gridN;   // la dernière ligne/colonne reboucle sur la première
            uint col = (local % verticesPerRow) % gridN;
            uint i = row * gridN + col; // L’indice dans les buffers de la FFT (rangés à plat, comme avant).

            float sign = ((row + col) & 1) ? -1.0 : 1.0;
            // Le damier (−1)^(row+col). Votre spectre est centré sur N/2 alors que la FFT attend k = 0 à l’indice 0. 
            // Ce décalage se traduit dans l’espace réel par un signe alterné, qu’on annule ici. C’est exactement sign de plane.cpp:93.

        
            position.x += lambda * sign * waveDx[i].x;
            position.y += lambda * sign * waveDz[i].x;
            position.z += sign * waveHeight[i].x;
            // On déplace le sommet. Seule la partie réelle (.x) compte, la partie imaginaire est ~0 après la FFT inverse. 
            // L’axe z est la hauteur, et x/y le déplacement horizontal, comme dans votre plane.cpp.
        }
        if (useGerstner != 0) {
            float2 rest = position.xy; // la phase utilise la position au repos, comme gerstner.cpp
            for (uint n = 0; n < gerstnerWaveCount; n++) {
                float4 wv = gerstnerWave[n];
                float2 d = gerstnerDir[n].xy;
                // On lit les paramètres de la vague n :
                //    wv.x = k, wv.y = A, wv.z = w, wv.w = s.
                //    d = (Dx, Dy).

                float phase = wv.x * dot(d, rest) - wv.z * gerstnerTime;
                position.xy += (wv.w / wv.x) * d * cos(phase);
                position.z += wv.y * sin(phase);
            }
        }

        VertexOut output;
        output.position = mul(viewProjection, float4(position, 1.0));
        output.color = color;
        output.worldPosition = position;
        return output;
    }

    float4 psMain(VertexOut input) : SV_TARGET {
        // The direction the surface faces, found from the position of the neighbour pixels
        float3 normal = normalize(cross(ddx(input.worldPosition), ddy(input.worldPosition)));
        if (normal.z < 0.0) {
            normal = -normal; // the water always faces up
        }

        float3 toSun = normalize(sunDirection);
        float3 toCamera = normalize(cameraPosition - input.worldPosition);
        float3 sunLight = sunColor * sunIntensity;

        // Diffuse: the surface is brighter when it faces the sun
        float diffuse = saturate(dot(normal, toSun));

        // Specular: the shine of the sun on the water, when the reflection goes to the camera
        float3 halfway = normalize(toSun + toCamera);
        float specular = pow(saturate(dot(normal, halfway)), shininess);

        float3 color = input.color * (ambient + diffuse * sunLight) + specular * sunLight;
        return float4(normal * 0.5 + 0.5, 1.0);
        //return float4(color, 1.0);
    }

    float4 psOutline() : SV_TARGET {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
)";

ComPtr<ID3DBlob> compileShader(const char* entry, const char* target) {
    ComPtr<ID3DBlob> code, errors;
    D3DCompile(SHADER_SOURCE, std::strlen(SHADER_SOURCE), nullptr, nullptr, nullptr,
               entry, target, 0, 0, &code, &errors);
    if (errors) {
        std::cerr << static_cast<const char*>(errors->GetBufferPointer()) << std::endl;
    }
    return code;
}


void cameraMatrix(const Config& config, const CameraState& camera, float out[16]) {
    float n = static_cast<float>(config.nearPlane);
    float f = static_cast<float>(config.farPlane);
    float halfHeight = static_cast<float>(config.halfHeightAtNear);
    float halfWidth = halfHeight * static_cast<float>(config.width) / static_cast<float>(config.height);

    float rotation[3][3];
    cameraRotation(camera, rotation);
    float shift[3];
    for (int row = 0; row < 3; ++row) {
        shift[row] = -(rotation[row][0] * camera.x + rotation[row][1] * camera.y + rotation[row][2] * camera.z);
    }

    const float projection[16] = {
        n / halfWidth, 0.0f,           0.0f,        0.0f,
        0.0f,          n / halfHeight, 0.0f,        0.0f,
        0.0f,          0.0f,           f / (n - f), n * f / (n - f),
        0.0f,          0.0f,           -1.0f,       0.0f,
    };
    const float view[16] = {
        rotation[0][0], rotation[0][1], rotation[0][2], shift[0],
        rotation[1][0], rotation[1][1], rotation[1][2], shift[1],
        rotation[2][0], rotation[2][1], rotation[2][2], shift[2],
        0.0f,           0.0f,           0.0f,           1.0f,
    };

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[row * 4 + col] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                out[row * 4 + col] += projection[row * 4 + k] * view[k * 4 + col];
            }
        }
    }
}

}

int runDirectX(const Config& config) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    // GLFW only provides the window; Direct3D draws into it.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(config.width, config.height, "DirectX Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    // Device (the GPU), context (its commands) and swap chain (the images shown in the window).
    DXGI_SWAP_CHAIN_DESC swapDesc{};
    swapDesc.BufferCount = 1;
    swapDesc.BufferDesc.Width = config.width;
    swapDesc.BufferDesc.Height = config.height;
    swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.OutputWindow = glfwGetWin32Window(window);
    swapDesc.SampleDesc.Count = 1;
    swapDesc.Windowed = TRUE;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapChain;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                             D3D11_SDK_VERSION, &swapDesc, &swapChain, &device, nullptr, &context))) {
        std::cerr << "Failed to create the Direct3D device" << std::endl;
        glfwTerminate();
        return 1;
    }

    // Where we draw: the window image, plus a depth image so nearer waves hide farther ones.
    ComPtr<ID3D11Texture2D> backBuffer;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    ComPtr<ID3D11RenderTargetView> colorView;
    // D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    // rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    device->CreateRenderTargetView(backBuffer.Get(), nullptr, &colorView);

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = config.width;
    depthDesc.Height = config.height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ComPtr<ID3D11Texture2D> depthTexture;
    device->CreateTexture2D(&depthDesc, nullptr, &depthTexture);
    ComPtr<ID3D11DepthStencilView> depthView;
    device->CreateDepthStencilView(depthTexture.Get(), nullptr, &depthView);

    // Shaders, and how a vertex is laid out in memory (position, then colour).
    ComPtr<ID3DBlob> vsCode = compileShader("vsMain", "vs_5_0");
    ComPtr<ID3DBlob> psCode = compileShader("psMain", "ps_4_0");
    ComPtr<ID3DBlob> outlineCode = compileShader("psOutline", "ps_4_0");
    if (!vsCode || !psCode || !outlineCode) {
        glfwTerminate();
        return 1;
    }
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader, outlineShader;
    device->CreateVertexShader(vsCode->GetBufferPointer(), vsCode->GetBufferSize(), nullptr, &vertexShader);
    device->CreatePixelShader(psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &pixelShader);
    device->CreatePixelShader(outlineCode->GetBufferPointer(), outlineCode->GetBufferSize(), nullptr, &outlineShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ComPtr<ID3D11InputLayout> inputLayout;
    device->CreateInputLayout(layout, 2, vsCode->GetBufferPointer(), vsCode->GetBufferSize(), &inputLayout);


    std::vector<Plane> chunks = generateChunks(config);

    Lighting lighting(device.Get(), config);
    Skybox skybox(device.Get(), config);


    const UINT indexCount = static_cast<UINT>(chunks[0].indices().size());
    const UINT verticesPerChunk = static_cast<UINT>(chunks[0].vertices().size());
    const UINT chunkBytes = verticesPerChunk * sizeof(Plane::Vertex);
    const UINT vertexBytes = chunkBytes * static_cast<UINT>(chunks.size());


    // --- Gerstner sur GPU ---
    const bool useGpuGerstner = (config.selectedmethod == "gerstner" && config.g.process == "gpu");
    GerstnerGpu gerstnerGpu;
    if (useGpuGerstner && !gerstnerGpu.init(device.Get(), config.g)) {
        std::cerr << "Failed to initialise the GPU FFT" << std::endl;
        glfwTerminate();
        return 1;
    }

    // --- FFT sur GPU ---
    const bool useGpuFFT = (config.selectedmethod == "fft" && config.f.process == "gpu");
    FFTGpu fftGpu;
    if (useGpuFFT && !fftGpu.init(device.Get(), config.f)) {
        std::cerr << "Failed to initialise the GPU FFT" << std::endl;
        glfwTerminate();
        return 1;
    }

    struct GpuWaveParams {
        UINT useGpuWave, gridN, verticesPerChunk, verticesPerRow;
        float lambda, pad[3];
    }; 
    // La copie C++ du cbuffer GpuWave, dans le même ordre. Elle fait 32 octets.


    GpuWaveParams waveParams{useGpuFFT ? 1u : 0u, static_cast<UINT>(config.f.N), verticesPerChunk,
                             static_cast<UINT>(config.cellsnumber + 1), config.f.lambda, {0, 0, 0}};
    D3D11_BUFFER_DESC waveDesc{};
    waveDesc.ByteWidth = sizeof(GpuWaveParams);   // 32 octets, multiple de 16
    waveDesc.Usage = D3D11_USAGE_IMMUTABLE;
    waveDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA waveData{&waveParams, 0, 0};
    ComPtr<ID3D11Buffer> waveBuffer;
    device->CreateBuffer(&waveDesc, &waveData, &waveBuffer);
    // Ces valeurs ne changent jamais, donc le buffer est créé une fois pour toutes avec ses données (waveData).
    // --------------------


    D3D11_BUFFER_DESC vertexDesc{};
    vertexDesc.ByteWidth = vertexBytes;
    vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ComPtr<ID3D11Buffer> vertexBuffer;
    device->CreateBuffer(&vertexDesc, nullptr, &vertexBuffer);

    D3D11_BUFFER_DESC indexDesc{};
    indexDesc.ByteWidth = indexCount * sizeof(unsigned int);
    indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexData{chunks[0].indices().data(), 0, 0};
    ComPtr<ID3D11Buffer> indexBuffer;
    device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

    // camera states
    CameraState cameraState;
    float startTilt = config.tiltDegrees * 3.14159265358979f / 180.0f;
    cameraState.tiltDegrees = config.tiltDegrees;
    cameraState.y = -config.cameraDistance * std::sin(startTilt);
    cameraState.z = -config.cameraDistance * std::cos(startTilt);
    float cameraValues[16];
    D3D11_BUFFER_DESC cameraDesc{};
    cameraDesc.ByteWidth = sizeof(cameraValues);
    cameraDesc.Usage = D3D11_USAGE_DYNAMIC;
    cameraDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cameraDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ComPtr<ID3D11Buffer> cameraBuffer;
    device->CreateBuffer(&cameraDesc, nullptr, &cameraBuffer);

    // // Draw both sides of the triangles
    D3D11_RASTERIZER_DESC rasterDesc{};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = TRUE;
    ComPtr<ID3D11RasterizerState> rasterState;
    device->CreateRasterizerState(&rasterDesc, &rasterState);

    // Outlines of the triangles
    rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterDesc.DepthBias = -1000;
    rasterDesc.SlopeScaledDepthBias = -1.0f;
    ComPtr<ID3D11RasterizerState> outlineState;
    device->CreateRasterizerState(&rasterDesc, &outlineState);

    // Settings that stay the same for every frame.
    UINT stride = sizeof(Plane::Vertex), offset = 0;
    D3D11_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(config.width), static_cast<float>(config.height), 0.0f, 1.0f};
    context->OMSetRenderTargets(1, colorView.GetAddressOf(), depthView.Get());
    context->RSSetViewports(1, &viewport);
    context->IASetInputLayout(inputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, cameraBuffer.GetAddressOf());
    const float background[4] = {0.05f, 0.05f, 0.1f, 1.0f};

    // Performance metrics
    const double metricsInterval = 0.5;
    const size_t pointCount = static_cast<size_t>(verticesPerChunk) * chunks.size();
    double metricsStart = glfwGetTime();
    int framesCounted = 0;
    int updatesCounted = 0;

    // The plane is recomputed config.calculusfrequency times per second
    const double updateInterval = config.calculusfrequency > 0 ? 1.0 / config.calculusfrequency : 0.0;
    double lastUpdate = -1.0;
    double lastFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double time = glfwGetTime();

        updateCamera(window, cameraState, static_cast<float>(time - lastFrame));
        lastFrame = time;
        cameraMatrix(config, cameraState, cameraValues);
        D3D11_MAPPED_SUBRESOURCE cameraMapped;
        context->Map(cameraBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cameraMapped);
        std::memcpy(cameraMapped.pData, cameraValues, sizeof(cameraValues));
        context->Unmap(cameraBuffer.Get(), 0);

        // Chunks follow the camera (not limited by calculusfrequency)
        bool chunksMoved = updateChunks(chunks, config, cameraState.x, cameraState.y, static_cast<float>(time));

        bool recompute = lastUpdate < 0.0 || time - lastUpdate >= updateInterval;
        if (recompute) {
            lastUpdate = time;
            for (Plane& chunk : chunks) {
                chunk.update(static_cast<float>(time));
            }
            ++updatesCounted;

            if (useGpuFFT) {
                fftGpu.compute(context.Get(), static_cast<float>(time));
            }
            // À chaque mise à jour (recompute, selon calculusfrequency), on recalcule la FFT sur le GPU. Pas de copie de sommets, pas de Map ici.

            if (useGpuGerstner) {
                 gerstnerGpu.update(context.Get(), static_cast<float>(time));
            }
        }
        if (recompute || chunksMoved) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            context->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            for (size_t i = 0; i < chunks.size(); ++i) {
                std::memcpy(static_cast<char*>(mapped.pData) + i * chunkBytes, chunks[i].vertices().data(), chunkBytes);
            }
            context->Unmap(vertexBuffer.Get(), 0);
        }

        context->ClearRenderTargetView(colorView.Get(), background);
        context->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

        // The sky first, behind everything, then the water with the light of the sun
        lighting.update(context.Get(), cameraState);
        context->RSSetState(rasterState.Get());
        skybox.draw(context.Get(), cameraState);
        context->IASetInputLayout(inputLayout.Get()); // the sky changed them
        context->VSSetShader(vertexShader.Get(), nullptr, 0);

        // Coloured triangles
        if (useGpuFFT) {
            fftGpu.bindForVertexShader(context.Get());
        }
        if (useGpuGerstner) {
            gerstnerGpu.bindForVertexShader(context.Get());
        }
        context->VSSetConstantBuffers(2, 1, waveBuffer.GetAddressOf());
        // Juste avant de dessiner l’eau : on branche les 3 résultats sur t0..t2 et le cbuffer sur b2. Il faut le refaire à chaque frame parce que le skybox, 
        // dessiné juste avant, utilise aussi l’emplacement b2 pour son propre cbuffer Sky. En remettant le nôtre juste avant DrawIndexed, 
        // on est sûr que le vertex shader de l’eau lit le bon.
        context->RSSetState(rasterState.Get());
        context->PSSetShader(pixelShader.Get(), nullptr, 0);
        for (size_t i = 0; i < chunks.size(); ++i) {
            context->DrawIndexed(indexCount, 0, static_cast<INT>(i * verticesPerChunk));
        }

        // Same triangles again, as black outlines, so each triangle is visible
        if (config.showtriangles) {
            context->RSSetState(outlineState.Get());
            context->PSSetShader(outlineShader.Get(), nullptr, 0);
            for (size_t i = 0; i < chunks.size(); ++i) {
                context->DrawIndexed(indexCount, 0, static_cast<INT>(i * verticesPerChunk));
            }
        }

        swapChain->Present(0, 0); // FPS is not capped by screen refresh rate

        ++framesCounted;
        double now = glfwGetTime();
        if (now - metricsStart >= metricsInterval) {
            double fps = framesCounted / (now - metricsStart);
            double planePasses = updatesCounted / (now - metricsStart);
            char title[256];
            std::snprintf(title, sizeof(title),
                          "DirectX Window | %.0f FPS | %.0f plane updates/s (%.2f M points/s) | %zu points",
                          fps, planePasses, planePasses * pointCount / 1e6, pointCount);
            glfwSetWindowTitle(window, title);
            metricsStart = now;
            framesCounted = 0;
            updatesCounted = 0;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
