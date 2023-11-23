#include "unpacker.h"

#include "binarize_impl.hpp"
#include "core/capi_adaptor.hpp"

struct bgcode_unpacker_t : public bgcode::binarize::UnpackingBlockParseHandler<
                               bgcode_block_parse_handler_ref_t> {
  using UnpackingBlockParseHandler<
      bgcode_block_parse_handler_ref_t>::UnpackingBlockParseHandler;
};

bgcode_unpacker_t *
bgcode_alloc_unpacker(bgcode_allocator_ref_t allocator,
                      bgcode_block_parse_handler_ref_t block_handler,
                      bgcode_metadata_handler_ref_t metadata_handler,
                      bgcode_gcode_handler_vtable_t gcode_handler) {
  return bgcode::core::create_bgobj<bgcode_unpacker_t>(
      allocator, block_handler /*, metadata_handler, gcode_handler*/);
}

bgcode_block_parse_handler_ref_t
bgcode_get_unpacking_block_parse_handler(bgcode_unpacker_t *unpacker) {
  return unpacker->get_child_handler();
}

bgcode_result_t
bgcode_parse_block_decompressed(bgcode_istream_ref_t stream,
                                const bgcode_block_header_t *block_header,
                                bgcode_handler_ref_t handler,
                                unsigned char *workbuf, size_t workbuf_len) {

  bgcode_block_parse_handler_ref_t block_handler{
                                                 handler.vtable->block_handler_vtable, handler.self};

  return bgcode::binarize::parse_block_decompressed(
      stream, *block_header, block_handler,
      reinterpret_cast<std::byte *>(workbuf), workbuf_len);
}
