#ifndef BGCODE_CHECKSUM_READER_H
#define BGCODE_CHECKSUM_READER_H

#include "core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bgcode_checksum_reader_t;
typedef struct bgcode_checksum_reader_t bgcode_checksum_reader_t;

BGCODE_CORE_EXPORT bgcode_checksum_reader_t *
bgcode_alloc_checksum_reader(bgcode_allocator_ref_t allocator,
                             bgcode_parse_handler_ref_t handler,
                             size_t checksum_buffer_size);

BGCODE_CORE_EXPORT bgcode_checksum_reader_t *
bgcode_init_checksum_reader(bgcode_parse_handler_ref_t handler,
                            size_t checksum_buffer_size);

BGCODE_CORE_EXPORT void
bgcode_free_checksum_reader(bgcode_checksum_reader_t *ptr);

BGCODE_CORE_EXPORT bgcode_parse_handler_ref_t
bgcode_get_checksum_checking_parse_handler(bgcode_checksum_reader_t *handler);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // CHECKSUM_READER_H
