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

inline EResult read(FILE &file, Checksum &chk)
{
  FILEInputStream istream(&file);
  return static_cast<EResult>(core::read(istream, chk));
}

} // namespace core
} // namespace bgcode

#endif // CORE_IMPL_HPP
