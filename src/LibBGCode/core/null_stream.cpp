#include "core/null_stream.h"
#include "core/bgcode_impl.hpp"

namespace bgcode {
namespace core {

struct NullStream {
  static const constexpr bgcode_version_t Version = core::VERSION;
  static const constexpr bgcode_checksum_type_t ChecksumType =
      bgcode_EChecksumType_None;

  static const constexpr bgcode_stream_vtable_t StreamVTable = {
      .last_error_description = [](const void *) { return ""; },
      .version = [](const void *) { return Version; },
      .checksum_type = [](const void *) { return ChecksumType; }};

  static const constexpr bgcode_raw_input_stream_vtable_t RawIStreamVTable = {
      .read = [](void *, unsigned char *, size_t) { return true; }};

  static const constexpr bgcode_input_stream_vtable_t IStreamVTable{
      .stream_vtable = &StreamVTable,
      .raw_istream_vtable = &RawIStreamVTable,
      .skip = [](void *self, size_t bytes) { return true; },
      .finished = [](const void *self) { return false; }};

  static const constexpr bgcode_raw_output_stream_vtable_t RawOStreamVTable = {
      .write = [](void *, const unsigned char *, size_t) { return true; }};

  static const constexpr bgcode_output_stream_vtable_t OStreamVTable = {
      .stream_vtable = &StreamVTable, .raw_ostream_vtable = &RawOStreamVTable};

public:
  static bgcode_input_stream_ref_t get_input_stream() {
    return {.vtable = &IStreamVTable, .self = nullptr};
  }
  static bgcode_output_stream_ref_t get_output_stream() {
    return {.vtable = &OStreamVTable, .self = nullptr};
  }
};

} // namespace core
} // namespace bgcode

bgcode_input_stream_ref_t bgcode_get_null_input_stream() {
  return bgcode::core::NullStream::get_input_stream();
}

bgcode_output_stream_ref_t bgcode_get_null_output_stream() {
  return bgcode::core::NullStream::get_output_stream();
}

bgcode_raw_input_stream_ref_t bgcode_get_null_raw_input_stream() {
  return {.vtable = bgcode::core::NullStream::get_input_stream()
                        .vtable->raw_istream_vtable,
          .self = nullptr};
}

bgcode_raw_output_stream_ref_t bgcode_get_null_raw_output_stream() {
  return {.vtable = bgcode::core::NullStream::get_output_stream()
                        .vtable->raw_ostream_vtable,
          .self = nullptr};
}
