#pragma once

#include <cstddef>

#include "framebuffer.h"

namespace chip8_tui {

inline constexpr int kFbW = Chip8FrameBuffer::WIDTH;
inline constexpr int kFbH = Chip8FrameBuffer::HEIGHT;
inline constexpr int kHalfRows = kFbH / 2;
inline constexpr int kStatusLines = 1;
inline constexpr int kCmdLines = 1;
inline constexpr int kPanelMinCols = 20;
inline constexpr int kMinLogRows = 3;
inline constexpr int kPairLit = 1;
inline constexpr std::size_t kCmdMaxLen = 256;
inline constexpr std::size_t kLogMaxLines = 400;

}  // namespace chip8_tui
