#ifndef BGCODE_CHECKSUM_WRITER_H
#define BGCODE_CHECKSUM_WRITER_H

#include "LibBGCode/core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bgcode_checksum_writer_t;
typedef struct bgcode_checksum_writer_t bgcode_checksum_writer_t;

BGCODE_CORE_EXPORT bgcode_checksum_writer_t *
bgcode_init_checksum_writer(bgcode_checksum_type_t chktype,
                            const bgcode_block_header_t *block_header,
                            bgcode_ostream_ref_t ostream);

BGCODE_CORE_EXPORT bgcode_checksum_writer_t *
bgcode_alloc_checksum_writer(bgcode_allocator_ref_t alloc,
                             bgcode_checksum_type_t chktype,
                             const bgcode_block_header_t *block_header,
                             bgcode_ostream_ref_t ostream);

BGCODE_CORE_EXPORT void
bgcode_free_checksum_writer(bgcode_checksum_writer_t *ptr);

BGCODE_CORE_EXPORT bgcode_ostream_ref_t
bgcode_get_checksum_writer_ostream(bgcode_checksum_writer_t *chkostream);

BGCODE_CORE_EXPORT void bgcode_get_checksum(bgcode_checksum_writer_t *chkwriter,
                                            unsigned char *chkbuf,
                                            size_t chksz);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
