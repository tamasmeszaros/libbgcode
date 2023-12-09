#ifndef BGCODE_H
#define BGCODE_H

#include <stddef.h>

#include "LibBGCode/core/bgcode_defs.h"
#include "LibBGCode/core/bgcode_lowlevel.h"
#include "LibBGCode/core/export.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get block type from block header. The returned integer is one of
// bgcode_EBlockType values if this version of the library is able to handle the
// block. Otherwise the block can still be skipped.
BGCODE_CORE_EXPORT bgcode_block_type_t
bgcode_get_block_type(const bgcode_block_header_t *header);

BGCODE_CORE_EXPORT bgcode_compression_type_t
bgcode_get_compression_type(const bgcode_block_header_t *header);

BGCODE_CORE_EXPORT bgcode_size_t
bgcode_get_uncompressed_size(const bgcode_block_header_t *header);

BGCODE_CORE_EXPORT bgcode_size_t
bgcode_get_compressed_size(const bgcode_block_header_t *header);

// Returns the size of the content (parameters + data + checksum) of the block
// with the given header, in bytes.
BGCODE_CORE_EXPORT size_t bgcode_block_content_size(
    bgcode_checksum_type_t checksum_type, const bgcode_block_header_t *header);

// Returns the size of the payload (parameters + data) of the block with the
// given header, in bytes.
BGCODE_CORE_EXPORT size_t
bgcode_block_payload_size(const bgcode_block_header_t *block_header);

// Returns a string description of the given result
BGCODE_CORE_EXPORT const char *bgcode_translate_result(bgcode_result_t result);

// Returns the size of the parameters of the given block type, in bytes.
BGCODE_CORE_EXPORT size_t
bgcode_block_parameters_size(bgcode_block_type_t type);

// Returns the size of the checksum of the given type, in bytes.
BGCODE_CORE_EXPORT size_t bgcode_checksum_size(bgcode_checksum_type_t type);

// Highest version of the binary format supported by this library instance
BGCODE_CORE_EXPORT bgcode_version_t bgcode_max_format_version();

// Version of the library
BGCODE_CORE_EXPORT const char *bgcode_version();

typedef struct {
  bgcode_parse_handler_result_t (*handle_block)(
      void *self, bgcode_istream_ref_t stream,
      const bgcode_block_header_t *header);

  bool (*can_continue)(void *self);
} bgcode_parse_handler_vtable_t;

typedef struct {
  const bgcode_parse_handler_vtable_t *vtable;
  void * self;
} bgcode_parse_handler_ref_t;

// Read a binary gcode file with the provided parse handler
BGCODE_CORE_EXPORT bgcode_result_t bgcode_parse(
    bgcode_istream_ref_t stream, bgcode_parse_handler_ref_t handler);

// Skips the block with the given block header.
// If return == EResult::Success:
// - stream position will be set at the start of the next block header.
BGCODE_CORE_EXPORT bgcode_result_t
bgcode_skip_block(bgcode_istream_ref_t stream,
                  const bgcode_block_header_t *block_header);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BGCODE_H
