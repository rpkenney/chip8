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

**Configure and build:**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

`-S .` is the source directory (project root). `-B build` is the build directory (CMake writes build files there). `-DCMAKE_BUILD_TYPE=Release` sets the cache variable for an optimized, non-debug build (typical for Makefile/Ninja generators). `cmake --build build` compiles using whatever generator was configured for that directory.

Binary: `build/chip8`. Run with the ROM path as the first argument; close the window to quit.

```bash
./build/chip8 <rom>
```
