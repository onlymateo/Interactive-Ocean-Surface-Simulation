#include <d3dcompiler.h>
#include "calculus/fft_gpu.hpp"
#include "calculus/calculus.hpp"  
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {

// Ce texte est une chaîne C++ (R"( ... )", une raw string) qu'on compile à l'exécution avec D3DCompile.
const char* CS_SOURCE = R"(
    static const float PI = 3.14159265358979;
    static const float G = 9.81;

    cbuffer Params : register(b0) {
        float waveTime;
        float planeSize;
        uint horizontal;   // 1 = FFT sur les lignes, 0 = sur les colonnes
        uint unused;
    };
    // Un cbuffer est un petit paquet de valeurs envoyées par le CPU, identiques pour tous les threads. 
    // register(b0) signifie qu'il est branché sur l'emplacement b0, celui de CSSetConstantBuffers(0, ...).
    // - waveTime : le temps de l'animation.
    // - planeSize : f.size, la taille en mètres.
    // - horizontal : 1 pour faire la FFT sur les lignes, 0 sur les colonnes.
    // - unused : du remplissage, car un cbuffer doit faire un multiple de 16 octets (4 valeurs de 4 octets).

    StructuredBuffer<float2> h0 : register(t0);
    RWStructuredBuffer<float2> bufA : register(u0);   // hauteur
    RWStructuredBuffer<float2> bufB : register(u1);   // dx
    RWStructuredBuffer<float2> bufC : register(u2);   // dz
    // - StructuredBuffer<float2> : un tableau en lecture seule. t0 est l'emplacement des SRV.
    // - RWStructuredBuffer<float2> : un tableau en lecture + écriture. u0, u1, u2 sont les emplacements des UAV.
    // - Un float2 représente un nombre complexe : .x est la partie réelle, .y la partie imaginaire.
    // - bufA, bufB et bufC valent respectivement ht, dx et dz pendant l'étape 1. 
    // Pendant la FFT, on utilise seulement bufA (il pointe vers le buffer en cours de transformation).



    float2 cmul(float2 a, float2 b) {
        return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
    }
    // Le HLSL n'a pas de type complexe. On écrit donc (a + bi)(c + di) = (ac − bd) + (ad + bc)i. C'est l'équivalent de * sur des std::complex<double>.



    // ---- Étape 1 : c'est fftTimeEvo() de fft.cpp, pour les N*N points en parallèle ----
    [numthreads(8, 8, 1)] // chaque groupe contient 8×8×1 = 64 threads.
    void csTimeEvo(uint3 id : SV_DispatchThreadID) {
        int n = id.x; // colones
        int m = id.y; // lignes
        float kx = 2.0 * PI * (n - N / 2) / planeSize;
        float kz = 2.0 * PI * (m - N / 2) / planeSize;
        float k = max(sqrt(kx * kx + kz * kz), 1e-4);
        // Le vecteur d'onde k de ce point de la grille, comme dans fftTimeEvo. 
        // N est ici la macro injectée à la compilation. max(..., 1e-4) évite la division par 0 au centre du spectre.

        float2 a = h0[m * N + n]; // On lit h0(k). La grille 2D est stockée à plat : l'indice vaut ligne * N + colonne.
        float2 b = h0[((N - m) % N) * N + ((N - n) % N)];
        b.y = -b.y;                                   // conjugué de h0(-k)
        // h0(−k), c'est-à-dire la case « opposée » de la grille (même formule que f.h0[(N - m) % N][(N - n) % N]). 
        // On met ensuite sa partie imaginaire en négatif : c'est le conjugué (std::conj). Cela garantit que la hauteur finale est réelle.

        float w = sqrt(G * k); // La fréquence angulaire ω = √(g·k), la relation de dispersion des vagues en eau profonde.
        float2 e1 = float2(cos(w * waveTime), sin(w * waveTime));
        float2 e2 = float2(e1.x, -e1.y);
        // e1 = e^(iωt), et e2 = e^(−iωt) (le conjugué de e1). C'est std::polar(1.0, ±ωt).
        float2 h = cmul(a, e1) + cmul(b, e2);
        La formule centrale de time evolution : h(k,t) = h0(k)·e^(iωt) + conj(h0(−k))·e^(−iωt).

        uint i = m * N + n;
        bufA[i] = h;
        bufB[i] = float2(h.y, -h.x) * (kx / k);       // h * (0,-1) * kx/|k|
        bufC[i] = float2(h.y, -h.x) * (kz / k);
        // On écrit les 3 champs. Pour dx et dz, votre CPU multiplie h par (0, −1) · kx/|k|. 
        // Or (a + bi)·(−i) = b − ai, donc (h.y, −h.x), puis on multiplie par le réel kx/k.
    }

    // ---- Étape 2 : FFT inverse 1D (Stockham). Un groupe = une ligne ou une colonne ----
    groupshared float2 S[2][N];
    // Mémoire partagée par tous les threads d'un groupe, très rapide, de durée de vie limitée au groupe. 
    // S[0] et S[1] sont les deux tableaux du ping-pong : on lit dans l'un et on écrit dans l'autre.

    [numthreads(HALF_N, 1, 1)] // HALF_N threads par groupe (N/2). Chaque thread fait un papillon (une combinaison de deux valeurs) par passe.
    void csFFT(uint3 gid : SV_GroupID, uint t : SV_GroupIndex) {
        // - SV_GroupID : le numéro du groupe. Avec Dispatch(N, 1, 1), gid.x va de 0 à N−1. Un groupe = une ligne (ou une colonne).
        // - SV_GroupIndex : le numéro du thread dans son groupe, de 0 à N/2−1. On l'appelle t.
        uint lineIndex = gid.x; // Le numéro de la ligne ou colonne traitée par ce groupe.

        // chargement : chaque thread charge 2 valeurs
        for (uint r = 0; r < 2; r++) {
            uint i = t + r * HALF_N;
            uint idx = (horizontal != 0) ? (lineIndex * N + i) : (i * N + lineIndex);
            S[0][i] = bufA[idx];
        }
        GroupMemoryBarrierWithGroupSync();
        // - Il y a N valeurs à charger mais seulement N/2 threads, donc chaque thread en charge 2 (r = 0 puis r = 1, qui donnent t et t + N/2).
        // - idx : si horizontal, on parcourt la ligne lineIndex, d'où l'indice lineIndex·N + i. Sinon on parcourt la colonne lineIndex, d'où i·N + lineIndex.
        // - GroupMemoryBarrierWithGroupSync() : tous les threads attendent que tous aient fini d'écrire dans S avant de continuer. 
        // Sans cette barrière, un thread pourrait lire une case que son voisin n'a pas encore écrite.

        // log2(N) passes. On lit dans S[src] et on écrit dans S[src ^ 1].
        uint src = 0; // src indique quel tableau (0 ou 1) contient les données actuelles.
        for (uint Ns = 1; Ns < N; Ns <<= 1) {
            // Ns est la taille des sous-FFT déjà calculées : 1, 2, 4, …, N/2. Il y a donc log2(N) passes. Par exemple, 9 passes pour N = 512.
            uint k = t & (Ns - 1); // t mod Ns (valable car Ns est une puissance de 2). C'est la position du thread à l'intérieur de sa sous-FFT.
            uint dst = ((t - k) << 1) + k; 
            // L'indice d'écriture. (t − k) est le début de son bloc, << 1 le double (la sous-FFT de la passe suivante est deux fois plus grande), 
            // et on ajoute k pour garder la position dans le bloc. C'est ce réarrangement qui permet de se passer du bit-reversal.

            float angle = PI * k / Ns;                // signe + : FFT inverse
            float2 w = float2(cos(angle), sin(angle));
            // Le facteur de rotation (twiddle factor) e^(iπk/Ns). Le signe est + car c’est la FFT inverse, comme ang = 2π/len * (inverse ? 1 : -1) dans fft1d.

            float2 v0 = S[src][t];
            float2 v1 = cmul(S[src][t + HALF_N], w);
            // Les deux valeurs du papillon : la première, et la seconde multipliée par le facteur de rotation.

            S[src ^ 1][dst] = v0 + v1;
            S[src ^ 1][dst + Ns] = v0 - v1;
            // Le papillon : somme et différence, comme x[i+j] = u + v et x[i+j+len/2] = u − v dans votre code. src ^ 1 est l’autre tableau (0↔1).

            src ^= 1;
            GroupMemoryBarrierWithGroupSync();
            // On inverse les rôles des deux tableaux pour la passe suivante. La barrière garantit que tous ont fini d’écrire avant que quiconque relise.
        }

        // écriture du résultat
        for (uint r2 = 0; r2 < 2; r2++) {
            uint i = t + r2 * HALF_N;
            uint idx = (horizontal != 0) ? (lineIndex * N + i) : (i * N + lineIndex);
            bufA[idx] = S[src][i];
        }
        // On remet le résultat dans le buffer global, au même endroit qu’au chargement.
        // Le nom r2 évite de redéfinir r. Il n’y a pas de division par N, comme expliqué plus haut.
    }
)";

