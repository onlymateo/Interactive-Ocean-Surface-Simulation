# Interactive Ocean Surface Simulation

This project is a **prototype** that aims to implement and compare different algorithms for
interactive ocean surface simulation. It will be used as the prototype
of my thesis subject that I will submit for my PhD application.
So I am trying my best to limit my AI usage on this project in order to fully understand and learn the
maths behind it.

> **Status:** only the `gerstner` method is implemented. `perlin` and `fft` are selectable in the
> config but not implemented yet (the surface stays flat with them).

## Features

- Gerstner waves with several waves, a steepness parameter (sharp crests) and directions, all set in `config.json`
- Chunks around the camera that follow it, to make the ocean look infinite
- Free camera (keyboard)
- Sun lighting on the water (diffuse and shine) and a configurable skybox
- Performance metrics in the window title (FPS, plane updates per second, points per second)
- The waves are recomputed `calculusfrequency` times per second, independently from the FPS and the camera

## Development diary

Notes on what was done, what went wrong, what to do next, and thoughts about the project in [diary/](diary/), one page per date. Latest:
[2026-10-11](diary/2026-10-11.md).

## Roadmap

**Done**
- Initialize the repository
- Open a window
- Plane with editable triangle positions
- Direct3D 11 backend (OpenGL removed)
- Configuration file (plane, camera, window)
- Gerstner algorithm (several waves, steepness)
- Real-time performance metrics
- Chunk generation around the camera
- Camera controller
- Sun lighting and skybox

**Next**
- Perlin noise algorithm
- Log file and a viewer for it
- Stress-tests and performance logs
- FFT algorithm
- Water shader: foam, reflectance, scattered light

## Build and run

You need `g++` (MSYS2 UCRT64), GLFW and [nlohmann/json](https://github.com/nlohmann/json)
(`pacman -S mingw-w64-ucrt-x86_64-nlohmann-json`). From the project root:

```
make        # build simulation.exe
make run    # build and run
make clean  # delete simulation.exe
```

## Controls

| Key | Action |
|---|---|
| Z / S | move forward / backward |
| Q / D | move left / right |
| Shift / Ctrl | move up / down |
| Left / Right arrow | turn left / right |
| Up / Down arrow | look up / down |

The keys are the letters printed on an AZERTY keyboard. The window must have the focus.

## Configuration

Everything is in `config.json`. If a key is missing, its default value is used.

| Section | Keys |
|---|---|
| `window` | `width`, `height` (pixels) |
| `plane` | `cellsnumber` (cells per side of a chunk), `planesize` (side of a chunk), `color`, `basezpos`, `chunksnumber`, `showtriangles` |
| `camera` | `nearplane`, `farplane`, `halfheightatnear`, `cameradistance`, `tiltdegrees` |
| `sun` | `azimuth`, `elevation` (degrees), `color`, `intensity`, `ambient`, `shininess` |
| `sky` | `zenith` and `horizon` colors |
| `calculusfrequency` | how many times per second the waves are recomputed |
| `selectedmethod` | `gerstner`, `perlin` or `fft` (any capitalization) |
| `methods` | the parameters of each method, see below |

`chunksnumber` is the number of chunks, in a square grid: its square root is rounded up
(9 gives 3x3 chunks, 10 gives 4x4).

**Gerstner** (`methods` entry named `gerstner`): one value per wave in each list.

| Key | Meaning |
|---|---|
| `k` | wave number (2π / wavelength) |
| `A` | amplitude |
| `w` | angular frequency (for deep water `w = sqrt(9.81 * k)`) |
| `D` | direction `[x, y]` (normalized when loaded) |
| `s` | steepness: 0 is round, about 1 is sharp. Keep the sum of all the `s` under 1 to avoid loops |

## Project structure

```
main.cpp                   reads config.json and starts the DirectX window
config.json                all the settings
source/
├── utils/config.*         loads config.json
├── calculus/              wave methods (gerstner, perlin, fft)
├── render/plane.*         one mesh (chunk): update(time), moveTo(), vertices() and indices()
└── engines/
    ├── directx.*          Direct3D 11 window, water shader and render loop
    ├── controller.*       keyboard camera
    ├── chunk.*            creates the chunks and moves them with the camera
    ├── lighting.*         the sun
    └── skybox.*           the sky
```

## Adding a wave method

Add the function in `source/calculus/`, declare it in `calculus.hpp`,
call it in `Plane::update` (`source/render/plane.cpp`), add its name to
`selectablemethods` (`source/utils/config.hpp`), and add the file to `SRCS` in the Makefile.
