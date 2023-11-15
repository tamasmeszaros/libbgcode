#ifndef BGCODE_BLOCK_WRITER_H
#define BGCODE_BLOCK_WRITER_H

#include "core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bgcode_block_writer_t;

typedef struct bgcode_block_writer_t bgcode_block_writer_t;

BGCODE_CORE_EXPORT bgcode_block_writer_t *
bgcode_init_block_writer(bgcode_output_stream_ref_t ostream);

BGCODE_CORE_EXPORT bgcode_block_writer_t *
bgcode_alloc_block_writer(bgcode_allocator_ref_t alloc,
                          bgcode_output_stream_ref_t ostream);

BGCODE_CORE_EXPORT void
bgcode_free_block_writer(bgcode_block_writer_t *block_writer);

// Use the underlying stream with caution!
BGCODE_CORE_EXPORT bgcode_output_stream_ref_t
bgcode_get_output_stream(bgcode_block_writer_t *writer);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_start_block(
    bgcode_block_writer_t *writer, bgcode_block_type_t block_type,
    bgcode_compression_type_t compression_type, bgcode_size_t uncompressed_size,
    bgcode_size_t compressed_size);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_start_metadata_block(
    bgcode_block_writer_t *writer, bgcode_block_type_t block_type,
    bgcode_compression_type_t compression_type, bgcode_size_t uncompressed_size,
    bgcode_size_t compressed_size, bgcode_metadata_encoding_type_t encoding);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_start_gcode_block(
    bgcode_block_writer_t *writer, bgcode_compression_type_t compression_type,
    bgcode_size_t uncompressed_size, bgcode_size_t compressed_size,
    bgcode_gcode_encoding_type_t encoding);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_start_thumbnail_block(
    bgcode_block_writer_t *writer, bgcode_compression_type_t compression_type,
    bgcode_size_t uncompressed_size, bgcode_size_t compressed_size,
    bgcode_thumbnail_format_t format, bgcode_thumbnail_size_t width,
    bgcode_thumbnail_size_t height);

BGCODE_CORE_EXPORT bgcode_result_t bgcode_write_block_data(
    bgcode_block_writer_t *writer, const unsigned char *data, size_t len);

BGCODE_CORE_EXPORT bgcode_result_t
bgcode_finish_block(bgcode_block_writer_t *writer);

BGCODE_CORE_EXPORT const unsigned char *
bgcode_block_writer_get_checksum(bgcode_block_writer_t *writer);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BGCODE_BLOCK_WRITER_H
