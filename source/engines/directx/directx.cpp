#include "engines/directx/directx.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include "render/plane.hpp"

using Microsoft::WRL::ComPtr;

namespace {

// Vertex shader: place the point on screen. Pixel shader: use the colour of the vertex.
const char* SHADER_SOURCE = R"(
cbuffer Camera : register(b0) {
    row_major float4x4 viewProjection;
};

struct VertexOut {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VertexOut vsMain(float3 position : POSITION, float3 color : COLOR) {
    VertexOut output;
    output.position = mul(viewProjection, float4(position, 1.0));
    output.color = color;
    return output;
}

float4 psMain(VertexOut input) : SV_TARGET {
    return float4(input.color, 1.0);
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

// Projection and model-view in one matrix: where the plane sits and how it is turned.
// Same maths as glFrustum + glTranslatef + glRotatef, with depth mapped to 0..1 for Direct3D.
void cameraMatrix(const Config& config, float out[16]) {
    float n = static_cast<float>(config.nearPlane);
    float f = static_cast<float>(config.farPlane);
    float halfHeight = static_cast<float>(config.halfHeightAtNear);
    float halfWidth = halfHeight * static_cast<float>(config.width) / static_cast<float>(config.height);

    float tilt = config.tiltDegrees * 3.14159265358979f / 180.0f;
    float c = std::cos(tilt), s = std::sin(tilt);

    const float projection[16] = {
        n / halfWidth, 0.0f,           0.0f,        0.0f,
        0.0f,          n / halfHeight, 0.0f,        0.0f,
        0.0f,          0.0f,           f / (n - f), n * f / (n - f),
        0.0f,          0.0f,           -1.0f,       0.0f,
    };
    const float view[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c,    -s,   0.0f,
        0.0f, s,    c,    config.cameraDistance,
        0.0f, 0.0f, 0.0f, 1.0f,
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

int runDirectX(const Config& config, HeightFunction wave) {
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
    ComPtr<ID3DBlob> vsCode = compileShader("vsMain", "vs_4_0");
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

    // The plane: the triangles never change, the heights are rewritten every frame.
    Plane plane(config.planeSize, config.cellsnumber, wave);
    const UINT indexCount = static_cast<UINT>(plane.indices().size());
    const UINT vertexBytes = static_cast<UINT>(plane.vertices().size() * sizeof(Plane::Vertex));

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
    D3D11_SUBRESOURCE_DATA indexData{plane.indices().data(), 0, 0};
    ComPtr<ID3D11Buffer> indexBuffer;
    device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

    // The camera never moves, so its matrix is computed once.
    float camera[16];
    cameraMatrix(config, camera);
    D3D11_BUFFER_DESC cameraDesc{};
    cameraDesc.ByteWidth = sizeof(camera);
    cameraDesc.Usage = D3D11_USAGE_IMMUTABLE;
    cameraDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA cameraData{camera, 0, 0};
    ComPtr<ID3D11Buffer> cameraBuffer;
    device->CreateBuffer(&cameraDesc, &cameraData, &cameraBuffer);

    // Draw both sides of the triangles (by default Direct3D hides the back side).
    D3D11_RASTERIZER_DESC rasterDesc{};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = TRUE;
    ComPtr<ID3D11RasterizerState> rasterState;
    device->CreateRasterizerState(&rasterDesc, &rasterState);

    // The same triangles as outlines, pulled slightly towards the camera so they are not hidden by the fill.
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

    // Each frame: clear, move the waves to the current time, draw, then show the image.
    const float background[4] = {0.05f, 0.05f, 0.1f, 1.0f};
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        plane.update(static_cast<float>(glfwGetTime()));
        D3D11_MAPPED_SUBRESOURCE mapped;
        context->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        std::memcpy(mapped.pData, plane.vertices().data(), vertexBytes);
        context->Unmap(vertexBuffer.Get(), 0);

        context->ClearRenderTargetView(colorView.Get(), background);
        context->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

        // Coloured triangles
        context->RSSetState(rasterState.Get());
        context->PSSetShader(pixelShader.Get(), nullptr, 0);
        context->DrawIndexed(indexCount, 0, 0);

        // Same triangles again, as black outlines, so each triangle is visible
        context->RSSetState(outlineState.Get());
        context->PSSetShader(outlineShader.Get(), nullptr, 0);
        context->DrawIndexed(indexCount, 0, 0);

        swapChain->Present(1, 0);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
