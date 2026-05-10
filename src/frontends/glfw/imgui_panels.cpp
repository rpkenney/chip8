#include "imgui_panels.h"

#include "cpu.h"
#include "debug_frame.h"
#include "debugger.h"
#include "disassemble_chip8.h"
#include "memory.h"

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace imgui_panels {
namespace {

// Fixed layout constants. Right column = debug UI fraction of total width;
// upper half is slightly larger than its lower half.
constexpr float kRightFraction = 0.38f;
constexpr float kRightTopFraction = 0.55f;

void renderRegisters(const Chip8DebugFrame& frame, Chip8CPU& cpu) {
    static Chip8CpuSnapshot prev_regs{};
    static bool have_prev_regs = false;

    if (ImGui::BeginTable("v_regs", 4, ImGuiTableFlags_BordersInnerV)) {
        for (int i = 0; i < 16; ++i) {
            ImGui::TableNextColumn();
            const bool changed =
                have_prev_regs && prev_regs.v[static_cast<std::size_t>(i)] !=
                                      frame.cpu.v[static_cast<std::size_t>(i)];
            if (changed) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 1.0f, 0.88f, 1.0f));
            }
            ImGui::Text("V%X = %02X", i, frame.cpu.v[i]);
            if (changed) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndTable();
    }
    prev_regs = frame.cpu;
    have_prev_regs = true;
    ImGui::Separator();
    ImGui::Text("PC = 0x%04X    I = 0x%03X", frame.cpu.pc, frame.cpu.I);
    ImGui::Text("SP = %u", frame.cpu.sp);
    ImGui::SameLine();
    ImGui::TextDisabled("DT = %u", frame.cpu.dt);
    ImGui::SameLine();
    ImGui::TextDisabled("ST = %u", frame.cpu.st);

    // Debug controls for timers
    ImGui::Separator();
    ImGui::TextDisabled("Debug: Edit Timers");
    static char dt_buf[4] = "0";
    static char st_buf[4] = "0";
    ImGui::SetNextItemWidth(60.0f);
    ImGui::InputText("##dt_input", dt_buf, sizeof(dt_buf), ImGuiInputTextFlags_CharsDecimal);
    ImGui::SameLine();
    if (ImGui::Button("Set DT")) {
        cpu.setDelayTimer(static_cast<std::uint8_t>(std::atoi(dt_buf)));
    }
    ImGui::SetNextItemWidth(60.0f);
    ImGui::InputText("##st_input", st_buf, sizeof(st_buf), ImGuiInputTextFlags_CharsDecimal);
    ImGui::SameLine();
    if (ImGui::Button("Set ST")) {
        cpu.setSoundTimer(static_cast<std::uint8_t>(std::atoi(st_buf)));
    }

    ImGui::Separator();
    ImGui::Text("Instruction: 0x%04X  %s", frame.opcode, frame.mnemonic.c_str());
    ImGui::TextWrapped("%s", frame.description.c_str());
    ImGui::Separator();
    ImGui::Text("Stack:");
    for (int i = 0; i < frame.cpu.sp; ++i) {
        ImGui::SameLine();
        ImGui::Text(" 0x%04X", frame.cpu.stack[i]);
    }
    if (frame.cpu.sp == 0) {
        ImGui::SameLine();
        ImGui::TextDisabled(" (empty)");
    }
}

void renderRecentInstructions(const Chip8DebugFrame& frame) {
    ImGui::TextDisabled("Executed, oldest first (max %u).",
                        static_cast<unsigned>(Chip8Debugger::INSTRUCTION_HISTORY_CAPACITY));
    ImGui::BeginChild("##recent_instr_scroll", ImVec2(0, 0), ImGuiChildFlags_Border);
    if (frame.instruction_history.empty()) {
        ImGui::TextDisabled("(none)");
    } else {
        for (const auto& e : frame.instruction_history) {
            if (e.pc < Chip8Memory::PROGRAM_START) {
                continue;
            }
            const std::string mn = disassembleChip8(e.opcode);
            ImGui::Text("  %04X  %04X  %s", e.pc, e.opcode, mn.c_str());
        }
    }
    ImGui::EndChild();
}

void renderHexdump(const Chip8DebugFrame& frame, const Chip8Memory& memory) {
    static bool center_pc = true;
    ImGui::Checkbox("Center PC", &center_pc);

    ImGui::Text("PC = 0x%04X   %04X  %s", frame.cpu.pc, frame.opcode,
                frame.mnemonic.c_str());

    const std::uint16_t pc = frame.cpu.pc;
    constexpr std::size_t kPerLine = 16;
    const ImVec4 pc_byte_color(0.38f, 0.78f, 0.92f, 1.0f);

    ImGui::BeginChild("##hex_scroll", ImVec2(0, 0), ImGuiChildFlags_Border);
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0) {
        center_pc = false;
    }

    for (std::uint16_t row_base = 0; row_base < Chip8Memory::MEMORY_SIZE;
         row_base = static_cast<std::uint16_t>(row_base + kPerLine)) {
        if (center_pc && pc >= row_base &&
            pc < static_cast<std::uint16_t>(row_base + kPerLine)) {
            ImGui::SetScrollHereY(0.35f);
        }
        ImGui::Text("%04X:", static_cast<unsigned>(row_base));
        for (std::size_t j = 0; j < kPerLine; ++j) {
            const std::uint16_t addr = static_cast<std::uint16_t>(row_base + j);
            if (addr >= Chip8Memory::MEMORY_SIZE) {
                break;
            }
            ImGui::SameLine();
            const bool in_current_insn =
                (addr == pc) ||
                (pc + 1 < Chip8Memory::MEMORY_SIZE &&
                 addr == static_cast<std::uint16_t>(pc + 1));
            if (in_current_insn) {
                ImGui::PushStyleColor(ImGuiCol_Text, pc_byte_color);
            }
            ImGui::Text("%02X", memory.readByte(addr));
            if (in_current_insn) {
                ImGui::PopStyleColor();
            }
        }
    }
    ImGui::EndChild();
}

