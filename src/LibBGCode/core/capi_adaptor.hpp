#ifndef BGCODE_CAPI_STREAM_TRAITS_HPP
#define BGCODE_CAPI_STREAM_TRAITS_HPP

#include "core/bgcode.h"
#include "core/bgcode_cxx_traits.hpp"

#include <memory_resource>

namespace bgcode {
namespace core {

class StreamVTableBuilder {
  bgcode_stream_vtable_t vtable;

public:
  constexpr StreamVTableBuilder() : vtable{} {}

  constexpr StreamVTableBuilder &
  last_error_description(decltype(vtable.last_error_description) fn) {
    vtable.last_error_description = fn;
    return *this;
  }

  constexpr StreamVTableBuilder &
  checksum_type(decltype(vtable.checksum_type) fn) {
    vtable.checksum_type = fn;
    return *this;
  }

  constexpr StreamVTableBuilder &
  version(decltype(vtable.version) fn) {
    vtable.version = fn;
    return *this;
  }

  constexpr operator const bgcode_stream_vtable_t&() const { return vtable; }
};

class RawIStreamVTableBuilder {
  bgcode_raw_istream_vtable_t vtable;

public:
  constexpr RawIStreamVTableBuilder() : vtable{nullptr} {}

  constexpr RawIStreamVTableBuilder &read(decltype(vtable.read) fn) {
    vtable.read = fn;
    return *this;
  }
  
  constexpr operator const bgcode_raw_istream_vtable_t &() const {
    return vtable;
  }
};

class RawOStreamVTableBuilder {
  bgcode_raw_ostream_vtable_t vtable;

public:
  constexpr RawOStreamVTableBuilder(): vtable{} {};

  constexpr RawOStreamVTableBuilder &write(decltype(vtable.write) fn) {
    vtable.write = fn;
    return *this;
  }
  
  constexpr operator const bgcode_raw_ostream_vtable_t &() const {
    return vtable;
  }
};

class IStreamVTableBuilder {
  bgcode_istream_vtable_t vtable;

public:
  constexpr IStreamVTableBuilder(): vtable{} {};

  constexpr IStreamVTableBuilder &skip(decltype(vtable.skip) fn) {
    vtable.skip = fn;
    return *this;
  }

  constexpr IStreamVTableBuilder &is_finished(decltype(vtable.is_finished) fn) {
    vtable.is_finished = fn;
    return *this;
  }

  constexpr IStreamVTableBuilder &
  stream_vtable(const bgcode_stream_vtable_t *vt) {
    vtable.stream_vtable = vt;
    return *this;
  }

  constexpr IStreamVTableBuilder &
  raw_istream_vtable(const bgcode_raw_istream_vtable_t *vt) {
    vtable.raw_istream_vtable = vt;
    return *this;
  }
  
  constexpr operator const bgcode_istream_vtable_t &() const {
    return vtable;
  }
};

class OStreamVTableBuilder {
  bgcode_ostream_vtable_t vtable;

public:
  constexpr OStreamVTableBuilder(): vtable{} {}

  constexpr OStreamVTableBuilder &
  stream_vtable(const bgcode_stream_vtable_t *vt) {
    vtable.stream_vtable = vt;
    return *this;
  }

  constexpr OStreamVTableBuilder &
  raw_ostream_vtable(const bgcode_raw_ostream_vtable_t *vt) {
    vtable.raw_ostream_vtable = vt;
    return *this;
  }
  
  constexpr operator const bgcode_ostream_vtable_t &() const {
    return vtable;
  }
};

class AllocatorVTableBuilder {
  bgcode_allocator_vtable_t vtable;

public:
  constexpr AllocatorVTableBuilder(): vtable{} {}

  constexpr AllocatorVTableBuilder &allocate(decltype(vtable.allocate) fn) {
    vtable.allocate = fn;
    return *this;
  }

  constexpr AllocatorVTableBuilder &deallocate(decltype(vtable.deallocate) fn) {
    vtable.deallocate = fn;
    return *this;
  }

  constexpr operator const bgcode_allocator_vtable_t &() const {
    return vtable;
  }
};

class ParseHandlerVTableBuilder {
  bgcode_parse_handler_vtable_t vtable;

public:
  constexpr ParseHandlerVTableBuilder(): vtable{nullptr, nullptr} {}

  constexpr ParseHandlerVTableBuilder &
  handle_block(decltype(vtable.handle_block) fn) {
    vtable.handle_block = fn;
    return *this;
  }

  constexpr ParseHandlerVTableBuilder &
  can_continue(decltype(vtable.can_continue) fn) {
    vtable.can_continue = fn;
    return *this;
  }

  constexpr operator const bgcode_parse_handler_vtable_t &() const {
    return vtable;
  }
};

class BlockParseHandlerVTableBuilder {
  bgcode_block_parse_handler_vtable_t vtable;

public:
  constexpr BlockParseHandlerVTableBuilder() : vtable{} {}

