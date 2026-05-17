#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/// PC → text debug map for CHIP-8, plus optional ROM container format (see constants below).
namespace chip8::debug_map {

/// Trailing 8 bytes at end of file: ASCII `C8DB` + `debug_len` (uint32 LE).
constexpr std::size_t kContainerFooterBytes = 8;

constexpr char kFooterMagic[4] = {'C', '8', 'D', 'B'};

/// Inner blob marker (distinct from footer); bytes in file order `C8MP`.
constexpr std::uint32_t kBlobInnerMagic = 0x504D3843u; // LE memory: 'C','8','M','P'

/// Result of probing a file buffer for a container footer.
struct ContainerProbe {
    bool is_container = false;
    /// Bytes to load into CHIP-8 memory from the start of the file.
    std::size_t rom_size = 0;
    /// Byte offset where the debug blob starts (`rom_size`).
    std::size_t debug_offset = 0;
    /// Length of the debug blob (not including the 8-byte footer).
    std::size_t debug_size = 0;
};

/// If `file_size < kContainerFooterBytes` or magic mismatch, returns `is_container == false`
/// and `rom_size == file_size` (load entire buffer as a plain ROM).
[[nodiscard]] ContainerProbe probe_container(const std::uint8_t* data, std::size_t file_size);

/// Parsed map: sorted by `pc` ascending; lookup uses binary search.
class DebugMap {
public:
    DebugMap() = default;
    explicit DebugMap(std::vector<std::pair<std::uint16_t, std::string>> entries);

    [[nodiscard]] bool empty() const { return entries_.empty(); }
    [[nodiscard]] std::size_t size() const { return entries_.size(); }

    /// Exact PC match; nullptr if none.
    [[nodiscard]] const std::string* find(std::uint16_t pc) const;

    [[nodiscard]] const std::vector<std::pair<std::uint16_t, std::string>>& entries() const { return entries_; }

private:
    std::vector<std::pair<std::uint16_t, std::string>> entries_;
};

/// Parse the binary debug blob (contents only; no footer).
[[nodiscard]] std::optional<DebugMap> parse_blob(const std::uint8_t* blob, std::size_t blob_size,
                                                std::string* error_out = nullptr);

/// Encode sorted (or unsorted) PC/text pairs into a blob. Duplicate PCs are rejected.
[[nodiscard]] bool encode_blob(const std::vector<std::pair<std::uint16_t, std::string>>& entries,
                               std::vector<std::uint8_t>& out_blob, std::string* error_out = nullptr);

/// `rom` || `debug_blob` || footer (magic + LE length of debug_blob).
[[nodiscard]] std::vector<std::uint8_t> wrap_container(const std::vector<std::uint8_t>& rom,
                                                     const std::vector<std::uint8_t>& debug_blob);

/// Convenience: encode map and wrap.
[[nodiscard]] bool build_container_file(const std::vector<std::uint8_t>& rom,
                                        const std::vector<std::pair<std::uint16_t, std::string>>& entries,
                                        std::vector<std::uint8_t>& out_file, std::string* error_out = nullptr);

} // namespace chip8::debug_map
