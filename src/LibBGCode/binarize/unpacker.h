#ifndef BGCODE_UNPACKER_H
#define BGCODE_UNPACKER_H

#include "LibBGCode/binarize/export.h"
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

typedef struct bgcode_metadata_handler_ref_t {
  const bgcode_metadata_handler_vtable_t *vtable;
  void *self;
} bgcode_metadata_handler_ref_t;

typedef struct {
  void (*const gcode_line)(void *self, const char *line);
} bgcode_gcode_handler_vtable_t;

typedef struct bgcode_gcode_handler_ref_t {
  const bgcode_gcode_handler_vtable_t *vtable;
  void *self;
} bgcode_gcode_handler_ref_t;

typedef struct bgcode_unpacker_t bgcode_unpacker_t;

BGCODE_BINARIZE_EXPORT bgcode_unpacker_t *
bgcode_init_unpacker(
    bgcode_block_parse_handler_ref_t block_handler,
    bgcode_metadata_handler_ref_t metadata_handler,
    bgcode_gcode_handler_ref_t gcode_handler,
    size_t workbuf_len);

BGCODE_BINARIZE_EXPORT bgcode_unpacker_t *
bgcode_alloc_unpacker(
    bgcode_allocator_ref_t allocator,
    bgcode_block_parse_handler_ref_t block_handler,
    bgcode_metadata_handler_ref_t metadata_handler,
    bgcode_gcode_handler_ref_t gcode_handler,
    size_t workbuf_len);

BGCODE_BINARIZE_EXPORT void bgcode_free_unpacker(bgcode_unpacker_t *unpacker);

BGCODE_BINARIZE_EXPORT bgcode_block_parse_handler_ref_t
bgcode_get_unpacking_block_parse_handler(bgcode_unpacker_t *unpacker);

BGCODE_BINARIZE_EXPORT bgcode_metadata_handler_ref_t bgcode_empty_metadata_handler();
BGCODE_BINARIZE_EXPORT bgcode_gcode_handler_ref_t bgcode_empty_gcode_handler();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PARSE_DECOMP_H