  constexpr operator const bgcode_block_parse_handler_vtable_t &() const {
    return vtable;
  }

//  size_t (*const payload_chunk_size)(const void *self);
//  unsigned char *(*const payload_chunk_buffer)(void *self);

//  void (*const int_param)(void *self, const char *name, long value,
//                          size_t bytes_width);
//  void (*const string_param)(void *self, const char *name, const char *value);
//  void (*const float_param)(void *self, const char *name, float value);
//  void (*const double_param)(void *self, const char *name, double value);
//  void (*const payload)(void *self, const unsigned char *data_bytes,
//                        size_t bytes_count);
//  void (*const checksum)(void *self, const unsigned char *checksum_bytes,
//                         size_t bytes_count);
};

namespace traits {

// Teach the cxx traits based interface to use the in-house C style vtables

template <> struct ReferenceType<bgcode_stream_ref_t> {
  using Type = bgcode_stream_ref_t;
};
template <> struct ReferenceType<bgcode_istream_ref_t> {
  using Type = bgcode_istream_ref_t;
};
template <> struct ReferenceType<bgcode_ostream_ref_t> {
  using Type = bgcode_ostream_ref_t;
};
template <> struct ReferenceType<bgcode_raw_istream_ref_t> {
  using Type = bgcode_raw_istream_ref_t;
};
template <> struct ReferenceType<bgcode_raw_ostream_ref_t> {
  using Type = bgcode_raw_ostream_ref_t;
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

template <> struct StreamTraits<bgcode_istream_ref_t> {
  static const char *
  last_error_description(const bgcode_istream_ref_t &obj) {
    return obj.vtable->stream_vtable->last_error_description(obj.self);
  }
  static bgcode_version_t version(const bgcode_istream_ref_t &obj) {
    return obj.vtable->stream_vtable->version(obj.self);
  };
  static bgcode_checksum_type_t
  checksum_type(const bgcode_istream_ref_t &obj) {
    return obj.vtable->stream_vtable->checksum_type(obj.self);
  };
};

template <> struct StreamTraits<bgcode_ostream_ref_t> {
  static const char *
  last_error_description(const bgcode_ostream_ref_t &obj) {
    return obj.vtable->stream_vtable->last_error_description(obj.self);
  }
  static bgcode_version_t version(const bgcode_ostream_ref_t &obj) {
    return obj.vtable->stream_vtable->version(obj.self);
  };
  static bgcode_checksum_type_t
  checksum_type(const bgcode_ostream_ref_t &obj) {
    return obj.vtable->stream_vtable->checksum_type(obj.self);
  };
};

template <> struct RawInputStreamTraits<bgcode_istream_ref_t> {
  static bool read(bgcode_istream_ref_t &obj, std::byte *buf, size_t sz) {
    return obj.vtable->raw_istream_vtable->read(
        obj.self, reinterpret_cast<unsigned char *>(buf), sz);
  }
};

template <> struct RawInputStreamTraits<bgcode_raw_istream_ref_t> {
  static bool read(bgcode_raw_istream_ref_t &obj, std::byte *buf,
                   size_t sz) {
    return obj.vtable->read(obj.self, reinterpret_cast<unsigned char *>(buf),
                            sz);
  }
};

template <>
struct InputStreamTraits<bgcode_istream_ref_t>
    : public StreamTraits<bgcode_istream_ref_t>,
      public RawInputStreamTraits<bgcode_istream_ref_t> {
  static bool skip(bgcode_istream_ref_t &obj, size_t bytes) {
    return obj.vtable->skip(obj.self, bytes);
  }
  static bool is_finished(const bgcode_istream_ref_t &obj) {
    return obj.vtable->is_finished(obj.self);
  }
};

template <> struct RawOutputStreamTraits<bgcode_ostream_ref_t> {
  static bool write(bgcode_ostream_ref_t &obj, const std::byte *buf,
                    size_t sz) {
    return obj.vtable->raw_ostream_vtable->write(
        obj.self, reinterpret_cast<const unsigned char *>(buf), sz);
  }
};

template <> struct RawOutputStreamTraits<bgcode_raw_ostream_ref_t> {
  static bool write(bgcode_raw_ostream_ref_t &obj, const std::byte *buf,
                    size_t sz) {
    return obj.vtable->write(obj.self,
                             reinterpret_cast<const unsigned char *>(buf), sz);
  }
};

template <>
struct OutputStreamTraits<bgcode_ostream_ref_t>
    : public StreamTraits<bgcode_ostream_ref_t>,
      public RawOutputStreamTraits<bgcode_ostream_ref_t> {};

template <class IStreamT>
struct IStreamVTableAdaptor : public bgcode_istream_ref_t {
  static const bgcode_stream_vtable_t StreamVTable;
  
  static const bgcode_raw_istream_vtable_t RawIStreamVTable;
  
  static const bgcode_istream_vtable_t IStreamVTable;

  explicit IStreamVTableAdaptor(IStreamT &stream_obj)
      : bgcode_istream_ref_t{&IStreamVTable, this}, obj{&stream_obj} {}

  IStreamT *obj;
};

template<class IStreamT>
const bgcode_stream_vtable_t IStreamVTableAdaptor<IStreamT>::StreamVTable =
    StreamVTableBuilder{}
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
const bgcode_raw_istream_vtable_t
    IStreamVTableAdaptor<IStreamT>::RawIStreamVTable =
        RawIStreamVTableBuilder{}.read([](void *self, unsigned char *buf,
                                        size_t sz) {
          return read_from_stream(
              *static_cast<IStreamVTableAdaptor<IStreamT> *>(self)->obj,
              reinterpret_cast<std::byte *>(buf), sz);
        });

template <class IStreamT>
const bgcode_istream_vtable_t IStreamVTableAdaptor<
    IStreamT>::IStreamVTable =
    IStreamVTableBuilder{}
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
struct IStreamVTableAdaptor<bgcode_istream_ref_t>
    : public bgcode_istream_ref_t {
  IStreamVTableAdaptor(bgcode_istream_ref_t &other)
      : bgcode_istream_ref_t{other.vtable, other.self} {}
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
                          const char *name, double value) {
    obj.vtable->float_param(obj.self, name, value);
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

  static void block_start(bgcode_block_parse_handler_ref_t &obj, const bgcode_block_header_t &header) {
    obj.vtable->block_start(obj.self, &header);
  }
};

} // namespace traits
} // namespace core
} // namespace bgcode

#endif // BGCODE_CAPI_STREAM_TRAITS_HPP
