#include <cstring>
#include <memory>

#include <memory_resource>

#include "capi_adaptor.hpp"
#include "bgcode_impl.hpp"

bgcode_block_type_t bgcode_get_block_type(const bgcode_block_header_t *header) {
  return bgcode::core::get_block_type(*header);
}

bgcode_compression_type_t
bgcode_get_compression_type(const bgcode_block_header_t *header) {
  return bgcode::core::get_compression_type(*header);
}

bgcode_size_t
bgcode_get_uncompressed_size(const bgcode_block_header_t *header) {
  return bgcode::core::get_uncompressed_length(*header);
}

bgcode_size_t bgcode_get_compressed_size(const bgcode_block_header_t *header) {
  return bgcode::core::get_compressed_length(*header);
}

size_t bgcode_block_payload_size(const bgcode_block_header_t *header) {
  return bgcode::core::block_payload_length(*header);
}

size_t bgcode_checksum_size(bgcode_checksum_type_t type) {
  return bgcode::core::checksum_length(type);
}

size_t bgcode_block_content_size(bgcode_checksum_type_t checksum_type,
                                 const bgcode_block_header_t *header) {
  return bgcode::core::block_content_length(checksum_type, *header);
}

const char *bgcode_version() { return LibBGCode_VERSION; }

bgcode_version_t bgcode_max_format_version() {
  return bgcode::core::max_bgcode_format_version();
}

const char *bgcode_translate_result(bgcode_result_t result) {
  return bgcode::core::translate_result_code(result);
}

size_t bgcode_block_parameters_size(bgcode_block_type_t type) {
  return bgcode::core::block_parameters_length(type);
}

bgcode_result_t bgcode_parse(bgcode_istream_ref_t stream,
                             bgcode_parse_handler_ref_t handler) {
  bgcode_result_t ret = bgcode_EResult_Success;

  try {
    ret = bgcode::core::parse_stream(stream, handler);
  } catch (...) {
    ret = bgcode_EResult_UnknownError;
  }

  return ret;
}

bgcode_result_t bgcode_skip_block(bgcode_istream_ref_t stream,
                                  const bgcode_block_header_t *block_header) {
  bgcode_result_t ret = bgcode_EResult_Success;

  try {
    ret = bgcode::core::skip_block(stream, *block_header);
  } catch (...) {
    ret = bgcode_EResult_UnknownError;
  }

  return ret;
}

bool bgcode_can_follow_block(bgcode_block_type_t block,
                             bgcode_block_type_t prev_block)
{
  return bgcode::core::can_follow_block(block, prev_block);
}

namespace bgcode { namespace core {

class StaticAllocator {
  std::pmr::monotonic_buffer_resource memres;

  static const bgcode_allocator_vtable_t VTable;

public:
  StaticAllocator(void *buf, size_t size)
      : memres{buf, size, std::pmr::null_memory_resource()} {}

  void *allocate(size_t bytes, size_t alignment) {
    return memres.allocate(bytes, alignment);
  }

  void deallocate(void *ptr, size_t bytes, size_t alignment) {
    memres.deallocate(ptr, bytes, alignment);
  }

  bgcode_allocator_ref_t get_bgcode_allocator() {
    return {&VTable, this};
  }
};

const bgcode_allocator_vtable_t StaticAllocator::VTable =
    bgcode::core::AllocatorVTableBuilder{}
        .allocate([](void *self, size_t bytes, size_t alignment) {
          return static_cast<StaticAllocator *>(self)->allocate(bytes,
                                                                alignment);
        })
        .deallocate([](void *self, void *ptr, size_t bytes, size_t alignment) {
          static_cast<StaticAllocator *>(self)->deallocate(ptr, bytes,
                                                           alignment);
        });

} // namespace core
} // namespace bgcode

namespace {

const bgcode_allocator_vtable_t MallocatorVTable =
    bgcode::core::AllocatorVTableBuilder{}
        .allocate([](void * /*self*/, size_t bytes,
                     size_t alignment) -> void * {
          return std::pmr::get_default_resource()->allocate(bytes, alignment);
        })
        .deallocate([](void * /*self*/, void *ptr, size_t bytes,
                       size_t alignment) {
          return std::pmr::get_default_resource()->deallocate(ptr, bytes,
                                                              alignment);
        });

} // namespace

bgcode_allocator_ref_t bgcode_default_allocator() {
  return {&MallocatorVTable, nullptr};
}

