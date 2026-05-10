#ifndef CHIP8_IMGUI_PANELS_H
#define CHIP8_IMGUI_PANELS_H

class Chip8CPU;
class Chip8Debugger;
class Chip8Memory;

namespace imgui_panels {

/// Rect of the dockspace's central node, in ImGui's display-coordinate space
/// (logical pixels, top-left origin). The frontend maps this onto its GL
/// viewport so the CHIP-8 quad is drawn into the central region only.
struct CentralRect {
    bool valid = false;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

/// Build the dockspace + all debugger panels for this frame. Caller is
/// responsible for `ImGui::NewFrame()` before and `ImGui::Render()` after.
/// Returns the central node's rect so the caller can size the GL render
/// of the CHIP-8 framebuffer to it.
CentralRect build(Chip8Debugger& debugger,
                  Chip8CPU& cpu,
                  const Chip8Memory& memory);

}  // namespace imgui_panels

#endif