struct ParamsData {
    float time;
    float size;
    unsigned int horizontal;
    unsigned int unused;
};
// La copie en C++ du cbuffer Params. Mêmes champs, même ordre, mêmes types, sinon le GPU lit n’importe quoi. 16 octets au total.

ComPtr<ID3D11ComputeShader> compileShader(ID3D11Device* device, const char* entry, int N) {
    std::string n = std::to_string(N);
    std::string half = std::to_string(N / 2);
    D3D_SHADER_MACRO defines[] = {{"N", n.c_str()}, {"HALF_N", half.c_str()}, {nullptr, nullptr}};
    // On crée les macros N et HALF_N comme si on avait écrit #define N 512 dans le shader. Le tableau se termine par {nullptr, nullptr}, qui sert de marqueur de fin.

    ComPtr<ID3DBlob> code, errors;
    D3DCompile(CS_SOURCE, std::strlen(CS_SOURCE), nullptr, defines, nullptr,
               entry, "cs_5_0", 0, 0, &code, &errors);
    // Compile le texte HLSL.
    //  -  entry : le nom de la fonction à compiler (csTimeEvo ou csFFT).
    //  -  "cs_5_0" : compute shader, Shader Model 5.
    //  -  code reçoit le programme compilé, errors le texte des erreurs.


    if (errors) {
        std::cerr << static_cast<const char*>(errors->GetBufferPointer()) << std::endl;
    }
    // Affiche les erreurs de compilation HLSL dans la console.

    ComPtr<ID3D11ComputeShader> shader;
    if (code) {
        device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader);
    }
    // Transforme le programme compilé en objet utilisable par le GPU.
    return shader;
}

} // namespace


