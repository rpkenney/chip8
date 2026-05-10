#include "tui_commands.h"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>

#include "cpu.h"
#include "debug_frame.h"
#include "debugger.h"
#include "disassemble_chip8.h"
#include "memory.h"

#include "tui_support.h"

namespace chip8_tui {

void dispatchTuiCommand(const std::string& line, std::vector<std::string>& log, bool& quit,
                        TuiPagerState& pager, int log_view_rows, Chip8Debugger& debugger,
                        Chip8CPU& cpu, Chip8Memory& memory) {
    const std::string t = trimCopy(line);
    if (t.empty()) {
        return;
    }
    std::istringstream iss(t);
    std::string cmd;
    iss >> cmd;
    toLowerAscii(cmd);

    if (cmd == "h" || cmd == "?") {
        appendLogMultiline(log,
                           "commands: h ? | regs | mem <addr> [n] | hist [n] | pause | cont | step | "
                           "next|n | q quit | less\n"
                           "breakpoints: b list | b add <addr> | b del <addr>\n"
                           "less: scroll log (PgUp/PgDn Home/End j/k g/G, q or Esc exit)\n"
                           "addr: hex 0x.. or decimal. empty line+Enter = resume. space = step when "
                           "line empty.\n"
                           "while running: F1 = pause (p is for typing / CHIP-8).");
        return;
    }
    if (cmd == "q" || cmd == "quit") {
        std::string rest;
        std::getline(iss, rest);
        if (trimCopy(rest).empty()) {
            quit = true;
        } else {
            appendLogLine(log, "use: q   or   quit   (no extra args)");
        }
        return;
    }
    if (cmd == "less" || cmd == "page") {
        pager.active = true;
        pager.first_visible = logMaxFirstIndex(static_cast<int>(log.size()), log_view_rows);
        clampPagerFirst(pager, log_view_rows, static_cast<int>(log.size()));
        return;
    }
    if (cmd == "pause") {
        std::string rest;
        std::getline(iss, rest);
        if (!trimCopy(rest).empty()) {
            appendLogLine(log, "pause: no arguments");
            return;
        }
        debugger.requestPause();
        appendLogLine(log, "(paused)");
        return;
    }
    if (cmd == "cont") {
        pager.active = false;
        debugger.requestResume();
        appendLogLine(log, "(running)");
        return;
    }
    if (cmd == "step") {
        debugger.requestStep();
        appendLogLine(log, "(step)");
        return;
    }
    if (cmd == "next" || cmd == "n") {
        std::string rest;
        std::getline(iss, rest);
        if (!trimCopy(rest).empty()) {
            appendLogLine(log, "next (or n): no arguments");
            return;
        }
        debugger.requestStepOver();
        appendLogLine(log, "(step over)");
        return;
    }
    if (cmd == "regs") {
        const Chip8DebugFrame frame = debugger.captureFrame(cpu, memory);
        char buf[96];
        for (int i = 0; i < 16; ++i) {
            std::snprintf(buf, sizeof(buf), "V%X = %02X", i,
                          static_cast<unsigned>(frame.cpu.v[static_cast<std::size_t>(i)]));
            appendLogLine(log, buf);
        }
        std::snprintf(buf, sizeof(buf), "PC=%04X I=%03X SP=%u DT=%u ST=%u", frame.cpu.pc, frame.cpu.I,
                      static_cast<unsigned>(frame.cpu.sp), static_cast<unsigned>(frame.cpu.dt),
                      static_cast<unsigned>(frame.cpu.st));
        appendLogLine(log, buf);
        std::snprintf(buf, sizeof(buf), "next: %04X  %s", frame.opcode, frame.mnemonic.c_str());
        appendLogLine(log, buf);
        return;
    }
    if (cmd == "hist") {
        std::string ntok;
        iss >> ntok;
        const Chip8DebugFrame frame = debugger.captureFrame(cpu, memory);
        int n = 20;
        if (!ntok.empty()) {
            try {
                n = std::stoi(ntok);
            } catch (...) {
                appendLogLine(log, "hist: bad count");
                return;
            }
        }
        n = std::max(1, std::min(n, 100));
        const auto& h = frame.instruction_history;
        if (h.empty()) {
            appendLogLine(log, "(no history)");
            return;
        }
        std::vector<std::string> acc;
        acc.reserve(h.size());
        for (const auto& e : h) {
            if (e.pc < Chip8Memory::PROGRAM_START) {
                continue;
            }
            char buf[80];
            std::snprintf(buf, sizeof(buf), "%04X  %04X  %s", e.pc, e.opcode,
                          disassembleChip8(e.opcode).c_str());
            acc.push_back(buf);
        }
        if (acc.empty()) {
            appendLogLine(log, "(no history)");
            return;
        }
        const int want = std::min(n, static_cast<int>(acc.size()));
        const std::size_t start = acc.size() - static_cast<std::size_t>(want);
        for (std::size_t i = start; i < acc.size(); ++i) {
            appendLogLine(log, acc[i]);
        }
        return;
    }
    if (cmd == "mem") {
        std::string atok;
        iss >> atok;
        std::string ntok;
        iss >> ntok;
        if (atok.empty()) {
            appendLogLine(log, "mem: need address (mem 0x200 [16])");
            return;
        }
        std::uint16_t addr = 0;
        if (!parseU16(atok, addr)) {
            appendLogLine(log, "mem: bad address");
            return;
        }
        std::size_t count = 16;
        if (!ntok.empty()) {
            try {
                const unsigned long c = std::stoul(ntok, nullptr, 10);
                count = static_cast<std::size_t>(std::min(c, 256UL));
            } catch (...) {
                appendLogLine(log, "mem: bad length");
                return;
            }
        }
        count = std::max<std::size_t>(1, count);
        std::uint32_t cur = addr;
        std::size_t remaining = count;
        while (remaining > 0 && cur < Chip8Memory::MEMORY_SIZE) {
            const std::size_t space =
                static_cast<std::size_t>(Chip8Memory::MEMORY_SIZE - static_cast<std::uint16_t>(cur));
            const std::size_t row_len = (std::min)({std::size_t{16}, remaining, space});
            if (row_len == 0) {
                break;
            }
            std::string line;
            char head[16];
            std::snprintf(head, sizeof(head), "%04X:", static_cast<unsigned>(cur & 0xFFFFU));
            line = head;
            for (std::size_t j = 0; j < row_len; ++j) {
                const std::uint16_t a = static_cast<std::uint16_t>(cur + j);
                char b[8];
                std::snprintf(b, sizeof(b), " %02X", static_cast<unsigned>(memory.readByte(a)));
                line += b;
            }
            appendLogLine(log, line);
            cur += static_cast<std::uint32_t>(row_len);
            remaining -= row_len;
        }
        return;
    }
    if (cmd == "b" || cmd == "break") {
        std::string sub;
        iss >> sub;
        toLowerAscii(sub);
        if (sub.empty() || sub == "list" || sub == "ls") {
            const auto& bps = debugger.getBreakpoints();
            if (bps.empty()) {
                appendLogLine(log, "(no breakpoints)");
            } else {
                std::vector<std::uint16_t> sorted(bps.begin(), bps.end());
                std::sort(sorted.begin(), sorted.end());
                for (const std::uint16_t a : sorted) {
                    char buf[16];
                    std::snprintf(buf, sizeof(buf), "  %04X", static_cast<unsigned>(a));
                    appendLogLine(log, buf);
                }
            }
            return;
        }
        if (sub == "add" || sub == "set") {
            std::string atok;
            iss >> atok;
            if (atok.empty()) {
                appendLogLine(log, "b add: need address");
                return;
            }
            std::uint16_t addr = 0;
            if (!parseU16(atok, addr)) {
                appendLogLine(log, "b add: bad address");
                return;
            }
            debugger.addBreakpoint(addr);
            char buf[40];
            std::snprintf(buf, sizeof(buf), "breakpoint +%04X", static_cast<unsigned>(addr));
            appendLogLine(log, buf);
            return;
        }
        if (sub == "del" || sub == "rm" || sub == "clear" || sub == "remove") {
            std::string atok;
            iss >> atok;
            if (atok.empty()) {
                appendLogLine(log, "b del: need address");
                return;
            }
            std::uint16_t addr = 0;
            if (!parseU16(atok, addr)) {
                appendLogLine(log, "b del: bad address");
                return;
            }
            debugger.removeBreakpoint(addr);
            char buf[40];
            std::snprintf(buf, sizeof(buf), "breakpoint -%04X", static_cast<unsigned>(addr));
            appendLogLine(log, buf);
            return;
        }
        appendLogLine(log, "b: b list | b add <addr> | b del <addr>");
        return;
    }

    appendLogLine(log, "unknown command (h for help)");
}

}  // namespace chip8_tui
