#include "checksum_reader.h"
#include "bgcode_impl.hpp"
#include "capi_adaptor.hpp"

struct bgcode_checksum_reader_t {
  bgcode::core::ChecksumCheckingParseHandler<bgcode_parse_handler_ref_t>
      handler;

  bgcode_checksum_reader_t(bgcode_allocator_ref_t alloc,
                           bgcode_parse_handler_ref_t hdl, std::byte *buf,
                           size_t len)
      : handler{hdl, buf, len}, allocator{alloc} {}

  bgcode_allocator_ref_t allocator;
};

bgcode_checksum_reader_t *
bgcode_alloc_checksum_reader(bgcode_allocator_ref_t allocator,
                             bgcode_parse_handler_ref_t handler,
                             size_t checksum_buffer_size) {

  static constexpr size_t DefaultBufLen = 64;

  bgcode_checksum_reader_t *ret = nullptr;

  try {
    bgcode::core::MRes mres{allocator};
    std::pmr::polymorphic_allocator<std::byte> palloc{&mres};
    if (checksum_buffer_size == 0)
      checksum_buffer_size = DefaultBufLen;

    std::byte *workbuf = palloc.allocate(checksum_buffer_size);

    if (workbuf)
      ret = bgcode::core::create_bgobj<bgcode_checksum_reader_t>(
          allocator, handler, workbuf, checksum_buffer_size);
  } catch (...) {
  }

  return ret;
}

void bgcode_free_checksum_reader(bgcode_checksum_reader_t *ptr) {
  bgcode::core::free_bgobj(ptr);
}

bgcode_checksum_reader_t *
bgcode_init_checksum_reader(bgcode_parse_handler_ref_t handler,
                            size_t checksum_buffer_size) {
  return bgcode_alloc_checksum_reader(bgcode_default_allocator(), handler,
                                      checksum_buffer_size);
}

bgcode_parse_handler_ref_t
bgcode_get_checksum_checking_parse_handler(bgcode_checksum_reader_t *handler) {
  bgcode::core::ParseHandlerVTableAdaptor adaptor{handler->handler};
  return adaptor;
}
