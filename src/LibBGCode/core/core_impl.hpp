#ifndef BGCODE_CORE_IMPL_HPP
#define BGCODE_CORE_IMPL_HPP

#include "core/bgcode_impl.hpp"
#include "core/core.hpp"

namespace bgcode {
namespace core {

// Updates the given checksum with the data of this BlockHeader
inline void update_checksum(Checksum &checksum,
                            const BlockHeader &block_header) {
  checksum.append(block_header.type);
  checksum.append(block_header.compression);
  checksum.append(block_header.uncompressed_size);
  if (block_header.compression != bgcode_ECompressionType_None)
    checksum.append(block_header.compressed_size);
}

inline EResult read(FILE &file, Checksum &chk) {
  FILEInputStream istream(&file);
  return static_cast<EResult>(core::read(istream, chk));
}

inline EResult write(FILE &file, const Checksum &chk) {
  FILEOutputStream ostream(&file);
  return static_cast<EResult>(core::write(ostream, chk));
}

inline bgcode_stream_header_t to_bgcode_header(const FileHeader &hdr) noexcept {
  std::array<char, 4> mg;
  core::store_integer_le(hdr.magic, mg.data());

  return bgcode_stream_header_t{}
      .set_checksum_type(hdr.checksum_type)
      .set_version(hdr.version)
      .set_magic(mg);
}

inline bgcode_block_header_t to_bgcode_header(const BlockHeader &hdr) noexcept {
  return {.type = hdr.type,
          .compression = hdr.compression,
          .uncompressed_size = hdr.uncompressed_size,
          .compressed_size = hdr.compressed_size};
}

} // namespace core
} // namespace bgcode

#endif // CORE_IMPL_HPP
