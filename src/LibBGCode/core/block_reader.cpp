#include "block_reader.h"
#include "bgcode_impl.hpp"
#include "capi_adaptor.hpp"

struct bgcode_block_reader_t : public bgcode::core::AllBlocksParseHandler<
                                   bgcode_block_parse_handler_ref_t> {

  bgcode_block_reader_t(bgcode_allocator_ref_t alloc,
                        bgcode_block_parse_handler_ref_t hdl)
      : AllBlocksParseHandler{hdl}, allocator{alloc} {}

  bgcode_allocator_ref_t allocator;
};

bgcode_block_reader_t *
bgcode_alloc_block_reader(bgcode_allocator_ref_t allocator,
                          bgcode_block_parse_handler_ref_t handler) {
  bgcode_block_reader_t *ret = nullptr;

  try {
    ret = bgcode::core::create_bgobj<bgcode_block_reader_t>(allocator, handler);
  } catch (...) {
  }

  return ret;
}

bgcode_block_reader_t *
bgcode_init_block_reader(bgcode_block_parse_handler_ref_t handler) {
  return bgcode_alloc_block_reader(bgcode_default_allocator(), handler);
}

void bgcode_free_block_reader(bgcode_block_reader_t *ptr) {
  bgcode::core::free_bgobj(ptr);
}

bgcode_parse_handler_ref_t
bgcode_get_block_reader_parse_handler(bgcode_block_reader_t *block_reader) {
  bgcode::core::ParseHandlerVTableAdaptor adaptor{*block_reader};

  return adaptor;
}

bgcode_result_t
bgcode_parse_block(bgcode_istream_ref_t stream,
                   const bgcode_block_header_t *block_header,
                   bgcode_block_parse_handler_ref_t block_handler) {
  bgcode_result_t ret = bgcode_EResult_Success;

  try {
    ret = bgcode::core::parse_block(stream, *block_header, block_handler);
  } catch (...) {
    ret = bgcode_EResult_UnknownError;
  }

  return ret;
}
