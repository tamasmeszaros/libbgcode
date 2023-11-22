#ifndef BGCODE_H
#define BGCODE_H

#include <stddef.h>

#include "LibBGCode/core/bgcode_defs.h"
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

// Stream API:

typedef struct {
  const char *(*last_error_description)(const void *self);
  bgcode_version_t (*version)(const void *self);
  bgcode_checksum_type_t (*checksum_type)(const void *self);
} bgcode_stream_vtable_t;

typedef struct {
  bool (* read)(void *self, unsigned char *buf, size_t len);
} bgcode_raw_istream_vtable_t;

typedef struct {
  const bgcode_stream_vtable_t * stream_vtable;
  const bgcode_raw_istream_vtable_t *raw_istream_vtable;
  bool (*skip)(void *self, size_t bytes);
  bool (*is_finished)(const void *self);
} bgcode_istream_vtable_t;

typedef struct {
  bool (*write)(void *self, const unsigned char *buf, size_t len);
} bgcode_raw_ostream_vtable_t;

typedef struct {
  const bgcode_stream_vtable_t *stream_vtable;
  const bgcode_raw_ostream_vtable_t *raw_ostream_vtable;
} bgcode_ostream_vtable_t;

typedef struct {
  const bgcode_stream_vtable_t * vtable;
  void * self;
} bgcode_stream_ref_t;

typedef struct {
  const bgcode_raw_istream_vtable_t *vtable;
  void *self;
} bgcode_raw_istream_ref_t;

typedef struct {
  const bgcode_istream_vtable_t *vtable;
  void *self;
} bgcode_istream_ref_t;

typedef struct {
  const bgcode_raw_ostream_vtable_t *vtable;
  void *self;
} bgcode_raw_ostream_ref_t;

typedef struct {
  const bgcode_ostream_vtable_t *vtable;
  void *self;
} bgcode_ostream_ref_t;

typedef struct {
  bgcode_parse_handler_result_t (*handle_block)(
      void *self, bgcode_istream_ref_t stream,
      const bgcode_block_header_t *header);

  bool (*can_continue)(void *self);
} bgcode_parse_handler_vtable_t;

typedef struct {
  const bgcode_parse_handler_vtable_t *vtable;
  void *self;
} bgcode_parse_handler_ref_t;

typedef struct {
  // Max buffer
  size_t (*const payload_chunk_size)(const void *self);
  unsigned char *(*const payload_chunk_buffer)(void *self);

  void (*const int_param)(void *self, const char *name, long value,
                          size_t bytes_width);
  void (*const string_param)(void *self, const char *name, const char *value);
  void (*const float_param)(void *self, const char *name, double value);
  void (*const payload)(void *self, const unsigned char *data_bytes,
                        size_t bytes_count);
  void (*const checksum)(void *self, const unsigned char *checksum_bytes,
                         size_t bytes_count);

  void (*const block_start)(void *self, const bgcode_block_header_t *header);
} bgcode_block_parse_handler_vtable_t;

typedef struct {
  const bgcode_block_parse_handler_vtable_t *const vtable;
  void *const self;
} bgcode_block_parse_handler_ref_t;

BGCODE_CORE_EXPORT bgcode_stream_ref_t
bgcode_get_ostream_base(bgcode_ostream_ref_t ostream);

BGCODE_CORE_EXPORT bgcode_stream_ref_t
bgcode_get_istream_base(bgcode_istream_ref_t ostream);

BGCODE_CORE_EXPORT bgcode_version_t
bgcode_get_stream_version(bgcode_stream_ref_t stream);

BGCODE_CORE_EXPORT bgcode_checksum_type_t
bgcode_get_stream_checksum_type(bgcode_stream_ref_t stream);

BGCODE_CORE_EXPORT const char *
bgcode_get_stream_last_error_str(bgcode_stream_ref_t stream);

BGCODE_CORE_EXPORT bool
bgcode_read_from_stream(bgcode_istream_ref_t istream, unsigned char *buf,
                        size_t len);

BGCODE_CORE_EXPORT bool
bgcode_read_from_raw_stream(bgcode_raw_istream_ref_t istream,
                            unsigned char *buf, size_t len);

BGCODE_CORE_EXPORT bool
bgcode_write_to_stream(bgcode_ostream_ref_t ostream,
                       const unsigned char *buf, size_t len);

BGCODE_CORE_EXPORT bool
bgcode_write_to_raw_stream(bgcode_raw_ostream_ref_t ostream,
                           const unsigned char *buf, size_t len);

BGCODE_CORE_EXPORT bgcode_allocator_ref_t bgcode_default_allocator();

BGCODE_CORE_EXPORT bgcode_allocator_ref_t
bgcode_init_static_allocator(unsigned char *memory, size_t len);

BGCODE_CORE_EXPORT bgcode_stream_header_t *bgcode_init_stream_header();

BGCODE_CORE_EXPORT bgcode_stream_header_t *
bgcode_alloc_stream_header(bgcode_allocator_ref_t alloc);

BGCODE_CORE_EXPORT void
bgcode_free_stream_header(bgcode_stream_header_t *header);

BGCODE_CORE_EXPORT void
bgcode_set_stream_header_version(bgcode_stream_header_t *header,
                                 bgcode_version_t version);

BGCODE_CORE_EXPORT void
bgcode_set_stream_header_checksum_type(bgcode_stream_header_t *header,
                                       bgcode_checksum_type_t checksum_type);

BGCODE_CORE_EXPORT bgcode_version_t
bgcode_get_stream_header_version(bgcode_stream_header_t *header);

BGCODE_CORE_EXPORT bgcode_checksum_type_t
bgcode_get_stream_header_checksum_type(bgcode_stream_header_t *header);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_read_stream_header(
    bgcode_raw_istream_ref_t stream, bgcode_stream_header_t *header);

BGCODE_CORE_EXPORT bgcode_result_t
bgcode_write_stream_header(bgcode_raw_ostream_ref_t stream,
                           const bgcode_stream_header_t *header);

// Read a binary gcode file with the provided handler
BGCODE_CORE_EXPORT bgcode_result_t bgcode_parse(
    bgcode_istream_ref_t stream, bgcode_parse_handler_ref_t handler);

// Read binary gcode file but fail if any block checksum is invalid
BGCODE_CORE_EXPORT bgcode_result_t bgcode_checksum_safe_parse(
    bgcode_istream_ref_t stream, bgcode_parse_handler_ref_t parse_handler,
    unsigned char *checksum_buffer, size_t checksum_buffer_size);

// Skips the block with the given block header.
// If return == EResult::Success:
// - stream position will be set at the start of the next block header.
BGCODE_CORE_EXPORT bgcode_result_t
bgcode_skip_block(bgcode_istream_ref_t stream,
                  const bgcode_block_header_t *block_header);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_parse_block(
    bgcode_istream_ref_t stream, const bgcode_block_header_t *block_header,
    bgcode_block_parse_handler_ref_t block_handler);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_parse_blocks(
    bgcode_istream_ref_t stream,
    bgcode_block_parse_handler_ref_t block_handler);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_checksum_safe_parse_blocks(
    bgcode_istream_ref_t stream,
    bgcode_block_parse_handler_ref_t block_handler,
    unsigned char *checksum_buffer, size_t checksum_buffer_size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BGCODE_H
