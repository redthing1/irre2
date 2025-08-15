#include "object.hpp"
#include "../util.hpp"
#include <cstring>

namespace irre::assembler {

std::vector<byte> object_file::to_binary() const {
  std::vector<byte> result;

  // calculate total size: 24 byte header + code + data
  size_t header_size = 24;
  size_t total_size = header_size + code.size() + data.size();

  result.reserve(total_size);

  // write header using portable utilities
  byte_io::write_magic(result);                                      // [0-3]: magic 'RGVM'
  byte_io::write_u16_le(result, version);                            // [4-5]: version
  byte_io::write_u16_le(result, 0);                                  // [6-7]: reserved
  byte_io::write_u32_le(result, entry_offset);                       // [8-11]: entry offset
  byte_io::write_u32_le(result, static_cast<uint32_t>(code.size())); // [12-15]: code size
  byte_io::write_u32_le(result, static_cast<uint32_t>(data.size())); // [16-19]: data size
  byte_io::write_u32_le(result, 0);                                  // [20-23]: reserved

  // write sections
  result.insert(result.end(), code.begin(), code.end());
  result.insert(result.end(), data.begin(), data.end());

  return result;
}

result<object_file, std::string> object_file::from_binary(const std::vector<byte>& binary) {
  // validate minimum size for header
  if (binary.empty()) {
    return std::string("error: empty file - cannot load object file from empty data");
  }

  if (binary.size() < 24) {
    char msg[128];
    std::snprintf(
        msg, sizeof(msg), "error: file too small (%zu bytes) - IRRE object files require at least 24 bytes for header",
        binary.size()
    );
    return std::string(msg);
  }

  const byte* data = binary.data();

  // check magic with detailed feedback
  if (!byte_io::check_magic(data)) {
    char actual_magic[5] = {0};
    std::memcpy(actual_magic, data, 4);
    char msg[128];
    std::snprintf(
        msg, sizeof(msg),
        "error: invalid magic bytes '%.4s' (0x%02x%02x%02x%02x) - expected 'RGVM' for IRRE object file", actual_magic,
        data[0], data[1], data[2], data[3]
    );
    return std::string(msg);
  }

  // read header fields
  uint16_t file_version = byte_io::read_u16_le(data + 4);
  if (file_version != version) {
    char msg[128];
    std::snprintf(
        msg, sizeof(msg), "error: unsupported version %u - this loader supports version %u", file_version, version
    );
    return std::string(msg);
  }

  uint32_t entry_offset = byte_io::read_u32_le(data + 8);
  uint32_t code_size = byte_io::read_u32_le(data + 12);
  uint32_t data_size = byte_io::read_u32_le(data + 16);

  // validate section sizes
  if (code_size > 0x1000000) { // 16MB limit
    char msg[128];
    std::snprintf(msg, sizeof(msg), "error: code section too large (%u bytes) - maximum is 16MB", code_size);
    return std::string(msg);
  }

  if (data_size > 0x1000000) { // 16MB limit
    char msg[128];
    std::snprintf(msg, sizeof(msg), "error: data section too large (%u bytes) - maximum is 16MB", data_size);
    return std::string(msg);
  }

  // validate file size
  size_t expected_size = 24 + code_size + data_size;
  if (binary.size() != expected_size) {
    char msg[256];
    std::snprintf(
        msg, sizeof(msg),
        "error: file size mismatch - got %zu bytes, expected %zu bytes (24 header + %u code + %u data)", binary.size(),
        expected_size, code_size, data_size
    );
    return std::string(msg);
  }

  // validate that entry offset is within code section
  if (code_size > 0 && entry_offset >= code_size) {
    char msg[128];
    std::snprintf(
        msg, sizeof(msg), "error: entry point at offset %u is outside code section (size %u bytes)", entry_offset,
        code_size
    );
    return std::string(msg);
  }

  // validate entry point alignment (instructions are 4-byte aligned)
  if (entry_offset % 4 != 0) {
    char msg[128];
    std::snprintf(
        msg, sizeof(msg), "error: entry point at offset %u is not 4-byte aligned (instructions must be aligned)",
        entry_offset
    );
    return std::string(msg);
  }

  // build result
  object_file result;
  result.entry_offset = entry_offset;

  // read sections
  size_t offset = 24;
  if (code_size > 0) {
    result.code.assign(binary.begin() + offset, binary.begin() + offset + code_size);
    offset += code_size;
  }

  if (data_size > 0) {
    result.data.assign(binary.begin() + offset, binary.begin() + offset + data_size);
  }

  return result;
}

} // namespace irre::assembler