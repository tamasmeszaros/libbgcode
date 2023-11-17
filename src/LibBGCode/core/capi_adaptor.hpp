#ifndef BGCODE_CAPI_STREAM_TRAITS_HPP
#define BGCODE_CAPI_STREAM_TRAITS_HPP

#include "core/bgcode.h"
#include "core/bgcode_cxx_traits.hpp"

#include <memory_resource>

namespace bgcode {
namespace core {

class StreamVTableMaker {
  bgcode_stream_vtable_t vtable;

public:
  constexpr StreamVTableMaker() : vtable{nullptr, nullptr, nullptr} {}

  constexpr StreamVTableMaker &
  last_error_description(decltype(vtable.last_error_description) fn) {
    vtable.last_error_description = fn;
    return *this;
  }

  constexpr StreamVTableMaker &
  checksum_type(decltype(vtable.checksum_type) fn) {
    vtable.checksum_type = fn;
    return *this;
  }

  constexpr StreamVTableMaker &
  version(decltype(vtable.version) fn) {
    vtable.version = fn;
    return *this;
  }

  constexpr operator const bgcode_stream_vtable_t&() const { return vtable; }
};

class RawIStreamVTableMaker {
  bgcode_raw_input_stream_vtable_t vtable;

public:
  constexpr RawIStreamVTableMaker &read(decltype(vtable.read) fn) {
    vtable.read = fn;
    return *this;
  }
  constexpr operator const bgcode_raw_input_stream_vtable_t &() const {
    return vtable;
  }
};

class RawOStreamVTableMaker {
  bgcode_raw_output_stream_vtable_t vtable;

public:
  constexpr RawOStreamVTableMaker &write(decltype(vtable.write) fn) {
    vtable.write = fn;
    return *this;
  }
  constexpr operator const bgcode_raw_output_stream_vtable_t &() const {
    return vtable;
  }
};

class IStreamVTableMaker {
  bgcode_input_stream_vtable_t vtable;

public:
  constexpr IStreamVTableMaker(): vtable{nullptr, nullptr, nullptr, nullptr} {};

  constexpr IStreamVTableMaker &skip(decltype(vtable.skip) fn) {
    vtable.skip = fn;
    return *this;
  }

  constexpr IStreamVTableMaker &is_finished(decltype(vtable.is_finished) fn) {
    vtable.is_finished = fn;
    return *this;
  }

  constexpr IStreamVTableMaker &
  stream_vtable(const bgcode_stream_vtable_t *vt) {
    vtable.stream_vtable = vt;
    return *this;
  }

  constexpr IStreamVTableMaker &
  raw_istream_vtable(const bgcode_raw_input_stream_vtable_t *vt) {
    vtable.raw_istream_vtable = vt;
    return *this;
  }

  constexpr operator const bgcode_input_stream_vtable_t &() const {
    return vtable;
  }
};

class OStreamVTableMaker {
  bgcode_output_stream_vtable_t vtable;

public:
  constexpr OStreamVTableMaker &
  stream_vtable(const bgcode_stream_vtable_t *vt) {
    vtable.stream_vtable = vt;
    return *this;
  }

  constexpr OStreamVTableMaker &
  raw_ostream_vtable(const bgcode_raw_output_stream_vtable_t *vt) {
    vtable.raw_ostream_vtable = vt;
    return *this;
  }

