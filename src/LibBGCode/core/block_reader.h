#ifndef BGCODE_BLOCK_READER_H
#define BGCODE_BLOCK_READER_H

#include "core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bgcode_block_reader_t;
typedef struct bgcode_block_reader_t bgcode_block_reader_t;

BGCODE_CORE_EXPORT bgcode_block_reader_t *
bgcode_alloc_block_reader(bgcode_allocator_ref_t allocator,
                          bgcode_block_parse_handler_ref_t handler);

BGCODE_CORE_EXPORT bgcode_block_reader_t *
bgcode_init_block_reader(bgcode_block_parse_handler_ref_t handler);

BGCODE_CORE_EXPORT void bgcode_free_block_reader(bgcode_block_reader_t *ptr);

BGCODE_CORE_EXPORT bgcode_parse_handler_ref_t
bgcode_get_block_reader_parse_handler(bgcode_block_reader_t *block_reader);


#ifdef __cplusplus
} /* extern C */
#endif

#endif // BLOCK_READER_H