void renderExecution(Chip8Debugger& debugger) {
    const bool paused = debugger.pacing() == PausePacing::Paused;
    ImGui::Text("%s", paused ? "Paused" : "Running");
    ImGui::Separator();

    if (paused) {
        if (ImGui::Button("Step (Space)")) debugger.requestStep();
        ImGui::SameLine();
        if (ImGui::Button("Step Over (N)")) debugger.requestStepOver();
        ImGui::SameLine();
        if (ImGui::Button("Resume (Enter)")) debugger.requestResume();
    } else {
        if (ImGui::Button("Pause")) debugger.requestPause();
    }

    ImGui::TextDisabled("F1: panels");

    ImGui::Separator();
    static std::uint16_t breakpoint_addr = 0x200;
    ImGui::SetNextItemWidth(72);
    ImGui::InputScalar("Addr", ImGuiDataType_U16, &breakpoint_addr, nullptr, nullptr,
                       "%X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    if (ImGui::Button("Add breakpoint")) {
        debugger.addBreakpoint(static_cast<std::uint16_t>(breakpoint_addr & 0x0FFF));
    }

    ImGui::Separator();
    const float bp_list_h = ImGui::GetTextLineHeightWithSpacing() * 5.5f;
    ImGui::BeginChild("##bp_scroll", ImVec2(0, bp_list_h), ImGuiChildFlags_Border);
    {
        std::vector<std::uint16_t> sorted(debugger.getBreakpoints().begin(),
                                          debugger.getBreakpoints().end());
        std::sort(sorted.begin(), sorted.end());

        std::uint16_t to_remove = 0xFFFF;
        bool removing = false;
        for (std::uint16_t pc : sorted) {
            ImGui::Text("0x%04X", pc);
            ImGui::SameLine();
            char btn[16];
            std::snprintf(btn, sizeof(btn), "X##bp_%04X", pc);
            if (ImGui::SmallButton(btn)) {
                to_remove = pc;
                removing = true;
            }
        }
        if (removing) {
            debugger.removeBreakpoint(to_remove);
        }
        if (sorted.empty()) {
            ImGui::TextDisabled("(none)");
        }
    }
    ImGui::EndChild();
}

}  // namespace

CentralRect build(Chip8Debugger& debugger, Chip8CPU& cpu, const Chip8Memory& memory) {
    const Chip8DebugFrame frame = debugger.captureFrame(cpu, memory);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Host window: no background so the left pane stays transparent for the
    // GL quad. Solid backgrounds are reapplied on the right children below.
    constexpr ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Chip8Layout", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    const float total_w = ImGui::GetContentRegionAvail().x;
    const float total_h = ImGui::GetContentRegionAvail().y;
    const float right_w = total_w * kRightFraction;
    const float left_w = total_w - right_w;

    CentralRect rect{};

    // Left pane: empty + transparent. Carries no ImGui content; the GL quad
    // is drawn into this rect by the frontend.
    ImGui::BeginChild("##left", ImVec2(left_w, total_h), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoBackground);
    {
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        rect.valid = size.x > 0 && size.y > 0;
        rect.x = pos.x;
        rect.y = pos.y;
        rect.w = size.x;
        rect.h = size.y;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right pane: vertical split with a tab bar in each half. Panels paint
    // a fully opaque background using the theme's WindowBg color (forced
    // alpha=1) so they read as solid against the transparent host window.
    ImVec4 panel_bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    panel_bg.w = 1.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_bg);
    ImGui::BeginChild("##right", ImVec2(0, total_h));
    {
        const float right_h = ImGui::GetContentRegionAvail().y;
        const float top_h = right_h * kRightTopFraction;

        ImGui::BeginChild("##right_top", ImVec2(0, top_h), ImGuiChildFlags_Border);
        if (ImGui::BeginTabBar("##right_top_tabs")) {
            if (ImGui::BeginTabItem("Registers")) {
                renderRegisters(frame, cpu);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Hexdump")) {
                renderHexdump(frame, memory);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Recent instrs")) {
                renderRecentInstructions(frame);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();

        ImGui::BeginChild("##right_bottom", ImVec2(0, 0), ImGuiChildFlags_Border);
        ImGui::SeparatorText("Execution");
        renderExecution(debugger);
        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();

    return rect;
}

}  // namespace imgui_panels