bgcode_allocator_ref_t bgcode_init_static_allocator(unsigned char *memory,
                                                    size_t len) {
  // Using the beginning of 'memory' buffer to store the StaticAllocator
  // instance:
  // 1) Get aligned pointer into memory for StaticAllocator
  // 2) placement new for StaticAllocator and construct it with the remaining
  // memory

  using bgcode::core::StaticAllocator;

  auto *memp = static_cast<void *>(memory);

  auto *p =
      std::align(alignof(StaticAllocator), sizeof(StaticAllocator), memp, len);

  if (p && len > sizeof(StaticAllocator)) {
    auto *allocator = new (p) StaticAllocator(static_cast<unsigned char *>(p) +
                                                  sizeof(StaticAllocator),
                                              len - sizeof(StaticAllocator));

    return allocator->get_bgcode_allocator();
  }

  return {nullptr, nullptr};
}

bgcode_stream_header_t *
bgcode_alloc_stream_header(bgcode_allocator_ref_t alloc) {
  return bgcode::core::create_bgobj<bgcode_stream_header_t>(alloc);
}

bgcode_stream_header_t *bgcode_init_stream_header() {
  return bgcode_alloc_stream_header(bgcode_default_allocator());
}

void bgcode_free_stream_header(bgcode_stream_header_t *header) {
  bgcode::core::free_bgobj(header);
}

void bgcode_set_stream_header_version(bgcode_stream_header_t *header,
                                      bgcode_version_t version) {
  header->set_version(version);
}

void bgcode_set_stream_header_checksum_type(
    bgcode_stream_header_t *header, bgcode_checksum_type_t checksum_type) {
  header->set_checksum_type(checksum_type);
}

bgcode_version_t
bgcode_get_stream_header_version(bgcode_stream_header_t *header) {
  return header->version;
}

bgcode_checksum_type_t
bgcode_get_stream_header_checksum_type(bgcode_stream_header_t *header) {
  return header->checksum_type;
}

bgcode_result_t bgcode_read_stream_header(bgcode_raw_istream_ref_t stream,
                                          bgcode_stream_header_t *header) {
  return bgcode::core::read_header(stream, *header);
}

bgcode_result_t
bgcode_write_stream_header(bgcode_raw_ostream_ref_t stream,
                           const bgcode_stream_header_t *header) {
  return bgcode::core::write_header(stream, *header);
}

bgcode_stream_ref_t
bgcode_get_ostream_base(bgcode_ostream_ref_t ostream) {
  return {ostream.vtable->stream_vtable, ostream.self};
}

bgcode_stream_ref_t bgcode_get_istream_base(bgcode_istream_ref_t istream) {
  return {istream.vtable->stream_vtable, istream.self};
}

bgcode_version_t bgcode_get_stream_version(bgcode_stream_ref_t stream) {
  return bgcode::core::stream_bgcode_version(stream);
}

bgcode_checksum_type_t
bgcode_get_stream_checksum_type(bgcode_stream_ref_t stream) {
  return bgcode::core::stream_checksum_type(stream);
}

const char *bgcode_get_stream_last_error_str(bgcode_stream_ref_t stream) {
  return bgcode::core::last_error_description(stream);
}

bool bgcode_read_from_stream(bgcode_istream_ref_t istream,
                             unsigned char *buf, size_t sz) {
  return bgcode::core::read_from_stream(istream, buf, sz);
}

bool bgcode_read_from_raw_stream(bgcode_raw_istream_ref_t istream,
                                 unsigned char *buf, size_t sz) {
  return bgcode::core::read_from_stream(istream, buf, sz);
}

bool bgcode_write_to_stream(bgcode_ostream_ref_t ostream,
                            const unsigned char *buf, size_t sz) {
  return bgcode::core::write_to_stream(ostream, buf, sz);
}

bool bgcode_write_to_raw_stream(bgcode_raw_ostream_ref_t ostream,
                                const unsigned char *buf, size_t sz) {
  return bgcode::core::write_to_stream(ostream, buf, sz);
}

bgcode_block_parse_handler_vtable_t bgcode_init_block_parse_handler_vtable(
    bgcode_block_parse_handler_vtable_t prototype) {

  bgcode_block_parse_handler_vtable_t ret =
      bgcode::core::BlockParseHandlerVTableBuilder{};
  if (prototype.block_start)
    ret.block_start = prototype.block_start;
  if (prototype.int_param)
    ret.int_param = prototype.int_param;
  if (prototype.float_param)
    ret.float_param = prototype.float_param;
  if (prototype.string_param)
    ret.string_param = prototype.string_param;
  if (prototype.checksum)
    ret.checksum = prototype.checksum;
  if (prototype.payload)
    ret.payload = prototype.payload;
  if (prototype.payload_chunk_buffer)
    ret.payload_chunk_buffer = prototype.payload_chunk_buffer;
  if (prototype.payload_chunk_size)
    ret.payload_chunk_size = prototype.payload_chunk_size;
  if (prototype.status)
    ret.status = prototype.status;

  return ret;
}
