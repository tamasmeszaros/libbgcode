#include "core/checksum_writer.h"

#include "core/bgcode_impl.hpp"
#include "core/capi_adaptor.hpp"

struct bgcode_checksum_writer_t
    : public bgcode::core::ChecksumWriter<bgcode_ostream_ref_t> {

  static constexpr const bgcode_stream_vtable_t StreamVTable =
      bgcode::core::StreamVTableBuilder{}
          .last_error_description([](const void *self) {
            return bgcode::core::last_error_description(
                *static_cast<const ChecksumWriter *>(self));
          })
          .checksum_type([](const void *self) {
            return bgcode::core::stream_checksum_type(
                *static_cast<const ChecksumWriter *>(self));
          })
          .version([](const void *self) {
            return bgcode::core::stream_bgcode_version(
                *static_cast<const ChecksumWriter *>(self));
          });

  static constexpr const bgcode_raw_ostream_vtable_t RawOStreamVTable =
      bgcode::core::RawOStreamVTableBuilder{}.write(
          [](void *self, const unsigned char *buf, size_t len) {
            return bgcode::core::write_to_stream(
                *static_cast<ChecksumWriter *>(self), buf, len);
          });

  static constexpr const bgcode_ostream_vtable_t OStreamVTable =
      bgcode::core::OStreamVTableBuilder{}
          .stream_vtable(&StreamVTable)
          .raw_ostream_vtable(&RawOStreamVTable);

  bgcode_allocator_ref_t allocator;

  bgcode_checksum_writer_t(bgcode_allocator_ref_t alloc,
                           bgcode_checksum_type_t chktype,
                           bgcode_ostream_ref_t ostream)
      : ChecksumWriter{chktype, ostream}, allocator{alloc} {}
  
  bgcode_ostream_ref_t get_output_stream() noexcept {
    return {&OStreamVTable, this};
  }
};

bgcode_checksum_writer_t *
bgcode_init_checksum_writer(bgcode_checksum_type_t chktype,
                            const bgcode_block_header_t *block_header,
                            bgcode_ostream_ref_t ostream) {
  return bgcode_alloc_checksum_writer(bgcode_default_allocator(), chktype,
                                      block_header, ostream);
}

bgcode_checksum_writer_t *
bgcode_alloc_checksum_writer(bgcode_allocator_ref_t alloc,
                             bgcode_checksum_type_t chktype,
                             const bgcode_block_header_t *block_header,
                             bgcode_ostream_ref_t ostream) {
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

bgcode_ostream_ref_t
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
