#include "core/cfile_stream.h"
#include "core/bgcode_impl.hpp"

struct bgcode_cfile_stream_t : public bgcode::core::CFileStream {
  using Base = bgcode::core::CFileStream;

  static bgcode_checksum_type_t f_checksum_type(const void *self) {
    return bgcode::core::stream_checksum_type(*static_cast<const Base *>(self));
  }

  static const char *f_last_error_description(const void *self) {
    return bgcode::core::last_error_description(
        *static_cast<const Base *>(self));
  }

  static bgcode_version_t f_version(const void *self) {
    return bgcode::core::stream_bgcode_version(
        *static_cast<const Base *>(self));
  }

  static bool f_read(void *self, unsigned char *buf, size_t len) {
    return bgcode::core::read_from_stream(*static_cast<Base *>(self), buf, len);
  }

  static bool f_skip(void *self, size_t bytes) {
    return bgcode::core::skip_stream(*static_cast<Base *>(self), bytes);
  }

  static bool f_is_finished(const void *self) {
    return bgcode::core::is_stream_finished(*static_cast<const Base *>(self));
  }

  static bool f_write(void *self, const unsigned char *buf, size_t len) {
    return bgcode::core::write_to_stream(*static_cast<Base *>(self), buf, len);
  }

  static const constexpr bgcode_stream_vtable_t StreamVtable{
      .last_error_description = f_last_error_description,
      .version = f_version,
      .checksum_type = f_checksum_type,
  };

  static const constexpr bgcode_raw_input_stream_vtable_t RawIStreamVTable{
      .read = f_read,
  };

  static const constexpr bgcode_input_stream_vtable_t IStreamVTable{
      .stream_vtable = &StreamVtable,
      .raw_istream_vtable = &RawIStreamVTable,
      .skip = f_skip,
      .is_finished = f_is_finished};

  static const constexpr bgcode_raw_output_stream_vtable_t RawOStreamVTable{
      .write = f_write};

  static const constexpr bgcode_output_stream_vtable_t OStreamVTable{
      .stream_vtable = &StreamVtable,
      .raw_ostream_vtable = &RawOStreamVTable,
  };

public:
  bgcode_cfile_stream_t(bgcode_allocator_ref_t alloc, FILE *fp,
                        bgcode_checksum_type_t chk_type, bgcode_version_t ver)
      : CFileStream{fp, chk_type, ver}, allocator{alloc} {}

  bgcode_input_stream_ref_t get_input_stream() {
    return {.vtable = &IStreamVTable, .self = this};
  }

  bgcode_output_stream_ref_t get_output_stream() {
    return {.vtable = &OStreamVTable, .self = this};
  }

  bgcode_allocator_ref_t allocator;
};

namespace bgcode {
namespace core {

static const constexpr bgcode_raw_input_stream_vtable_t FILERawIStreamVTable{
    .read = [](void *self, unsigned char *buf, size_t len) {
      FILEInputStream stream{static_cast<FILE *>(self)};
      return read_from_stream(stream, buf, len);
    }};

static const constexpr bgcode_raw_output_stream_vtable_t FILERawOStreamVTable{
    .write = [](void *self, const unsigned char *buf, size_t len) {
      FILEOutputStream stream{static_cast<FILE *>(self)};
      return write_to_stream(stream, buf, len);
    }};

} // namespace core
} // namespace bgcode

bgcode_cfile_stream_t *
bgcode_init_cfile_input_stream(FILE *fp,
                               const bgcode_version_t *const max_version) {
  return bgcode_alloc_cfile_input_stream(bgcode_default_allocator(), fp,
                                         max_version);
}

bgcode_cfile_stream_t *
bgcode_init_cfile_output_stream(FILE *fp, bgcode_checksum_type_t checksum_type,
                                bgcode_version_t version) {
  return bgcode_alloc_cfile_output_stream(bgcode_default_allocator(), fp,
                                          checksum_type, version);
}

bgcode_raw_input_stream_ref_t bgcode_get_cfile_raw_input_stream(FILE *fp) {
  return {.vtable = &bgcode::core::FILERawIStreamVTable, .self = fp};
}

bgcode_raw_output_stream_ref_t bgcode_get_cfile_raw_output_stream(FILE *fp) {
  return {.vtable = &bgcode::core::FILERawOStreamVTable, .self = fp};
}

bgcode_cfile_stream_t *
bgcode_alloc_cfile_input_stream(bgcode_allocator_ref_t allocator, FILE *fp,
                                const bgcode_version_t *const max_version) {
  using namespace bgcode::core;

  if (!allocator.vtable)
    return nullptr;

  if (!fp) {
    return nullptr;
  }

  bgcode_cfile_stream_t *ret = nullptr;

  try {
    bgcode_stream_header_t header;
    FILEInputStream istream{fp};
    auto res = read_header(istream, header, max_version);

    if (res == bgcode_EResult_Success) {
      auto *stream = create_bgobj<bgcode_cfile_stream_t>(
          allocator, fp, header.checksum_type, header.version);

      ret = stream;
    }
  } catch (...) {
    ret = nullptr;
  }

  return ret;
}

bgcode_cfile_stream_t *
bgcode_alloc_cfile_output_stream(bgcode_allocator_ref_t allocator, FILE *fp,
                                 bgcode_checksum_type_t checksum_type,
                                 bgcode_version_t version) {
  using namespace bgcode::core;

  if (!allocator.vtable) {
    return nullptr;
  }

  if (!fp) {
    return nullptr;
  }

  bgcode_cfile_stream_t *ret = nullptr;

  try {
    auto header = bgcode_stream_header_t{}
                      .set_magic(MAGIC)
                      .set_version(version)
                      .set_checksum_type(checksum_type);

    FILEOutputStream istream{fp};

    auto res = write_header(istream, header);

    if (res == bgcode_EResult_Success) {
      ret = create_bgobj<bgcode_cfile_stream_t>(
          allocator, fp, header.checksum_type, header.version);
    }
  } catch (...) {
    ret = nullptr;
  }

  return ret;
}

bgcode_input_stream_ref_t
bgcode_get_cfile_input_stream(bgcode_cfile_stream_t *cfile_stream) {
  return cfile_stream->get_input_stream();
}

bgcode_output_stream_ref_t
bgcode_get_cfile_output_stream(bgcode_cfile_stream_t *cfile_stream) {
  return cfile_stream->get_output_stream();
}

void bgcode_free_cfile_stream(bgcode_cfile_stream_t *cfile_stream) {
  bgcode::core::free_bgobj(cfile_stream);
}