// Crée un tableau de N*N (count) float2 sur le GPU (avec ses vues lecture et écriture).
static bool makeBuffer(ID3D11Device* device, int count, const void* initialData,
                       ComPtr<ID3D11Buffer>& buffer,
                       ComPtr<ID3D11ShaderResourceView>& srv,
                       ComPtr<ID3D11UnorderedAccessView>& uav) {
    D3D11_BUFFER_DESC desc{};

    desc.ByteWidth = count * 2 * sizeof(float);
    // La taille totale : count éléments de 2 floats.

    desc.Usage = D3D11_USAGE_DEFAULT;
    // Mémoire GPU standard : le CPU ne la modifie pas directement.

    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    // Autorise les deux vues : lecture (SRV) et lecture-écriture (UAV).

    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = 2 * sizeof(float);
    // Dit que c’est un buffer structuré (tableau d’éléments de taille fixe), avec 8 octets par élément (un float2).

    D3D11_SUBRESOURCE_DATA data{initialData, 0, 0};
    if (FAILED(device->CreateBuffer(&desc, initialData ? &data : nullptr, &buffer))) return false;
    // Crée le buffer. Si on fournit initialData (cas de h0), il y est copié. Sinon (pour ht, dx, dz), il reste vide.

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.NumElements = count;
    // La vue lecture. FORMAT_UNKNOWN est obligatoire pour un buffer structuré, car c’est le stride qui décrit l’élément.
    // NumElements est le nombre d’éléments visibles. La vue UAV (uavDesc) est faite pareil.

    if (FAILED(device->CreateShaderResourceView(buffer.Get(), &srvDesc, &srv))) return false;

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = count;
    // La vue lecture et écriture

    return SUCCEEDED(device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, &uav));

}

bool FFTGpu::init(ID3D11Device* device, FFTConfig f) {
    N_ = f.N;
    size_ = f.size;
    if (N_ < 8 || (N_ & (N_ - 1)) != 0 || N_ > 2048) {
        std::cerr << "GPU FFT: N must be a power of two between 8 and 2048 (got " << N_ << ")" << std::endl;
        return false;
    }
    // Vérifie les conditions de validité :
    //  N ≥ 8, car le time-evo travaille par blocs de 8.
    //  N & (N−1) == 0 : astuce classique pour vérifier que N est une puissance de 2.
    //  N ≤ 2048, car on ne peut pas dépasser 1024 threads par groupe (N/2).


    initFFTSpectrum(f);
    // h0 est tiré une seule fois sur le CPU (même code qu'avant), puis envoyé au GPU


    std::vector<float> h0Data(static_cast<size_t>(N_) * N_ * 2);
    for (int m = 0; m < N_; m++) {
        for (int n = 0; n < N_; n++) {
            h0Data[(m * N_ + n) * 2 + 0] = static_cast<float>(f.h0[m][n].real());
            h0Data[(m * N_ + n) * 2 + 1] = static_cast<float>(f.h0[m][n].imag());
        }
    }
    // Convertit la grille de complex<double> en un tableau plat de floats : 
    // réel, imaginaire, réel, imaginaire… C’est le même ordre que float2 dans le shader. 
    // Le GPU travaille en float, d’où la conversion depuis double.

    const int count = N_ * N_;
    if (!makeBuffer(device, count, h0Data.data(), h0_.buffer, h0_.srv, h0_.uav)) return false;
    if (!makeBuffer(device, count, nullptr, ht_.buffer, ht_.srv, ht_.uav)) return false;
    if (!makeBuffer(device, count, nullptr, dx_.buffer, dx_.srv, dx_.uav)) return false;
    if (!makeBuffer(device, count, nullptr, dz_.buffer, dz_.srv, dz_.uav)) return false;
    // Un buffer avec les données de h0, trois buffers vides pour les résultats.


    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = sizeof(ParamsData);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&cbDesc, nullptr, &params_))) return false;
    // Le petit buffer de constantes (16 octets).


    timeEvoShader_ = compileShader(device, "csTimeEvo", N_);
    fftShader_ = compileShader(device, "csFFT", N_);
    return timeEvoShader_ && fftShader_;
    // Compile les deux shaders avec la valeur de N. Renvoie true si les deux ont réussi.
}

