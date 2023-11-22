#ifndef PARSE_DECOMP_H
#define PARSE_DECOMP_H

#include "LibBGCode/core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  void (*const string_value)(void *self, const char *name, const char *str);
  void (*const int_value)(void *self, const char *name, long value);
  void (*const float_value)(void *self, const char *name, double value);

  void (*const begin_object)(void *self, const char *name);
  void (*const end_object)(void *self);

  void (*const begin_object_array)(void *self, const char *name);
  void (*const begin_int_array)(void *self, const char *name);
  void (*const begin_string_array)(void *self, const char *name);
  void (*const begin_float_array)(void *self, const char *name);
  void (*const end_array)(void *self);
} bgcode_metadata_handler_vtable_t;

typedef struct {
  void (*const gcode_line)(void *self, const char *line);
} bgcode_gcode_handler_vtable_t;

typedef struct {
  const bgcode_block_parse_handler_vtable_t *block_handler_vtable;
  const bgcode_metadata_handler_vtable_t *metadata_handler_vtable;
  const bgcode_gcode_handler_vtable_t *gcode_handler_vtable;
} bgcode_handler_vtable_t;

typedef struct {
  const bgcode_handler_vtable_t *vtable;
  void *self;
} bgcode_handler_ref_t;

// Parse a block. The payload will be presented to the handler uncompressed.
BGCODE_CORE_EXPORT bgcode_result_t bgcode_parse_block_decompressed(
    bgcode_istream_ref_t stream, const bgcode_block_header_t *block_header,
    bgcode_handler_ref_t handler, unsigned char *workbuffer,
    size_t workbuf_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PARSE_DECOMP_H
