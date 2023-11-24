#ifndef BGCODE_VTABLE_ADAPTOR_HPP
#define BGCODE_VTABLE_ADAPTOR_HPP

#include "core/bgcode.h"
#include "core/bgcode_cxx_traits.hpp"

namespace bgcode { namespace core {

// Utilities to adapt C++ trait based interface to in-house vtables
// e.g. build vtables or auto-create vtables for conforming objects

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
  constexpr ParseHandlerVTableBuilder(): vtable{} {
    vtable.handle_block = [](void * /*self*/, bgcode_istream_ref_t /*istream*/,
                             const bgcode_block_header_t * /*header*/)
        -> bgcode_parse_handler_result_t { return {}; };
    vtable.can_continue = [](void */*self*/){ return true; };
  }

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

  static void block_start_f(void *self, const bgcode_block_header_t *header){}

public:
  constexpr BlockParseHandlerVTableBuilder() : vtable{} {
    vtable.payload_chunk_size = [](const void *) { return size_t{0}; };
    vtable.payload_chunk_buffer = [](void *) -> unsigned char * { return nullptr; };
    vtable.int_param = [](void *, const char *, long, size_t ) {};
    vtable.string_param = [](void *, const char *, const char *) {};
    vtable.float_param = [](void *, const char *, double) {};
    vtable.payload = [](void *, const unsigned char *, size_t) {};
    vtable.checksum = [](void *, const unsigned char *, size_t) {};
    vtable.block_start = block_start_f;
  }

  constexpr operator const bgcode_block_parse_handler_vtable_t &() const {
    return vtable;
  }

  constexpr BlockParseHandlerVTableBuilder &
  payload_chunk_size(size_t (*fn)(const void *self)) {
    vtable.payload_chunk_size = fn;
    return *this;
  }

  constexpr BlockParseHandlerVTableBuilder &
  payload_chunk_buffer(unsigned char *(*fn)(void *self)) {
    vtable.payload_chunk_buffer = fn;
    return *this;
  }

  constexpr BlockParseHandlerVTableBuilder &
  int_param(void (*fn)(void *self, const char *name, long val, size_t width)) {
    vtable.int_param = fn;
    return *this;
  }

  constexpr BlockParseHandlerVTableBuilder &
  string_param(void (*fn)(void *self, const char *name, const char *value)) {
    vtable.string_param = fn;
    return *this;
  }

  constexpr BlockParseHandlerVTableBuilder &
  float_param(void (*fn)(void *self, const char *name, double value)) {
    vtable.float_param = fn;
    return *this;
  }

  constexpr BlockParseHandlerVTableBuilder &
  payload(void (*fn)(void *, const unsigned char *, size_t)) {
    vtable.payload = fn;
    return *this;
  }

  constexpr BlockParseHandlerVTableBuilder &
  checksum(void (*fn)(void *, const unsigned char *, size_t)) {
    vtable.checksum = fn;
    return *this;
  }

  constexpr BlockParseHandlerVTableBuilder &
  block_start(void (*fn)(void *self, const bgcode_block_header_t *header)) {
    vtable.block_start = fn;
    return *this;
  }
};

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

template <class HandlerT>
struct BlockParseHandlerVTableAdaptor : public bgcode_block_parse_handler_ref_t {
  static const bgcode_block_parse_handler_vtable_t VTable;

  explicit BlockParseHandlerVTableAdaptor(HandlerT &handler)
      : bgcode_block_parse_handler_ref_t{&VTable, &handler} {}

  static HandlerT *get_obj(void *s) {
    return static_cast<HandlerT *>(s);
  }
  static const HandlerT *get_obj(const void *s) {
    return static_cast<const HandlerT *>(s);
  }
};

template <class HandlerT>
const bgcode_block_parse_handler_vtable_t BlockParseHandlerVTableAdaptor<
    HandlerT>::VTable =
    BlockParseHandlerVTableBuilder{}
        .payload_chunk_size([](const void *self) {
          return handler_payload_chunk_size(*get_obj(self));
        })
        .payload_chunk_buffer([](void *self) {
          return reinterpret_cast<unsigned char *>(
              handler_payload_chunk_buffer(*get_obj(self)));
        })
        .int_param(
            [](void *self, const char *name, long value, size_t bytes_width) {
              handle_int_param(*get_obj(self), name, value, bytes_width);
            })
        .float_param([](void *self, const char *name, double value) {
          handle_float_param(*get_obj(self), name, value);
        })
        .string_param([](void *self, const char *name, const char *value) {
          handle_string_param(*get_obj(self), name, value);
        })
        .checksum([](void *self, const unsigned char *data_bytes, size_t len) {
          handle_checksum(
              *get_obj(self),
              reinterpret_cast<const std::byte *>(data_bytes), len);
        })
        .payload([](void *self, const unsigned char *data_bytes,
                    size_t bytes_count) {
          handle_payload(*get_obj(self), reinterpret_cast<const std::byte*>(data_bytes), bytes_count);
        })
        .block_start([](void *s, const bgcode_block_header_t *header) {
          handle_block_start(*get_obj(s), *header);
        });

template <>
struct BlockParseHandlerVTableAdaptor<bgcode_block_parse_handler_ref_t>
    : public bgcode_block_parse_handler_ref_t {
  BlockParseHandlerVTableAdaptor(bgcode_block_parse_handler_ref_t &other)
      : bgcode_block_parse_handler_ref_t{other.vtable, other.self} {}
};

} // namespace core
} // namespace bgcode

#endif // BGCODE_VTABLE_ADAPTOR_HPP