  constexpr operator const bgcode_output_stream_vtable_t &() const {
    return vtable;
  }
};

class AllocatorVTableMaker {
  bgcode_allocator_vtable_t vtable;

public:
  constexpr AllocatorVTableMaker &allocate(decltype(vtable.allocate) fn) {
    vtable.allocate = fn;
    return *this;
  }
  constexpr AllocatorVTableMaker &deallocate(decltype(vtable.deallocate) fn) {
    vtable.deallocate = fn;
    return *this;
  }
  constexpr operator const bgcode_allocator_vtable_t &() const {
    return vtable;
  }
};

namespace traits {

// Teach the cxx traits based interface to use the in-house C style vtables

template <> struct ReferenceType<bgcode_stream_ref_t> {
  using Type = bgcode_stream_ref_t;
};
template <> struct ReferenceType<bgcode_input_stream_ref_t> {
  using Type = bgcode_input_stream_ref_t;
};
template <> struct ReferenceType<bgcode_output_stream_ref_t> {
  using Type = bgcode_output_stream_ref_t;
};
template <> struct ReferenceType<bgcode_raw_input_stream_ref_t> {
  using Type = bgcode_raw_input_stream_ref_t;
};
template <> struct ReferenceType<bgcode_raw_output_stream_ref_t> {
  using Type = bgcode_raw_output_stream_ref_t;
};

// Specialize StreamTraits for bgcode_stream_ref_t: just call each corresponding
// function through the vtable of bgcode_stream_ref_t
template <> struct StreamTraits<bgcode_stream_ref_t> {
  static const char *last_error_description(const bgcode_stream_ref_t &obj) {
    return obj.vtable->last_error_description(obj.self);
  }
  static bgcode_version_t version(const bgcode_stream_ref_t &obj) {
    return obj.vtable->version(obj.self);
  };
  static bgcode_checksum_type_t checksum_type(const bgcode_stream_ref_t &obj) {
    return obj.vtable->checksum_type(obj.self);
  };
};

template <> struct StreamTraits<bgcode_input_stream_ref_t> {
  static const char *
  last_error_description(const bgcode_input_stream_ref_t &obj) {
    return obj.vtable->stream_vtable->last_error_description(obj.self);
  }
  static bgcode_version_t version(const bgcode_input_stream_ref_t &obj) {
    return obj.vtable->stream_vtable->version(obj.self);
  };
  static bgcode_checksum_type_t
  checksum_type(const bgcode_input_stream_ref_t &obj) {
    return obj.vtable->stream_vtable->checksum_type(obj.self);
  };
};

template <> struct StreamTraits<bgcode_output_stream_ref_t> {
  static const char *
  last_error_description(const bgcode_output_stream_ref_t &obj) {
    return obj.vtable->stream_vtable->last_error_description(obj.self);
  }
  static bgcode_version_t version(const bgcode_output_stream_ref_t &obj) {
    return obj.vtable->stream_vtable->version(obj.self);
  };
  static bgcode_checksum_type_t
  checksum_type(const bgcode_output_stream_ref_t &obj) {
    return obj.vtable->stream_vtable->checksum_type(obj.self);
  };
};

template <> struct RawInputStreamTraits<bgcode_input_stream_ref_t> {
  static bool read(bgcode_input_stream_ref_t &obj, std::byte *buf, size_t sz) {
    return obj.vtable->raw_istream_vtable->read(
        obj.self, reinterpret_cast<unsigned char *>(buf), sz);
  }
};

template <> struct RawInputStreamTraits<bgcode_raw_input_stream_ref_t> {
  static bool read(bgcode_raw_input_stream_ref_t &obj, std::byte *buf,
                   size_t sz) {
    return obj.vtable->read(obj.self, reinterpret_cast<unsigned char *>(buf),
                            sz);
  }
};

template <>
struct InputStreamTraits<bgcode_input_stream_ref_t>
    : public StreamTraits<bgcode_input_stream_ref_t>,
      public RawInputStreamTraits<bgcode_input_stream_ref_t> {
  static bool skip(bgcode_input_stream_ref_t &obj, size_t bytes) {
    return obj.vtable->skip(obj.self, bytes);
  }
  static bool is_finished(const bgcode_input_stream_ref_t &obj) {
    return obj.vtable->is_finished(obj.self);
  }
};

template <> struct RawOutputStreamTraits<bgcode_output_stream_ref_t> {
  static bool write(bgcode_output_stream_ref_t &obj, const std::byte *buf,
                    size_t sz) {
    return obj.vtable->raw_ostream_vtable->write(
        obj.self, reinterpret_cast<const unsigned char *>(buf), sz);
  }
};

template <> struct RawOutputStreamTraits<bgcode_raw_output_stream_ref_t> {
  static bool write(bgcode_raw_output_stream_ref_t &obj, const std::byte *buf,
                    size_t sz) {
    return obj.vtable->write(obj.self,
                             reinterpret_cast<const unsigned char *>(buf), sz);
  }
};

template <>
struct OutputStreamTraits<bgcode_output_stream_ref_t>
    : public StreamTraits<bgcode_output_stream_ref_t>,
      public RawOutputStreamTraits<bgcode_output_stream_ref_t> {};

template <class IStreamT>
struct IStreamVTableAdaptor : public bgcode_input_stream_ref_t {
  static const bgcode_stream_vtable_t StreamVTable;

  static const bgcode_raw_input_stream_vtable_t RawIStreamVTable;

  static const bgcode_input_stream_vtable_t IStreamVTable;

  explicit IStreamVTableAdaptor(IStreamT &stream_obj)
      : bgcode_input_stream_ref_t{&IStreamVTable, this}, obj{&stream_obj} {}

