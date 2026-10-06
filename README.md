# Interactive Ocean Surface Simulation

A C++ program that draws a grid of triangles whose heights change every frame, to simulate
the ocean surface. The engine and wave method are chosen in `.config`.

This project is a **prototype** that aims to implement and compare different algorithms for
interactive ocean surface simulation, across several graphics APIs.

> **Status:** the wave methods (`gerstner`, `perlin`, `fft`) are placeholders returning `0`,
> so the surface is flat until they are implemented.

## Roadmap

**Done**
- Initialize the repository
- Open an OpenGL window
- OpenGL plane with editable triangle positions

**In progress**
- .config base plane size
- Vulkan backend
- DirectX backend

**Next**
- Real-time performance metrics
- Stress-tests and Performance logs
- FFT algorithm
- Gerstner algorithm
- Perlin noise algorithm
- Benchmark each graphics library with each algorithm
- Procedural generation (chunks?)

## Build and run

From the project root:

```
make        # build simulation.exe
make run    # build and run
make clean  # delete simulation.exe
```

## Configuration

`.config` holds one `key=value` per line. Lines starting with `#` are comments.

```
engine=opengl     # window backend
width=800         # window width in pixels
height=600        # window height in pixels
method=perlin     # gerstner, perlin or fft
```

## Project structure

```
main.cpp                   reads .config and starts the engine
source/
├── utils/config.*         loads .config
├── calculus/              wave methods (gerstner, perlin, fft) and their lookup table
├── render/plane.*         triangle mesh: update(time) and draw()
└── engines/
    ├── engines.cpp        table of engines
    └── opengl/            window, camera and render loop
```

## Adding a wave method or an engine

- **Wave method:** add the function in `source/calculus/`, declare it in `calculus.hpp`,
  register it in `methods.cpp`, and add the file to `SRCS` in the Makefile.
- **Engine:** add `run(int width, int height, HeightFunction wave)` in `source/engines/`,
  register it in `engines.cpp`, and add its files to `SRCS`.
