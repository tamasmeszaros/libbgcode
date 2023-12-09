#ifndef BGCODE_LOWLEVEL_H
#define BGCODE_LOWLEVEL_H

#include "LibBGCode/core/export.h"
#include "LibBGCode/core/bgcode_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

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
  void * self;
} bgcode_ostream_ref_t;

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

BGCODE_CORE_EXPORT bgcode_allocator_ref_t bgcode_default_allocator();

BGCODE_CORE_EXPORT bgcode_allocator_ref_t
bgcode_init_static_allocator(unsigned char *memory, size_t len);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // BGCODE_LOWLEVEL_H