  IStreamT *obj;
};

template<class IStreamT>
const bgcode_stream_vtable_t IStreamVTableAdaptor<IStreamT>::StreamVTable =
    StreamVTableMaker{}
        .last_error_description([](const void *self) {
          return core::last_error_description(
              *static_cast<const IStreamVTableAdaptor *>(self)->obj);
        })
        .checksum_type([](const void *self) {
          return stream_checksum_type(
              *static_cast<const IStreamVTableAdaptor<IStreamT> *>(self)->obj);
        })
        .version([](const void *self) {
          return stream_bgcode_version(
              *static_cast<const IStreamVTableAdaptor<IStreamT> *>(self)->obj);
        });

template <class IStreamT>
const bgcode_raw_input_stream_vtable_t
    IStreamVTableAdaptor<IStreamT>::RawIStreamVTable =
        RawIStreamVTableMaker{}.read([](void *self, unsigned char *buf,
                                        size_t sz) {
          return read_from_stream(
              *static_cast<IStreamVTableAdaptor<IStreamT> *>(self)->obj,
              reinterpret_cast<std::byte *>(buf), sz);
        });

template <class IStreamT>
const bgcode_input_stream_vtable_t IStreamVTableAdaptor<
    IStreamT>::IStreamVTable =
    IStreamVTableMaker{}
        .stream_vtable(&StreamVTable)
        .raw_istream_vtable(&RawIStreamVTable)
        .skip([](void *self, size_t bytes) {
          return core::skip_stream(
              *static_cast<IStreamVTableAdaptor<IStreamT> *>(self)->obj, bytes);
        })
        .is_finished([](const void *self) {
          return core::is_stream_finished(
              *static_cast<const IStreamVTableAdaptor<IStreamT> *>(self)->obj);
        });

template <>
struct IStreamVTableAdaptor<bgcode_input_stream_ref_t>
    : public bgcode_input_stream_ref_t {
  IStreamVTableAdaptor(bgcode_input_stream_ref_t &other)
      : bgcode_input_stream_ref_t{other.vtable, other.self} {}
};

template <> struct ParseHandlerTraits<bgcode_parse_handler_ref_t> {
  template <class IStreamT>
  static bgcode_parse_handler_result_t
  handle_block(bgcode_parse_handler_ref_t &obj, IStreamT &stream,
               const bgcode_block_header_t &block_header) {
    return obj.vtable->handle_block(obj.self, IStreamVTableAdaptor{stream},
                                    &block_header);
  }

  static bool can_continue(bgcode_parse_handler_ref_t &obj) {
    return obj.vtable->can_continue(obj.self);
  }
};

template <> struct BlockParseHandlerTraits<bgcode_block_parse_handler_ref_t> {
  // Max buffer
  static size_t
  payload_chunk_size(const bgcode_block_parse_handler_ref_t &obj) {
    return obj.vtable->payload_chunk_size(obj.self);
  }

  static std::byte *
  payload_chunk_buffer(bgcode_block_parse_handler_ref_t &obj) {
    return reinterpret_cast<std::byte *>(
        obj.vtable->payload_chunk_buffer(obj.self));
  }

  static void int_param(bgcode_block_parse_handler_ref_t &obj, const char *name,
                        long value, size_t bytes_width) {
    obj.vtable->int_param(obj.self, name, value, bytes_width);
  }

  static void string_param(bgcode_block_parse_handler_ref_t &obj,
                           const char *name, const char *value) {
    obj.vtable->string_param(obj.self, name, value);
  }

  static void float_param(bgcode_block_parse_handler_ref_t &obj,
                          const char *name, float value) {
    obj.vtable->float_param(obj.self, name, value);
  }

  static void double_param(bgcode_block_parse_handler_ref_t &obj,
                           const char *name, double value) {
    obj.vtable->double_param(obj.self, name, value);
  }

  static void payload(bgcode_block_parse_handler_ref_t &obj,
                      const std::byte *data_bytes, size_t bytes_count) {
    obj.vtable->payload(obj.self,
                        reinterpret_cast<const unsigned char *>(data_bytes),
                        bytes_count);
  }

  static void checksum(bgcode_block_parse_handler_ref_t &obj,
                       const std::byte *checksum_bytes, size_t bytes_count) {
    obj.vtable->checksum(
        obj.self, reinterpret_cast<const unsigned char *>(checksum_bytes),
        bytes_count);
  }
};

} // namespace traits
} // namespace core
} // namespace bgcode

#endif // BGCODE_CAPI_STREAM_TRAITS_HPP
