#ifndef BGCODE_CAPI_STREAM_TRAITS_HPP
#define BGCODE_CAPI_STREAM_TRAITS_HPP

#include "core/bgcode.h"
#include "core/bgcode_cxx_traits.hpp"
#include "core/vtable_adaptor.hpp"

namespace bgcode {
namespace core {
namespace traits {

// Teach the cxx traits based interface to use the in-house C style vtables

template <> struct ReferenceType<bgcode_allocator_ref_t> {
  using Type = bgcode_allocator_ref_t;
};
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
template <> struct ReferenceType<bgcode_parse_handler_ref_t> {
  using Type = bgcode_parse_handler_ref_t;
};
template <> struct ReferenceType<bgcode_block_parse_handler_ref_t> {
  using Type = bgcode_block_parse_handler_ref_t;
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

template <> struct ParseHandlerTraits<bgcode_parse_handler_ref_t> {
  template <class IStreamT>
  static bgcode_parse_handler_result_t
  handle_block(bgcode_parse_handler_ref_t &obj, IStreamT &stream,
               const bgcode_block_header_t &block_header) {
    return obj.vtable->handle_block(obj.self, IStreamVTableAdaptor{stream},
                                    &block_header);
  }

  static bool can_continue(bgcode_parse_handler_ref_t &obj) {
    return obj.vtable->can_continue && obj.vtable->can_continue(obj.self);
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

  static void block_start(bgcode_block_parse_handler_ref_t &obj,
                          const bgcode_block_header_t &header) {
    obj.vtable->block_start(obj.self, &header);
  }

  static bgcode_EBlockParseStatus status(const bgcode_block_parse_handler_ref_t &obj) {
    return obj.vtable->status(obj.self);
  }
};

} // namespace traits
} // namespace core
} // namespace bgcode

#endif // BGCODE_CAPI_STREAM_TRAITS_HPP
