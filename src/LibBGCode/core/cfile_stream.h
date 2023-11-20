#ifndef BGCODE_CFILE_STREAM_H
#define BGCODE_CFILE_STREAM_H

#include <stdio.h>

#include "LibBGCode/core/bgcode.h"
#include "LibBGCode/core/export.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bgcode_cfile_stream_t;

typedef struct bgcode_cfile_stream_t bgcode_cfile_stream_t;

BGCODE_CORE_EXPORT bgcode_cfile_stream_t *
bgcode_init_cfile_input_stream(FILE *fp,
                               const bgcode_version_t *const max_version);

BGCODE_CORE_EXPORT bgcode_cfile_stream_t *
bgcode_alloc_cfile_input_stream(bgcode_allocator_ref_t allocator, FILE *fp,
                                const bgcode_version_t *const max_version);

BGCODE_CORE_EXPORT bgcode_cfile_stream_t *
bgcode_init_cfile_output_stream(FILE *fp, bgcode_checksum_type_t checksum_type,
                                bgcode_version_t version);

BGCODE_CORE_EXPORT bgcode_cfile_stream_t *
bgcode_alloc_cfile_output_stream(bgcode_allocator_ref_t allocator, FILE *fp,
                                 bgcode_checksum_type_t checksum_type,
                                 bgcode_version_t version);

BGCODE_CORE_EXPORT void
bgcode_free_cfile_stream(bgcode_cfile_stream_t *cfile_stream);

BGCODE_CORE_EXPORT bgcode_raw_istream_ref_t
bgcode_get_cfile_raw_input_stream(FILE *fp);

BGCODE_CORE_EXPORT bgcode_raw_ostream_ref_t
bgcode_get_cfile_raw_output_stream(FILE *fp);

BGCODE_CORE_EXPORT bgcode_istream_ref_t
bgcode_get_cfile_input_stream(bgcode_cfile_stream_t *cfile_stream);

BGCODE_CORE_EXPORT bgcode_ostream_ref_t
bgcode_get_cfile_output_stream(bgcode_cfile_stream_t *cfile_stream);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CFILE_STREAM_H
