#ifndef BGCODE_CHECKSUM_READER_H
#define BGCODE_CHECKSUM_READER_H

#include "core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bgcode_checksum_reader_t;
typedef struct bgcode_checksum_reader_t bgcode_checksum_reader_t;

// BGCODE_CORE_EXPORT bgcode_checksum_reader_t *
// bgcode_init_checksum_reader(bgcode_checksum_type_t chktype,
//                             const bgcode_block_header_t *block_header,
//                             bgcode_istream_ref_t istream);

// BGCODE_CORE_EXPORT bgcode_checksum_reader_t *
// bgcode_alloc_checksum_reader(bgcode_allocator_ref_t alloc,
//                              bgcode_checksum_type_t chktype,
//                              const bgcode_block_header_t *block_header,
//                              bgcode_istream_ref_t istream);

// BGCODE_CORE_EXPORT void
// bgcode_free_checksum_reader(bgcode_checksum_reader_t *ptr);

// BGCODE_CORE_EXPORT bgcode_ostream_ref_t
// bgcode_get_checksum_reader_istream(bgcode_checksum_reader_t *chkistream);

// BGCODE_CORE_EXPORT void bgcode_get_checksum(bgcode_checksum_reader_t *chkreader,
//                                             unsigned char *chkbuf,
//                                             size_t chksz);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // CHECKSUM_READER_H