void FFTGpu::setParams(ID3D11DeviceContext* context, float time, unsigned int horizontal) {
    ParamsData data{time, size_, horizontal, 0};
    context->UpdateSubresource(params_.Get(), 0, nullptr, &data, 0, 0);
    // Écrit les valeurs dans le constant buffer. UpdateSubresource met la copie dans la file de commandes du GPU, 
    // donc elle est prise en compte dans l’ordre, entre deux Dispatch. 
    // C’est important : chaque dispatch de FFT voit sa propre valeur de horizontal.
}

void FFTGpu::compute(ID3D11DeviceContext* context, float time) {
    ID3D11ShaderResourceView* noSrv[3] = {nullptr, nullptr, nullptr};
    context->VSSetShaderResources(0, 3, noSrv);
    // Le vertex shader lisait peut-être ces buffers à la frame précédente. On les débranche, sinon on ne peut pas les brancher en écriture (UAV).

    context->CSSetConstantBuffers(0, 1, params_.GetAddressOf());
    // Branche le cbuffer sur b0 pour les compute shaders.



    // 1) évolution temporelle : N/8 x N/8 groupes de 8x8 threads = un thread par point
    setParams(context, time, 0);
    context->CSSetShader(timeEvoShader_.Get(), nullptr, 0);
    // Écrit les paramètres (horizontal n’est pas utilisé ici) et choisit le shader à exécuter.

    context->CSSetShaderResources(0, 1, h0_.srv.GetAddressOf());
    //Branche h0 en lecture sur t0.

    ID3D11UnorderedAccessView* all[3] = {ht_.uav.Get(), dx_.uav.Get(), dz_.uav.Get()};
    context->CSSetUnorderedAccessViews(0, 3, all, nullptr);
    // Branche les 3 buffers de résultat en écriture sur u0, u1, u2. Le dernier paramètre (nullptr) sert aux compteurs internes, dont on n’a pas besoin.

    context->Dispatch(N_ / 8, N_ / 8, 1);
    // Lance (N/8)×(N/8) groupes de 8×8 threads. Soit N×N threads au total, un par point de la grille. Pour N = 512, cela fait 64×64 = 4096 groupes.


    // 2) FFT : pour chacun des 3 buffers, d'abord sur les lignes puis sur les colonnes
    context->CSSetShader(fftShader_.Get(), nullptr, 0);
    ID3D11UnorderedAccessView* targets[3] = {ht_.uav.Get(), dx_.uav.Get(), dz_.uav.Get()};
    // On passe au shader de FFT, puis on traite les 3 champs l’un après l’autre.

    for (ID3D11UnorderedAccessView* target : targets) {
        ID3D11UnorderedAccessView* bound[3] = {target, nullptr, nullptr};
        context->CSSetUnorderedAccessViews(0, 3, bound, nullptr);
        // Le champ courant est branché sur u0, qui est bufA dans le shader. u1 et u2 sont vidés pour éviter toute confusion.

        for (unsigned int horizontal : {1u, 0u}) {
            setParams(context, time, horizontal);
            context->Dispatch(N_, 1, 1);
        }
        // Le champ courant est branché sur u0, qui est bufA dans le shader. u1 et u2 sont vidés pour éviter toute confusion.
    }

    // 3) Nettoyage
    ID3D11UnorderedAccessView* noUav[3] = {nullptr, nullptr, nullptr};
    context->CSSetUnorderedAccessViews(0, 3, noUav, nullptr);
    ID3D11ShaderResourceView* noCsSrv[1] = {nullptr};
    context->CSSetShaderResources(0, 1, noCsSrv);
    // On débranche tout du compute shader. Sans ça, les buffers resteraient « verrouillés en écriture » et le vertex shader ne pourrait pas les lire.
}

void FFTGpu::bindForVertexShader(ID3D11DeviceContext* context) {
    ID3D11ShaderResourceView* views[3] = {ht_.srv.Get(), dx_.srv.Get(), dz_.srv.Get()};
    context->VSSetShaderResources(0, 3, views);
    // Branche les trois résultats en lecture sur t0, t1, t2 du vertex shader. Ils correspondent à waveHeight, waveDx et waveDz.
}