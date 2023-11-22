#ifndef PARSE_DECOMP_H
#define PARSE_DECOMP_H

#include "LibBGCode/core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  void (*const on_string)(void *self, const char *name, const char *str);
  void (*const on_integer)(void *self, const char *name, long value);
  void (*const on_floating_point)(void *self, const char *name, double value);

  void (*const on_object_begin)(void *self, const char *name);
  void (*const on_object_end)(void *self);

  void (*const on_int_array)(void *self, const char *name);
  void (*const on_string_array)(void *self, const char *name);
  void (*const on_object_array)(void *self, const char *name);
  void (*const on_floating_point_array)(void *self, const char *name);
  void (*const on_array_end)(void *self);
} bgcode_metadata_handler_vtable_t;

typedef struct {
  void (*const gcode_line)(void *self, const char *line);
} bgcode_gcode_handler_vtable_t;

typedef struct {
  const bgcode_block_parse_handler_vtable_t *block_handler_vtable;
  const bgcode_metadata_handler_vtable_t *metadata_handler_vtable;
  const bgcode_gcode_handler_vtable_t *gcode_handler_vtable;
} bgcode_handler_vtable_t;

// Parse a block. The payload will be presented to the handler uncompressed.
BGCODE_CORE_EXPORT bgcode_result_t bgcode_parse_block_decompressed(
    bgcode_istream_ref_t stream, const bgcode_block_header_t *block_header,
    bgcode_block_parse_handler_ref_t block_handler, unsigned char *workbuffer,
    size_t workbuf_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PARSE_DECOMP_H
