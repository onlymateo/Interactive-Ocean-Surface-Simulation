# Questions / Questions

Questions I asked while learning the FFT and the GPU code, with the answers in French (**FR**) and in English (**EN**).
Questions posées pendant que j'apprenais la FFT et le code GPU, avec les réponses en français (**FR**) et en anglais (**EN**).

## Contents / Sommaire

1. [Maths: complex numbers and the FFT / Maths : nombres complexes et FFT](#1-maths-complex-numbers-and-the-fft--maths--nombres-complexes-et-fft)
   - [1.1 Why imaginary numbers in the FFT? / Pourquoi des nombres imaginaires dans la FFT ?](#11-why-imaginary-numbers-in-the-fft--pourquoi-des-nombres-imaginaires-dans-la-fft-)
   - [1.2 Complex vs imaginary / Complexe ou imaginaire ?](#12-complex-vs-imaginary--complexe-ou-imaginaire-)
   - [1.3 Why does the conjugate give a real height? / Pourquoi le conjugué donne une hauteur réelle ?](#13-why-does-the-conjugate-give-a-real-height--pourquoi-le-conjugué-donne-une-hauteur-réelle-)
   - [1.4 How are complex numbers handled in the code? / Comment sont gérés les complexes dans le code ?](#14-how-are-complex-numbers-handled-in-the-code--comment-sont-gérés-les-complexes-dans-le-code-)
2. [GPU and shaders / GPU et shaders](#2-gpu-and-shaders--gpu-et-shaders)
   - [2.1 Why SRV and UAV? / Pourquoi SRV et UAV ?](#21-why-srv-and-uav--pourquoi-srv-et-uav-)
   - [2.2 What is a numthreads group? / Qu'est-ce qu'un groupe numthreads ?](#22-what-is-a-numthreads-group--quest-ce-quun-groupe-numthreads-)
   - [2.3 b0, b1... t0, t1... u0, u1...](#23-b0-b1-t0-t1-u0-u1--b0-b1-t0-t1-u0-u1)
3. [Code details / Détails du code](#3-code-details--détails-du-code)
   - [3.1 What is a mod? `t & (Ns - 1)` / C'est quoi un mod ?](#31-what-is-a-mod-t--ns---1--cest-quoi-un-mod-)
   - [3.2 What is a stride in `makeBuffer`? / C'est quoi un stride dans `makeBuffer` ?](#32-what-is-a-stride-in-makebuffer--cest-quoi-un-stride-dans-makebuffer-)

---

# 1. Maths: complex numbers and the FFT / Maths : nombres complexes et FFT

## 1.1 Why imaginary numbers in the FFT? / Pourquoi des nombres imaginaires dans la FFT ?

**FR**
Une vague, c'est une sinusoïde. Pour la décrire il faut **deux informations** : son **amplitude** (sa hauteur) et sa **phase** (où elle en est dans son cycle, donc son décalage).
Un nombre complexe contient exactement deux nombres (`a + bi`), donc il range l'amplitude et la phase dans une seule valeur.

La formule d'Euler fait le lien : `e^(iθ) = cos θ + i sin θ`. Multiplier par `e^(iθ)` fait **tourner** le nombre d'un angle θ dans le plan complexe, ce qui décale la phase de la vague. On s'en sert à deux endroits :
- **L'évolution dans le temps** : `h(k,t) = h0(k) · e^(iωt) + ...`. À chaque instant on fait tourner la phase de chaque vague de `ωt`. C'est ce qui fait avancer les vagues.
- **La FFT elle-même** : à chaque étape (le « papillon »), on multiplie par `e^(±2πik/N)` (les *twiddle factors*). Sans complexes, il faudrait deux transformées séparées (une en cosinus, une en sinus) et jongler avec les deux.

Les nombres imaginaires ne sont donc qu'un outil de calcul pratique. Le résultat final (la hauteur de l'eau) est un nombre réel.

**EN**
A wave is a sinusoid. Describing it takes **two pieces of information**: its **amplitude** (how tall) and its **phase** (where it is in its cycle, i.e. its shift).
A complex number holds exactly two numbers (`a + bi`), so it stores the amplitude and the phase in a single value.

Euler's formula links them: `e^(iθ) = cos θ + i sin θ`. Multiplying by `e^(iθ)` **rotates** the number by an angle θ in the complex plane, which shifts the phase of the wave. It is used in two places:
- **Time evolution**: `h(k,t) = h0(k) · e^(iωt) + ...`. At each instant the phase of every wave is rotated by `ωt`. This is what makes the waves move.
- **The FFT itself**: at each step (the "butterfly"), values are multiplied by `e^(±2πik/N)` (the *twiddle factors*). Without complex numbers you would need two separate transforms (one for cosines, one for sines) and juggle both.

So imaginary numbers are only a convenient computing tool. The final result (the water height) is a real number.

---

## 1.2 Complex vs imaginary / Complexe ou imaginaire ?

**FR**
- Un nombre **imaginaire pur** est un multiple de `i`, où `i² = −1`. Exemples : `3i`, `−0.5i`.
- Un nombre **complexe** s'écrit `a + bi` : une **partie réelle** `a` et une **partie imaginaire** `b`. Exemple : `2 + 3i`.

Donc :
- un nombre réel est un complexe avec `b = 0` (par exemple `5 = 5 + 0i`),
- un imaginaire pur est un complexe avec `a = 0` (par exemple `3i = 0 + 3i`),
- les imaginaires purs et les réels sont des cas particuliers des complexes.

Dans le langage courant, on dit souvent « nombres imaginaires » pour dire « nombres complexes ». Dans la FFT, on manipule des **complexes** (partie réelle et imaginaire non nulles), pas seulement des imaginaires purs.

**EN**
- An **imaginary number** (pure imaginary) is a multiple of `i`, where `i² = −1`. Examples: `3i`, `−0.5i`.
- A **complex number** is written `a + bi`: a **real part** `a` and an **imaginary part** `b`. Example: `2 + 3i`.

So:
- a real number is a complex number with `b = 0` (e.g. `5 = 5 + 0i`),
- a pure imaginary number is a complex number with `a = 0` (e.g. `3i = 0 + 3i`),
- real and pure imaginary numbers are special cases of complex numbers.

In everyday speech people often say "imaginary numbers" to mean "complex numbers". In the FFT we work with **complex** numbers (both parts non-zero), not only pure imaginary ones.

---

## 1.3 Why does the conjugate give a real height? / Pourquoi le conjugué donne une hauteur réelle ?

**FR**
La hauteur de l'eau est une somme de vagues, chaque vague étant `h(k) · e^(i k·x)`. Pour que le total soit un nombre **réel**, il faut que les vagues se regroupent par paires, `k` et `−k`, qui se compensent.

Prenons la paire `k` / `−k` :

`h(k) · e^(i k·x) + h(−k) · e^(−i k·x)`

Si on impose `h(−k) = conj(h(k))` (le conjugué : on change le signe de la partie imaginaire), alors le deuxième terme est **le conjugué du premier** :

`conj( h(k) · e^(i k·x) ) = conj(h(k)) · e^(−i k·x)`

Or pour tout nombre complexe `z`, `z + conj(z) = 2 · Re(z)`, qui est **réel** : les parties imaginaires s'annulent. Chaque paire donne donc un nombre réel, et la somme de toutes les paires aussi.

C'est la raison de cette ligne dans le code :

`h(k,t) = h0(k) · e^(iωt) + conj(h0(−k)) · e^(−iωt)`

On prend le **conjugué de `h0(−k)`** (c'est pour ça que le code lit la case opposée de la grille, `(N−n) % N`, puis inverse le signe de sa partie imaginaire). On peut vérifier que ça donne bien `h(−k,t) = conj(h(k,t))` à chaque instant.

Les déplacements horizontaux `dx` et `dz`, multipliés par `−i · kx/|k|`, restent réels aussi, car `kx` change de signe entre `k` et `−k`.

En pratique, après la FFT inverse, la partie imaginaire n'est pas exactement 0 mais un tout petit nombre dû aux arrondis. C'est pour ça que le code ne lit que la partie réelle (`.real()` sur le CPU, `.x` sur le GPU).

**EN**
The water height is a sum of waves, each one being `h(k) · e^(i k·x)`. For the total to be a **real** number, the waves have to come in pairs, `k` and `−k`, that compensate each other.

Take the `k` / `−k` pair:

`h(k) · e^(i k·x) + h(−k) · e^(−i k·x)`

If we impose `h(−k) = conj(h(k))` (the conjugate: flip the sign of the imaginary part), the second term is **the conjugate of the first**:

`conj( h(k) · e^(i k·x) ) = conj(h(k)) · e^(−i k·x)`

For any complex number `z`, `z + conj(z) = 2 · Re(z)`, which is **real**: the imaginary parts cancel. Each pair gives a real number, and so does the sum of all pairs.

That is the reason for this line in the code:

`h(k,t) = h0(k) · e^(iωt) + conj(h0(−k)) · e^(−iωt)`

We take the **conjugate of `h0(−k)`** (this is why the code reads the opposite cell of the grid, `(N−n) % N`, then flips the sign of its imaginary part). One can check that this gives `h(−k,t) = conj(h(k,t))` at every instant.

The horizontal displacements `dx` and `dz`, multiplied by `−i · kx/|k|`, stay real too, because `kx` changes sign between `k` and `−k`.

In practice, after the inverse FFT the imaginary part is not exactly 0 but a tiny number due to rounding. That is why the code only reads the real part (`.real()` on the CPU, `.x` on the GPU).

---

## 1.4 How are complex numbers handled in the code? / Comment sont gérés les complexes dans le code ?

**FR**

**Sur le CPU**, on utilise `std::complex<double>` (renommé `Complex` dans `calculus.hpp`). Il sait faire les opérations de base :

| Besoin | Code CPU |
|---|---|
| créer `a + bi` | `Complex(a, b)` |
| multiplier, additionner | `*`, `+` |
| conjugué | `std::conj(z)` |
| `e^(iθ)` | `std::polar(1.0, θ)` |
| partie réelle / imaginaire | `z.real()`, `z.imag()` |
| multiplier par `−i` | `Complex(0, -1) * z` |

La grille est un `std::vector<std::vector<Complex>>` (`ComplexGrid` dans `FFTConfig`).

**Sur le GPU**, le HLSL n'a **pas** de type complexe. On utilise un `float2` : `.x` est la partie réelle, `.y` la partie imaginaire. Les opérations sont écrites à la main :

| Besoin | Code HLSL |
|---|---|
| `e^(iθ)` | `float2(cos(θ), sin(θ))` |
| conjugué | on inverse le signe de `.y` |
| multiplier `a` par `b` | `cmul(a, b)` = `(a.x·b.x − a.y·b.y, a.x·b.y + a.y·b.x)` |
| multiplier `(a, b)` par `−i` | `float2(b, −a)` |

Pour envoyer `h0` au GPU, on le met à plat dans un tableau de `float` : réel, imaginaire, réel, imaginaire... Cet ordre correspond à un `float2` dans le shader.

**Affichage :** les nombres complexes ne sont **jamais affichés** à l'écran. Seule la hauteur réelle finale compte : le code ne lit que `.real()` (CPU) ou `.x` (GPU), et la partie imaginaire est ignorée (elle vaut ~0). Pour déboguer, `std::cout << z` affiche un complexe sous la forme `(réel,imaginaire)` sur le CPU.

**EN**

**On the CPU**, we use `std::complex<double>` (renamed `Complex` in `calculus.hpp`). It knows the basic operations:

| Need | CPU code |
|---|---|
| create `a + bi` | `Complex(a, b)` |
| multiply, add | `*`, `+` |
| conjugate | `std::conj(z)` |
| `e^(iθ)` | `std::polar(1.0, θ)` |
| real / imaginary part | `z.real()`, `z.imag()` |
| multiply by `−i` | `Complex(0, -1) * z` |

The grid is a `std::vector<std::vector<Complex>>` (`ComplexGrid` in `FFTConfig`).

**On the GPU**, HLSL has **no** complex type. We use a `float2`: `.x` is the real part, `.y` the imaginary part. The operations are written by hand:

| Need | HLSL code |
|---|---|
| `e^(iθ)` | `float2(cos(θ), sin(θ))` |
| conjugate | flip the sign of `.y` |
| multiply `a` by `b` | `cmul(a, b)` = `(a.x·b.x − a.y·b.y, a.x·b.y + a.y·b.x)` |
| multiply `(a, b)` by `−i` | `float2(b, −a)` |

To send `h0` to the GPU, it is flattened into an array of `float`: real, imaginary, real, imaginary... This order matches a `float2` in the shader.

**Display:** complex numbers are **never displayed** on screen. Only the final real height matters: the code only reads `.real()` (CPU) or `.x` (GPU), and the imaginary part is ignored (it is ~0). To debug, `std::cout << z` prints a complex number as `(real,imaginary)` on the CPU.

---

# 2. GPU and shaders / GPU et shaders

## 2.1 Why SRV and UAV? / Pourquoi SRV et UAV ?

**FR**
Un buffer GPU n'est que de la **mémoire**. Pour qu'un shader l'utilise, il faut créer une **vue** qui dit comment le lire et avec quels droits :

| Vue | Nom complet | Droits | Dans HLSL |
|---|---|---|---|
| **SRV** | Shader Resource View | **lecture seule** | `StructuredBuffer<float2>`, `Texture2D` |
| **UAV** | Unordered Access View | **lecture + écriture** | `RWStructuredBuffer<float2>`, `RWTexture2D` |

« Unordered » veut dire que les threads peuvent écrire n'importe où et dans n'importe quel ordre.

Pourquoi deux vues et pas une seule ?
- Le GPU peut **optimiser** la lecture seule (caches, pas de risque de conflit entre threads).
- Un même buffer ne peut pas être lu **et** écrit en même temps par des étapes différentes : D3D11 ne laisse pas une ressource branchée en SRV et en UAV à la fois (il débranche l'une des deux). C'est pour ça que le code débranche tout entre les étapes.

Dans notre code :
- `h0` est lu avec son **SRV** (on ne l'écrit jamais).
- `ht`, `dx`, `dz` sont **écrits avec leur UAV** pendant la FFT (compute shader), puis **lus avec leur SRV** par le vertex shader pour déplacer les sommets. Un vertex shader ne peut pas écrire dans un UAV en D3D11, donc il lit en SRV.
- `makeBuffer` crée donc les deux vues pour chaque buffer.

**EN**
A GPU buffer is just **memory**. For a shader to use it, you create a **view** that says how to read it and with what rights:

| View | Full name | Rights | In HLSL |
|---|---|---|---|
| **SRV** | Shader Resource View | **read-only** | `StructuredBuffer<float2>`, `Texture2D` |
| **UAV** | Unordered Access View | **read + write** | `RWStructuredBuffer<float2>`, `RWTexture2D` |

"Unordered" means threads can write anywhere and in any order.

Why two views and not just one?
- The GPU can **optimise** read-only access (caches, no conflict between threads).
- A buffer cannot be read **and** written by different stages at the same time: D3D11 does not let a resource be bound as an SRV and a UAV at once (it unbinds one of them). This is why the code unbinds everything between steps.

In our code:
- `h0` is read through its **SRV** (it is never written).
- `ht`, `dx`, `dz` are **written through their UAV** during the FFT (compute shader), then **read through their SRV** by the vertex shader to move the vertices. A vertex shader cannot write to a UAV in D3D11, so it reads through the SRV.
- `makeBuffer` therefore creates both views for each buffer.

---

## 2.2 What is a numthreads group? / Qu'est-ce qu'un groupe numthreads ?

**FR**
Un GPU lance des milliers de petits calculs (**threads**) en parallèle. Pour les organiser, il les range en **groupes** (*thread groups*). Un groupe est un paquet de threads qui :
- tournent ensemble sur la même unité de calcul du GPU,
- peuvent **partager une mémoire rapide** (`groupshared`),
- peuvent **s'attendre** avec une barrière (`GroupMemoryBarrierWithGroupSync()`).

Deux threads de groupes différents ne peuvent pas se synchroniser.

`[numthreads(X, Y, Z)]` fixe la **taille d'un groupe** : `X × Y × Z` threads. Par exemple `[numthreads(8, 8, 1)]` = 64 threads rangés en carré de 8×8.
Côté C++, `Dispatch(GX, GY, GZ)` fixe le **nombre de groupes**. Le nombre total de threads est donc `GX·GY·GZ × (X·Y·Z)`.

Dans notre code :

| Shader | `numthreads` | `Dispatch` | Résultat |
|---|---|---|---|
| `csTimeEvo` | `(8, 8, 1)` | `(N/8, N/8, 1)` | **un thread par point** de la grille (N×N threads) |
| `csFFT` | `(N/2, 1, 1)` | `(N, 1, 1)` | **un groupe par ligne** (ou colonne), chaque thread fait un papillon |

Chaque thread sait qui il est grâce à des identifiants :
- `SV_DispatchThreadID` : son numéro global dans tout le dispatch (utilisé dans `csTimeEvo`),
- `SV_GroupID` : le numéro de son groupe (la ligne traitée dans `csFFT`),
- `SV_GroupIndex` : son numéro dans son groupe (`t` dans `csFFT`).

Limite : 1024 threads par groupe au maximum. C'est pour ça que la FFT GPU demande `N/2 ≤ 1024`, donc `N ≤ 2048`.

**EN**
A GPU runs thousands of small computations (**threads**) in parallel. To organise them, it arranges them in **groups** (*thread groups*). A group is a bundle of threads that:
- run together on the same compute unit of the GPU,
- can **share fast memory** (`groupshared`),
- can **wait for each other** with a barrier (`GroupMemoryBarrierWithGroupSync()`).

Two threads from different groups cannot synchronise.

`[numthreads(X, Y, Z)]` sets the **size of one group**: `X × Y × Z` threads. For example `[numthreads(8, 8, 1)]` = 64 threads in an 8×8 square.
On the C++ side, `Dispatch(GX, GY, GZ)` sets the **number of groups**. The total number of threads is therefore `GX·GY·GZ × (X·Y·Z)`.

In our code:

| Shader | `numthreads` | `Dispatch` | Result |
|---|---|---|---|
| `csTimeEvo` | `(8, 8, 1)` | `(N/8, N/8, 1)` | **one thread per grid point** (N×N threads) |
| `csFFT` | `(N/2, 1, 1)` | `(N, 1, 1)` | **one group per row** (or column), each thread does one butterfly |

Each thread knows who it is thanks to identifiers:
- `SV_DispatchThreadID`: its global number in the whole dispatch (used in `csTimeEvo`),
- `SV_GroupID`: the number of its group (the row handled in `csFFT`),
- `SV_GroupIndex`: its number inside its group (`t` in `csFFT`).

Limit: 1024 threads per group at most. This is why the GPU FFT needs `N/2 ≤ 1024`, so `N ≤ 2048`.

---

## 2.3 b0, b1... t0, t1... u0, u1... / b0, b1... t0, t1... u0, u1...

**FR**
Dans `register(xN)`, la **lettre** donne le **type de ressource** et le **chiffre** donne le **numéro d'emplacement** (*slot*) :

| Lettre | Type | Droits | Exemple dans le projet |
|---|---|---|---|
| **b** | *buffer* de constantes (`cbuffer`) | lecture seule | `b0` caméra, `b1` lumière, `b2` paramètres FFT, `b3` Gerstner |
| **t** | ressource en lecture (*texture*) : textures et buffers structurés (SRV) | lecture seule | `t0` = `h0` (compute) ; `t0..t2` = `waveHeight`, `waveDx`, `waveDz` (vertex) |
| **u** | ressource en lecture + écriture (UAV) | lecture + écriture | `u0..u2` = `bufA`, `bufB`, `bufC` (compute) |
| *s* | échantillonneur (*sampler*) | | pas utilisé ici |

Le chiffre doit correspondre au premier argument de l'appel C++ :
- `VSSetConstantBuffers(3, ...)` branche sur `b3`,
- `VSSetShaderResources(0, 3, ...)` branche sur `t0`, `t1`, `t2`,
- `CSSetUnorderedAccessViews(0, 3, ...)` branche sur `u0`, `u1`, `u2`.

Bon à savoir :
- Les compteurs sont **séparés par lettre** : `b0`, `t0` et `u0` n'entrent pas en conflit.
- Chaque **étape** (VS, PS, CS) a sa propre série d'emplacements : le `b0` du vertex shader est indépendant du `b0` du pixel shader.
- Le skybox utilise aussi `b2` (son cbuffer `Sky`), c'est pourquoi on rebranche notre `b2` juste avant de dessiner l'eau.

**EN**
In `register(xN)`, the **letter** gives the **resource type** and the **number** gives the **slot** index:

| Letter | Type | Rights | Example in the project |
|---|---|---|---|
| **b** | constant *buffer* (`cbuffer`) | read-only | `b0` camera, `b1` lighting, `b2` FFT parameters, `b3` Gerstner |
| **t** | read resource (*texture*): textures and structured buffers (SRV) | read-only | `t0` = `h0` (compute); `t0..t2` = `waveHeight`, `waveDx`, `waveDz` (vertex) |
| **u** | read + write resource (UAV) | read + write | `u0..u2` = `bufA`, `bufB`, `bufC` (compute) |
| *s* | *sampler* | | not used here |

The number must match the first argument of the C++ call:
- `VSSetConstantBuffers(3, ...)` binds to `b3`,
- `VSSetShaderResources(0, 3, ...)` binds to `t0`, `t1`, `t2`,
- `CSSetUnorderedAccessViews(0, 3, ...)` binds to `u0`, `u1`, `u2`.

Good to know:
- The counters are **separate per letter**: `b0`, `t0` and `u0` do not conflict.
- Each **stage** (VS, PS, CS) has its own set of slots: the vertex shader's `b0` is independent from the pixel shader's `b0`.
- The skybox also uses `b2` (its `Sky` cbuffer), which is why our `b2` is bound again right before drawing the water.

---

# 3. Code details / Détails du code

## 3.1 What is a mod? `t & (Ns - 1)` / C'est quoi un mod ?

**FR**
**Mod** est le **modulo** : le **reste de la division**. `13 mod 8 = 5`, car `13 = 1 × 8 + 5`. En C++ et en HLSL, c'est l'opérateur `%`.

Dans le shader, on voit aussi `t & (Ns - 1)`. Le `&` est un **ET bit à bit** : il compare deux nombres bit par bit et ne garde que les bits à 1 des deux.
Quand `Ns` est une **puissance de 2**, `t & (Ns - 1)` est **égal à `t % Ns`**, mais plus rapide pour le GPU.

Pourquoi ? Si `Ns = 8 = 1000` en binaire, alors `Ns − 1 = 7 = 0111`. Le ET ne garde que les 3 bits de droite, ce qui est justement le reste de la division par 8.

Exemple avec `t = 13` et `Ns = 8` :

```
t       = 13 = 1101
Ns - 1  =  7 = 0111
t & 7   =  5 = 0101   (= 13 mod 8)
```

Cela ne marche que pour une puissance de 2. Ça tombe bien : `Ns` vaut 1, 2, 4, 8... et `N` est une puissance de 2. C'est aussi pourquoi le code demande `N` puissance de 2.

À quoi sert `k = t & (Ns - 1)` ? C'est la **position du thread dans sa sous-FFT** de taille `Ns`. Elle sert à calculer l'angle de rotation (`PI * k / Ns`) et l'indice d'écriture.

Dans le même code, on trouve aussi de vrais modulos : `(N - m) % N` (case opposée de la grille) et `vertexId % verticesPerChunk` (indice dans le chunk). À côté, `<< 1` décale les bits d'une position vers la gauche, ce qui multiplie par 2.

**EN**
**Mod** is **modulo**: the **remainder of a division**. `13 mod 8 = 5`, because `13 = 1 × 8 + 5`. In C++ and HLSL, it is the `%` operator.

In the shader we also see `t & (Ns - 1)`. The `&` is a **bitwise AND**: it compares two numbers bit by bit and only keeps the bits that are 1 in both.
When `Ns` is a **power of 2**, `t & (Ns - 1)` is **equal to `t % Ns`**, but faster for the GPU.

Why? If `Ns = 8 = 1000` in binary, then `Ns − 1 = 7 = 0111`. The AND only keeps the 3 rightmost bits, which is exactly the remainder of the division by 8.

Example with `t = 13` and `Ns = 8`:

```
t       = 13 = 1101
Ns - 1  =  7 = 0111
t & 7   =  5 = 0101   (= 13 mod 8)
```

This only works for a power of 2. Luckily `Ns` is 1, 2, 4, 8... and `N` is a power of 2. This is also why the code requires `N` to be a power of 2.

What is `k = t & (Ns - 1)` for? It is the **position of the thread inside its sub-FFT** of size `Ns`. It is used to compute the rotation angle (`PI * k / Ns`) and the write index.

The same code also contains real modulos: `(N - m) % N` (opposite cell of the grid) and `vertexId % verticesPerChunk` (index inside the chunk). Next to it, `<< 1` shifts the bits one position to the left, which multiplies by 2.

---

## 3.2 What is a stride in `makeBuffer`? / C'est quoi un stride dans `makeBuffer` ?

**FR**
Le **stride** (« pas ») est le **nombre d'octets entre le début d'un élément et le début du suivant** dans un tableau. C'est la taille d'un élément.

Dans `makeBuffer` :

```cpp
desc.StructureByteStride = 2 * sizeof(float);   // 8 octets = un float2
desc.ByteWidth = count * 2 * sizeof(float);      // taille totale = count × stride
```

Un `float2` fait 2 floats de 4 octets, donc 8 octets. Le GPU en déduit que l'élément `i` est à l'adresse `i × 8`. C'est ce qui permet d'écrire `bufA[i]` dans le shader : D3D sait de combien d'octets avancer pour chaque indice. C'est aussi pourquoi le format de la vue est `DXGI_FORMAT_UNKNOWN` : le stride décrit déjà l'élément.

Le même mot revient ailleurs dans le projet. Dans `directx.cpp`, `UINT stride = sizeof(Plane::Vertex)` est le nombre d'octets d'**un sommet** (6 floats = 24 octets : position + couleur). Il dit au GPU où commence chaque sommet dans le vertex buffer.

Les deux `0` dans `D3D11_SUBRESOURCE_DATA data{initialData, 0, 0}` sont des « pitch » (le pas entre les lignes ou les tranches d'une texture). Ils ne servent à rien pour un buffer.

**EN**
The **stride** is the **number of bytes between the start of one element and the start of the next** in an array. It is the size of one element.

In `makeBuffer`:

```cpp
desc.StructureByteStride = 2 * sizeof(float);   // 8 bytes = one float2
desc.ByteWidth = count * 2 * sizeof(float);      // total size = count × stride
```

A `float2` is 2 floats of 4 bytes, so 8 bytes. The GPU deduces that element `i` is at address `i × 8`. This is what lets you write `bufA[i]` in the shader: D3D knows how many bytes to advance for each index. It is also why the view format is `DXGI_FORMAT_UNKNOWN`: the stride already describes the element.

The same word appears elsewhere in the project. In `directx.cpp`, `UINT stride = sizeof(Plane::Vertex)` is the number of bytes of **one vertex** (6 floats = 24 bytes: position + color). It tells the GPU where each vertex starts in the vertex buffer.

The two `0` in `D3D11_SUBRESOURCE_DATA data{initialData, 0, 0}` are "pitches" (the step between rows or slices of a texture). They are not used for a buffer.
