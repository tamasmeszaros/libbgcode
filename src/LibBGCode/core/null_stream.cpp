#include "core/null_stream.h"
#include "core/bgcode_impl.hpp"
#include "core/capi_adaptor.hpp"

namespace bgcode {
namespace core {

struct NullStream {
  static const constexpr bgcode_version_t Version = core::VERSION;
  static const constexpr bgcode_checksum_type_t ChecksumType =
      bgcode_EChecksumType_None;

  static const constexpr bgcode_stream_vtable_t StreamVTable =
      bgcode::core::StreamVTableMaker{}
          .last_error_description([](const void * /*self*/) { return ""; })
          .checksum_type([](const void * /*self*/) { return ChecksumType; })
          .version([](const void * /*self*/) { return Version; });

  static const constexpr bgcode_raw_input_stream_vtable_t RawIStreamVTable =
      bgcode::core::RawIStreamVTableMaker{}.read(
          [](void *, unsigned char *, size_t) { return true; });

  static const constexpr bgcode_input_stream_vtable_t IStreamVTable =
      bgcode::core::IStreamVTableMaker{}
          .stream_vtable(&StreamVTable)
          .raw_istream_vtable(&RawIStreamVTable)
          .skip([](void * /*self*/, size_t /*bytes*/) { return true; })
          .is_finished([](const void * /*self*/) { return false; });

  static const constexpr bgcode_raw_output_stream_vtable_t RawOStreamVTable =
      bgcode::core::RawOStreamVTableMaker{}.write(
          [](void *, const unsigned char *, size_t) { return true; });

  static const constexpr bgcode_output_stream_vtable_t OStreamVTable =
      bgcode::core::OStreamVTableMaker{}
          .stream_vtable(&StreamVTable)
          .raw_ostream_vtable(&RawOStreamVTable);

public:
  static bgcode_input_stream_ref_t get_input_stream() {
    return {&IStreamVTable, nullptr};
  }
  static bgcode_output_stream_ref_t get_output_stream() {
    return {&OStreamVTable, nullptr};
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
  return {
      bgcode::core::NullStream::get_input_stream().vtable->raw_istream_vtable,
      nullptr};
}

bgcode_raw_output_stream_ref_t bgcode_get_null_raw_output_stream() {
  return {
      bgcode::core::NullStream::get_output_stream().vtable->raw_ostream_vtable,
      nullptr};
}
