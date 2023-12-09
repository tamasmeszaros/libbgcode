#ifndef BGCODE_BLOCK_READER_H
#define BGCODE_BLOCK_READER_H

#include "LibBGCode/core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bgcode_block_reader_t;
typedef struct bgcode_block_reader_t bgcode_block_reader_t;

typedef struct {
  // Max buffer
  size_t (*payload_chunk_size)(const void *self);
  unsigned char *(*payload_chunk_buffer)(void *self);

  void (*int_param)(void *self, const char *name, long value,
                    size_t bytes_width);
  void (*string_param)(void *self, const char *name, const char *value);
  void (*float_param)(void *self, const char *name, double value);
  void (*payload)(void *self, const unsigned char *data_bytes,
                  size_t bytes_count);
  void (*checksum)(void *self, const unsigned char *checksum_bytes,
                   size_t bytes_count);

  void (*block_start)(void *self, const bgcode_block_header_t *header);

         // Return the status of the handler. Can be used to implement cancellation
         // or skip the currently processed block.
  bgcode_EBlockParseStatus (*status) (const void *self);

} bgcode_block_parse_handler_vtable_t;

typedef struct {
  const bgcode_block_parse_handler_vtable_t *vtable;
  void * self;
} bgcode_block_parse_handler_ref_t;

BGCODE_CORE_EXPORT bgcode_block_parse_handler_vtable_t
bgcode_init_block_parse_handler_vtable(
    bgcode_block_parse_handler_vtable_t prototype);


BGCODE_CORE_EXPORT bgcode_block_reader_t *
bgcode_alloc_block_reader(bgcode_allocator_ref_t allocator,
                          bgcode_block_parse_handler_ref_t handler);

BGCODE_CORE_EXPORT bgcode_block_reader_t *
bgcode_init_block_reader(bgcode_block_parse_handler_ref_t handler);

BGCODE_CORE_EXPORT void bgcode_free_block_reader(bgcode_block_reader_t *ptr);

BGCODE_CORE_EXPORT bgcode_parse_handler_ref_t
bgcode_get_block_reader_parse_handler(bgcode_block_reader_t *block_reader);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_parse_block(
    bgcode_istream_ref_t stream, const bgcode_block_header_t *block_header,
    bgcode_block_parse_handler_ref_t block_handler);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // BLOCK_READER_H
