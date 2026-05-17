// Unit tests for chip8::debug_map (parse/encode/probe + PC lookup).
#include <chip8/debug_map/debug_map.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

int main() {
    using chip8::debug_map::ContainerProbe;
    using chip8::debug_map::DebugMap;
    using chip8::debug_map::encode_blob;
    using chip8::debug_map::parse_blob;
    using chip8::debug_map::probe_container;
    using chip8::debug_map::wrap_container;

    // --- DebugMap::find (sorted PC → string) ---
    {
        DebugMap m({{0x0200, std::string("at_200")},
                    {0x0202, std::string("at_202")}});
        const std::string* a = m.find(0x0200);
        const std::string* b = m.find(0x0202);
        assert(a != nullptr && *a == "at_200");
        assert(b != nullptr && *b == "at_202");
        assert(m.find(0x0201) == nullptr);
    }

    // --- encode_blob ↔ parse_blob roundtrip ---
    {
        std::vector<std::pair<std::uint16_t, std::string>> entries = {
            {0x0204, "four"},
            {0x0200, "zero"},  // intentionally out of order
        };
        std::vector<std::uint8_t> blob;
        std::string enc_err;
        assert(encode_blob(entries, blob, &enc_err));
        std::string parse_err;
        auto parsed = parse_blob(blob.data(), blob.size(), &parse_err);
        assert(parsed.has_value());
        assert(parsed->find(0x0200) != nullptr &&
               *parsed->find(0x0200) == "zero");
        assert(parsed->find(0x0204) != nullptr &&
               *parsed->find(0x0204) == "four");
        assert(parsed->find(0x0202) == nullptr);
    }

    // --- wrap_container + probe: ROM slice and debug blob offset ---
    {
        const std::vector<std::uint8_t> rom = {0x12, 0x34, 0x56, 0x78};
        std::vector<std::pair<std::uint16_t, std::string>> entries = {{0x0200, "main"}};
        std::vector<std::uint8_t> dbg_blob;
        assert(encode_blob(entries, dbg_blob, nullptr));
        const std::vector<std::uint8_t> file = wrap_container(rom, dbg_blob);

        const ContainerProbe p = probe_container(file.data(), file.size());
        assert(p.is_container);
        assert(p.rom_size == rom.size());
        assert(p.debug_size == dbg_blob.size());
        assert(p.debug_offset == rom.size());

        auto map2 = parse_blob(file.data() + p.debug_offset, p.debug_size, nullptr);
        assert(map2.has_value());
        assert(map2->find(0x0200) != nullptr && *map2->find(0x0200) == "main");
    }

    // --- plain ROM: no footer → full file is "ROM" ---
    {
        const std::uint8_t bytes[] = {0x00, 0xE0, 0x12, 0x34};
        const ContainerProbe p = probe_container(bytes, sizeof(bytes));
        assert(!p.is_container);
        assert(p.rom_size == sizeof(bytes));
    }

    return 0;
}
