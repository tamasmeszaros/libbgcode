#include "order_checking_reader.h"
#include "bgcode_impl.hpp"
#include "capi_adaptor.hpp"

struct bgcode_order_checking_reader_t
    : public bgcode::core::OrderCheckingParseHandler<
          bgcode_parse_handler_ref_t> {
  bgcode_order_checking_reader_t(bgcode_allocator_ref_t alloc,
                                 bgcode_parse_handler_ref_t phandler)
      : OrderCheckingParseHandler{phandler}, allocator{alloc} {}

  bgcode_allocator_ref_t allocator;
};

bgcode_order_checking_reader_t *
bgcode_alloc_order_checking_reader(bgcode_allocator_ref_t allocator,
                                   bgcode_parse_handler_ref_t handler) {
  return bgcode::core::create_bgobj<bgcode_order_checking_reader_t>(allocator,
                                                                    handler);
}

bgcode_order_checking_reader_t *
bgcode_init_order_checking_reader(bgcode_parse_handler_ref_t handler) {
  return bgcode_alloc_order_checking_reader(bgcode_default_allocator(),
                                            handler);
}

void bgcode_free_order_checking_reader(bgcode_order_checking_reader_t *ptr) {
  bgcode::core::free_bgobj(ptr);
}

bgcode_parse_handler_ref_t bgcode_get_order_checking_parse_handler(
    bgcode_order_checking_reader_t *ochk_reader) {
  bgcode::core::ParseHandlerVTableAdaptor adaptor{*ochk_reader};

  return adaptor;
}
