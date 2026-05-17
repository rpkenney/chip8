#include <chip8/debug_map/debug_map.h>

#include <algorithm>
#include <cstring>

namespace chip8::debug_map {

namespace {

[[nodiscard]] std::uint32_t read_u32_le(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

[[nodiscard]] std::uint16_t read_u16_le(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) | (static_cast<std::uint16_t>(p[1]) << 8);
}

void write_u32_le(std::uint8_t* p, std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v & 0xFF);
    p[1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
    p[3] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
}

void write_u16_le(std::uint8_t* p, std::uint16_t v) {
    p[0] = static_cast<std::uint8_t>(v & 0xFF);
    p[1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
}

[[nodiscard]] bool footer_magic_matches(const std::uint8_t* footer_start) {
    return std::memcmp(footer_start, kFooterMagic, 4) == 0;
}

/// Read a NUL-terminated string starting at `off` within `pool` of length `pool_size`.
[[nodiscard]] std::optional<std::string> read_pool_cstring(const std::uint8_t* pool, std::size_t pool_size,
                                                           std::uint32_t off, std::string* err) {
    if (off >= pool_size) {
        if (err) {
            *err = "str_off out of range";
        }
        return std::nullopt;
    }
    const std::uint8_t* start = pool + off;
    const std::uint8_t* end = pool + pool_size;
    const std::uint8_t* nul = static_cast<const std::uint8_t*>(std::memchr(start, '\0', end - start));
    if (nul == nullptr) {
        if (err) {
            *err = "unterminated string in pool";
        }
        return std::nullopt;
    }
    return std::string(reinterpret_cast<const char*>(start), static_cast<std::size_t>(nul - start));
}

} // namespace

ContainerProbe probe_container(const std::uint8_t* data, std::size_t file_size) {
    ContainerProbe r;
    r.rom_size = file_size;

    if (data == nullptr || file_size < kContainerFooterBytes) {
        return r;
    }

    const std::uint8_t* footer = data + file_size - kContainerFooterBytes;
    if (!footer_magic_matches(footer)) {
        return r;
    }

    const std::uint32_t debug_len = read_u32_le(footer + 4);
    if (debug_len > file_size - kContainerFooterBytes) {
        return r;
    }

    const std::size_t rom_size = file_size - kContainerFooterBytes - static_cast<std::size_t>(debug_len);
    r.is_container = true;
    r.rom_size = rom_size;
    r.debug_offset = rom_size;
    r.debug_size = static_cast<std::size_t>(debug_len);
    return r;
}

DebugMap::DebugMap(std::vector<std::pair<std::uint16_t, std::string>> entries)
    : entries_(std::move(entries)) {}

const std::string* DebugMap::find(std::uint16_t pc) const {
    auto it = std::lower_bound(entries_.begin(), entries_.end(), pc,
                                 [](const std::pair<std::uint16_t, std::string>& e, std::uint16_t addr) {
                                     return e.first < addr;
                                 });
    if (it != entries_.end() && it->first == pc) {
        return &it->second;
    }
    return nullptr;
}

std::optional<DebugMap> parse_blob(const std::uint8_t* blob, std::size_t blob_size, std::string* error_out) {
    if (blob == nullptr || blob_size < 12) {
        if (error_out) {
            *error_out = "debug blob too small";
        }
        return std::nullopt;
    }

    const std::uint32_t inner = read_u32_le(blob);
    if (inner != kBlobInnerMagic) {
        if (error_out) {
            *error_out = "bad inner_magic";
        }
        return std::nullopt;
    }

    const std::uint32_t num_records = read_u32_le(blob + 4);
    const std::uint32_t pool_size = read_u32_le(blob + 8);

    const std::size_t need = 12u + static_cast<std::size_t>(num_records) * 6u + static_cast<std::size_t>(pool_size);
    if (need > blob_size || need != blob_size) {
        if (error_out) {
            *error_out = "debug blob size mismatch";
        }
        return std::nullopt;
    }

    std::vector<std::pair<std::uint16_t, std::string>> entries;
    entries.reserve(num_records);

    const std::uint8_t* rec_ptr = blob + 12;
    const std::uint8_t* pool = blob + 12 + static_cast<std::size_t>(num_records) * 6u;

    std::uint16_t prev_pc = 0;
    bool has_prev = false;

    for (std::uint32_t i = 0; i < num_records; ++i) {
        const std::uint16_t pc = read_u16_le(rec_ptr);
        rec_ptr += 2;
        const std::uint32_t str_off = read_u32_le(rec_ptr);
        rec_ptr += 4;

        if (has_prev && pc <= prev_pc) {
            if (error_out) {
                *error_out = "records not sorted by pc";
            }
            return std::nullopt;
        }
        has_prev = true;
        prev_pc = pc;

        std::string err;
        auto text = read_pool_cstring(pool, pool_size, str_off, &err);
        if (!text) {
            if (error_out) {
                *error_out = err;
            }
            return std::nullopt;
        }
        entries.emplace_back(pc, std::move(*text));
    }

    return DebugMap(std::move(entries));
}

bool encode_blob(const std::vector<std::pair<std::uint16_t, std::string>>& entries,
                 std::vector<std::uint8_t>& out_blob, std::string* error_out) {
    out_blob.clear();

    std::vector<std::pair<std::uint16_t, std::string>> sorted = entries;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    for (std::size_t i = 1; i < sorted.size(); ++i) {
        if (sorted[i].first == sorted[i - 1].first) {
            if (error_out) {
                *error_out = "duplicate pc in debug entries";
            }
            return false;
        }
    }

    std::vector<std::uint8_t> pool;
    std::vector<std::pair<std::uint16_t, std::uint32_t>> records;
    records.reserve(sorted.size());

    std::vector<std::pair<std::string, std::uint32_t>> string_offsets;
    string_offsets.reserve(sorted.size());

    for (const auto& kv : sorted) {
        const std::string& s = kv.second;
        std::uint32_t off = 0;
        auto it = std::find_if(string_offsets.begin(), string_offsets.end(),
                               [&](const auto& p) { return p.first == s; });
        if (it != string_offsets.end()) {
            off = it->second;
        } else {
            off = static_cast<std::uint32_t>(pool.size());
            for (char c : s) {
                pool.push_back(static_cast<std::uint8_t>(c));
            }
            pool.push_back(0);
            string_offsets.emplace_back(s, off);
        }
        records.emplace_back(kv.first, off);
    }

    const std::uint32_t num_records = static_cast<std::uint32_t>(records.size());
    const std::uint32_t pool_u32 = static_cast<std::uint32_t>(pool.size());

    const std::size_t total =
        12u + static_cast<std::size_t>(num_records) * 6u + static_cast<std::size_t>(pool_u32);
    out_blob.resize(total);

    std::uint8_t* p = out_blob.data();
    write_u32_le(p, kBlobInnerMagic);
    p += 4;
    write_u32_le(p, num_records);
    p += 4;
    write_u32_le(p, pool_u32);
    p += 4;

    for (const auto& rec : records) {
        write_u16_le(p, rec.first);
        p += 2;
        write_u32_le(p, rec.second);
        p += 4;
    }
    std::memcpy(p, pool.data(), pool.size());
    return true;
}

std::vector<std::uint8_t> wrap_container(const std::vector<std::uint8_t>& rom,
                                         const std::vector<std::uint8_t>& debug_blob) {
    std::vector<std::uint8_t> out;
    out.reserve(rom.size() + debug_blob.size() + kContainerFooterBytes);
    out.insert(out.end(), rom.begin(), rom.end());
    out.insert(out.end(), debug_blob.begin(), debug_blob.end());

    std::uint8_t footer[8];
    std::memcpy(footer, kFooterMagic, 4);
    const std::uint32_t len = static_cast<std::uint32_t>(debug_blob.size());
    write_u32_le(footer + 4, len);
    out.insert(out.end(), footer, footer + 8);
    return out;
}

bool build_container_file(const std::vector<std::uint8_t>& rom,
                          const std::vector<std::pair<std::uint16_t, std::string>>& entries, std::vector<std::uint8_t>& out_file,
                          std::string* error_out) {
    std::vector<std::uint8_t> blob;
    if (!encode_blob(entries, blob, error_out)) {
        return false;
    }
    out_file = wrap_container(rom, blob);
    return true;
}

} // namespace chip8::debug_map
