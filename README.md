# CHIP-8 emulator

## Build and run a ROM

**Stack:** **OpenGL** talks to the GPU to draw the framebuffer. **GLFW** opens the window and reads input. **GLAD** loads OpenGL 3.3 Core entry points at runtime.

**Dependencies:**

- CMake 3.16+
- C++17 compiler
- GLFW 3
- OpenGL dev libraries (headers + GL link library). On Linux this usually comes from the Mesa stack; GPU vendors ship their own drivers, but you still need a dev install that satisfies the build.
- [GLAD](https://glad.dav1d.de/) for OpenGL 3.3 Core (C loader)

If `src/glad.c` or `include/glad/` is missing after clone, regenerate GLAD (3.3 Core), put `glad.c` in `src/`, and put the `glad` and `KHR` trees under `include/`. **`KHR`** is Khronos-supplied glue (e.g. `khrplatform.h`) that GLAD’s headers include.

**Install toolchain and libs (Debian / Ubuntu example):**

```bash
sudo apt install build-essential cmake libglfw3-dev libgl1-mesa-dev
```

Adjust package names if you use another distro.

**Configure and build:**

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

- `-S .` — project root (where `CMakeLists.txt` lives).
- `-B build` — out-of-tree build directory.
- `-DCMAKE_BUILD_TYPE=Release` — optimized build (for single-config generators like Make/Ninja). For a debug build, use `Debug` instead.

Reconfigure after changing `CMakeLists.txt` by running the first `cmake` line again (or delete `build/` and start over).

**Run:**

Binary: `build/chip8`. Pass the ROM path (and optional flags such as `--trace`, `--step`, `-b breakpoints.txt`). While running, **P** prints registers and memory around PC to stderr when instruction pacing is **manual** (`--step`, or after a breakpoint). Close the window to quit.

```bash
./build/chip8 roms/test3
```

Quick smoke test (process should stay up; exit code 124 means `timeout` stopped it after 3 seconds):

```bash
timeout 3 ./build/chip8 roms/test3
```
