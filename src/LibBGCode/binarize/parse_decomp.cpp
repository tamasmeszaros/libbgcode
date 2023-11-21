#include "parse_decomp.h"

#include "binarize_impl.hpp"
#include "core/capi_adaptor.hpp"

bgcode_result_t
bgcode_parse_block_decompressed(bgcode_istream_ref_t stream,
                          const bgcode_block_header_t *block_header,
                          bgcode_block_parse_handler_ref_t block_handler) {

  return bgcode::binarize::parse_block_decompressed(stream, *block_header,
                                                    block_handler);
}