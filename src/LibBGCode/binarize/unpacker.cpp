#include "unpacker.h"

#include "binarize_impl.hpp"
#include "core/capi_adaptor.hpp"

struct bgcode_unpacker_t : public bgcode::binarize::UnpackingBlockParseHandler<
                               bgcode_block_parse_handler_ref_t> {

  bgcode_allocator_ref_t allocator;

  bgcode_unpacker_t(bgcode_allocator_ref_t alloc,
                    bgcode_block_parse_handler_ref_t block_handler,
                    bgcode_metadata_handler_ref_t /*metadata_handler*/,
                    bgcode_gcode_handler_ref_t /*gcode_handler*/,
                    std::byte *workbuf, size_t workbuf_len)
      : UnpackingBlockParseHandler{block_handler,
                                   /*metadata_handler, gcode_handler, */
                                   workbuf, workbuf_len},
        allocator{alloc} {}

  ~bgcode_unpacker_t() {
    if (workbuffer() != nullptr && workbuf_len() > 0) {
      bgcode::core::MRes mres{allocator};
      std::pmr::polymorphic_allocator<std::byte> palloc{&mres};
      palloc.deallocate(workbuffer(), workbuf_len());
    }
  }
};

bgcode_unpacker_t *
bgcode_alloc_unpacker(bgcode_allocator_ref_t allocator,
                      bgcode_block_parse_handler_ref_t block_handler,
                      bgcode_metadata_handler_ref_t metadata_handler,
                      bgcode_gcode_handler_ref_t gcode_handler,
                      size_t workbuf_len) {

  static constexpr size_t DefaultWorkbufLen = 64;
  bgcode_unpacker_t *ret = nullptr;

  try {
    bgcode::core::MRes mres{allocator};
    std::pmr::polymorphic_allocator<std::byte> palloc{&mres};
    if (workbuf_len == 0)
      workbuf_len = DefaultWorkbufLen;

    std::byte *workbuf = palloc.allocate(workbuf_len);

    if (workbuf)
      ret = bgcode::core::create_bgobj<bgcode_unpacker_t>(
          allocator, block_handler, metadata_handler, gcode_handler, workbuf,
          workbuf_len);
  } catch(...) {}

  return ret;
}

bgcode_unpacker_t *
bgcode_init_unpacker(bgcode_block_parse_handler_ref_t block_handler,
                     bgcode_metadata_handler_ref_t metadata_handler,
                     bgcode_gcode_handler_ref_t gcode_handler,
                     size_t workbuf_len) {
  return bgcode_alloc_unpacker(bgcode_default_allocator(), block_handler,
                               metadata_handler, gcode_handler, workbuf_len);
}

bgcode_block_parse_handler_ref_t
bgcode_get_unpacking_block_parse_handler(bgcode_unpacker_t *unpacker) {
  bgcode::core::BlockParseHandlerVTableAdaptor adaptor{*unpacker};
  return adaptor;
}

void bgcode_free_unpacker(bgcode_unpacker_t *unpacker)
{
  bgcode::core::free_bgobj(unpacker);
}

bgcode_metadata_handler_ref_t bgcode_get_empty_metadata_handler()
{
  return {nullptr, nullptr};
}

bgcode_gcode_handler_ref_t bgcode_get_empty_gcode_handler()
{
  return {nullptr, nullptr};
}
