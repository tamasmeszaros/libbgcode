#ifndef PARSE_DECOMP_H
#define PARSE_DECOMP_H

#include "LibBGCode/core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parse a block. The payload will be presented to the handler uncompressed.
BGCODE_CORE_EXPORT bgcode_result_t bgcode_parse_block_decompressed(
    bgcode_istream_ref_t stream, const bgcode_block_header_t *block_header,
    bgcode_block_parse_handler_ref_t block_handler);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PARSE_DECOMP_H
