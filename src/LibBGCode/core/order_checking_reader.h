#ifndef BGCODE_ORDER_CHECKING_READER_H
#define BGCODE_ORDER_CHECKING_READER_H

#include "LibBGCode/core/bgcode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bgcode_order_checking_reader_t;
typedef struct bgcode_order_checking_reader_t bgcode_order_checking_reader_t;

BGCODE_CORE_EXPORT bgcode_order_checking_reader_t *
bgcode_alloc_order_checking_reader(bgcode_allocator_ref_t allocator,
                                   bgcode_parse_handler_ref_t handler);

BGCODE_CORE_EXPORT bgcode_order_checking_reader_t *
bgcode_init_order_checking_reader(bgcode_parse_handler_ref_t handler);

BGCODE_CORE_EXPORT void
bgcode_free_order_checking_reader(bgcode_order_checking_reader_t *ptr);

BGCODE_CORE_EXPORT bgcode_parse_handler_ref_t
bgcode_get_order_checking_parse_handler(bgcode_order_checking_reader_t *handler);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // ORDERED_CHECKING_READER_H
