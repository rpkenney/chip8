#ifndef CHIP8_FORMAT_DEBUG_FRAME_H
#define CHIP8_FORMAT_DEBUG_FRAME_H

#include <cstdio>

struct Chip8DebugFrame;

/// Render a `Chip8DebugFrame` as the same multi-line text dump the original
/// `P` keybind produced (registers, current instruction, stack, memory window).
/// Frontend-neutral: requires only a writable `FILE*`. ImGui or other GUI
/// frontends would consume the same struct into widgets instead.
void formatDebugFrame(const Chip8DebugFrame& frame, std::FILE* out);

#endif
