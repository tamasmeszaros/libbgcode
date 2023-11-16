#include "core/checksum_writer.h"

#include "core/bgcode_impl.hpp"
#include "core/capi_adaptor.hpp"

struct bgcode_checksum_writer_t
    : public bgcode::core::ChecksumWriter<bgcode_output_stream_ref_t> {
  using Base = bgcode::core::ChecksumWriter<bgcode_output_stream_ref_t>;
  static constexpr const bgcode_stream_vtable_t StreamVTable{
      .last_error_description =
          [](const void *self) {
            return bgcode::core::last_error_description(
                *static_cast<const Base *>(self));
          },
      .version =
          [](const void *self) {
            return bgcode::core::stream_bgcode_version(
                *static_cast<const Base *>(self));
          },
      .checksum_type =
          [](const void *self) {
            return bgcode::core::stream_checksum_type(
                *static_cast<const Base *>(self));
          }};

  static constexpr const bgcode_raw_output_stream_vtable_t RawOStreamVTable{
      .write = [](void *self, const unsigned char *buf, size_t len) {
        return bgcode::core::write_to_stream(*static_cast<Base *>(self), buf,
                                             len);
      }};

  static constexpr const bgcode_output_stream_vtable_t OStreamVTable{
      .stream_vtable = &StreamVTable,
      .raw_ostream_vtable = &RawOStreamVTable,
  };

  bgcode_allocator_ref_t allocator;

  bgcode_checksum_writer_t(bgcode_allocator_ref_t alloc,
                           bgcode_checksum_type_t chktype,
                           bgcode_output_stream_ref_t ostream)
      : Base{chktype, ostream}, allocator{alloc} {}

  bgcode_output_stream_ref_t get_output_stream() noexcept {
    return {.vtable = &OStreamVTable, .self = this};
  }
};

bgcode_checksum_writer_t *
bgcode_init_checksum_writer(bgcode_checksum_type_t chktype,
                            const bgcode_block_header_t *block_header,
                            bgcode_output_stream_ref_t ostream) {
  return bgcode_alloc_checksum_writer(bgcode_default_allocator(), chktype,
                                      block_header, ostream);
}

bgcode_checksum_writer_t *
bgcode_alloc_checksum_writer(bgcode_allocator_ref_t alloc,
                             bgcode_checksum_type_t chktype,
                             const bgcode_block_header_t *block_header,
                             bgcode_output_stream_ref_t ostream) {
  auto *ret = bgcode::core::create_bgobj<bgcode_checksum_writer_t>(
      alloc, chktype, ostream);
  if (ret) {
    if (bgcode::core::write(*ret, *block_header) != bgcode_EResult_Success) {
      bgcode_free_checksum_writer(ret);
      ret = nullptr;
    }
  }

  return ret;
}

void bgcode_free_checksum_writer(bgcode_checksum_writer_t *p) {
  bgcode::core::free_bgobj(p);
}

bgcode_output_stream_ref_t
bgcode_get_checksum_writer_ostream(bgcode_checksum_writer_t *chkostream) {
  return chkostream->get_output_stream();
}

void bgcode_get_checksum(bgcode_checksum_writer_t *chkwriter,
                         unsigned char *chkbuf, size_t chksz) {
  auto *chksrc =
      reinterpret_cast<const unsigned char *>(chkwriter->get_checksum().data());
  size_t to_read = std::min(chkwriter->get_checksum().size(), chksz);
  std::copy(chksrc, chksrc + to_read, chkbuf);
}
