#ifndef BGCODE_CXX_TRAITS_HPP
#define BGCODE_CXX_TRAITS_HPP

#include <iterator>

#include "core/bgcode_defs.h"

namespace bgcode {
namespace core {

template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

namespace traits {

template <class T, class En = void> struct ReferenceType {
  using Type = T &;
};

template <class T, class En = void> struct StreamTraits {
  static const char *last_error_description(const T &obj) {
    return obj.last_error_description();
  }
  static bgcode_version_t version(const T &obj) { return obj.version(); };
  static bgcode_checksum_type_t checksum_type(const T &obj) {
    return obj.checksum_type();
  };
};

template <class T, class En = void> struct RawInputStreamTraits {
  static bool read(T &obj, std::byte *buf, size_t sz) {
    return obj.read(buf, sz);
  }
};

template <class T, class En = void>
struct InputStreamTraits : public StreamTraits<T>,
                           public RawInputStreamTraits<T> {
  static bool skip(T &obj, size_t bytes) { return obj.skip(bytes); }
  static bool is_finished(const T &obj) { return obj.is_finished(); }
};

template <class T, class En = void> struct RawOutputStreamTraits {
  static bool write(T &obj, const std::byte *buf, size_t sz) {
    return obj.write(buf, sz);
  }
};

template <class T, class En = void>
struct OutputStreamTraits : public StreamTraits<T>,
                            public RawOutputStreamTraits<T> {
  static bgcode_result_t start_block(T &obj, bgcode_block_type_t block_type,
                                     bgcode_compression_type_t compression_type,
                                     bgcode_size_t uncompressed_size,
                                     bgcode_size_t compressed_size) {
    return obj.start_block(block_type, compression_type, uncompressed_size,
                           compressed_size);
  };

  static bgcode_result_t finish_block(T &obj) { return obj.finish_block(); };
  static bgcode_result_t start_params(T &obj) { return obj.start_params(); };
  static bgcode_result_t finish_params(T &obj) { return obj.finish_params(); };
};

template <class T, class En = void> struct ParseHandlerTraits {
  template <class IStreamT>
  static bgcode_parse_handler_result_t
  handle_block(T &obj, IStreamT &stream,
               const bgcode_block_header_t &block_header) {
    return obj.handle_block(stream, block_header);
  }

  static bool can_continue(T &obj) { return obj.can_continue(); }
};

template <class T, class En = void> struct BlockParseHandlerTraits {
  // Max buffer
  static size_t payload_chunk_size(const T &obj) {
    return obj.payload_chunk_size();
  }
  static std::byte *payload_chunk_buffer(T &obj) {
    return obj.payload_chunk_buffer();
  }

  static bool int_param(T &obj, const char *name, long value,
                        size_t bytes_width) {
    return obj.int_param(name, value, bytes_width);
  }

  static bool string_param(T &obj, const char *name, const char *value) {
    return obj.string_param(obj, name, value);
  }

  static bool float_param(T &obj, const char *name, float value) {
    return obj.float_param(name, value);
  }

  static bool double_param(T &obj, const char *name, double value) {
    return obj.double_param(name, value);
  }

  static bool payload(T &obj, const std::byte *data_bytes, size_t bytes_count) {
    return obj.payload(data_bytes, bytes_count);
  }

  static bool checksum(T &obj, const std::byte *checksum_bytes,
                       size_t bytes_count) {
    return obj.checksum(checksum_bytes, bytes_count);
  }
};

} // namespace traits

template <class T>
using ReferenceType = typename traits::ReferenceType<remove_cvref_t<T>>::Type;

template <class T> using StreamTraits = traits::StreamTraits<remove_cvref_t<T>>;

template <class T>
using RawInputStreamTraits = traits::RawInputStreamTraits<remove_cvref_t<T>>;

template <class T>
using InputStreamTraits = traits::InputStreamTraits<remove_cvref_t<T>>;

template <class T>
using RawOutputStreamTraits = traits::RawOutputStreamTraits<remove_cvref_t<T>>;

template <class T>
using OutputStreamTraits = traits::OutputStreamTraits<remove_cvref_t<T>>;

template <class T>
using ParseHandlerTraits = traits::ParseHandlerTraits<remove_cvref_t<T>>;

template <class T>
using BlockParseHandlerTraits =
    traits::BlockParseHandlerTraits<remove_cvref_t<T>>;

template <class ROStreamT, class T>
static bool write_to_stream(ROStreamT &stream, const T *data,
                            size_t data_size) {
  return RawOutputStreamTraits<ROStreamT>::write(
      stream, reinterpret_cast<const std::byte *>(data), data_size);
}

template <class RIStreamT, class T>
static bool read_from_stream(RIStreamT &stream, T *data, size_t data_size) {
  static_assert(!std::is_const_v<T>, "Type of output buffer cannot be const!");

  return RawInputStreamTraits<RIStreamT>::read(
      stream, reinterpret_cast<std::byte *>(data), data_size);
}

template <class IStreamT>
static bool skip_stream(IStreamT &stream, size_t bytes) {
  return InputStreamTraits<IStreamT>::skip(stream, bytes);
}

template <class IStreamT>
static bool is_stream_finished(const IStreamT &stream) {
  return InputStreamTraits<IStreamT>::is_finished(stream);
}

template <class StreamT>
static bgcode_checksum_type_t stream_checksum_type(const StreamT &stream) {
  return StreamTraits<StreamT>::checksum_type(stream);
}

template <class StreamT>
static bgcode_version_t stream_bgcode_version(const StreamT &stream) {
  return StreamTraits<StreamT>::version(stream);
}

template <class StreamT>
static const char *last_error_description(const StreamT &stream) {
  return StreamTraits<StreamT>::last_error_description(stream);
}

template <class ParseHandlerT>
static bool handler_can_continue(ParseHandlerT &handler) {
  return ParseHandlerTraits<ParseHandlerT>::can_continue(handler);
}

template <class ParseHandlerT, class IStreamT>
static bgcode_parse_handler_result_t
handle_block(ParseHandlerT &handler, IStreamT &stream,
             const bgcode_block_header_t &block_header) {
  return ParseHandlerTraits<ParseHandlerT>::handle_block(handler, stream,
                                                         block_header);
}

template <class BlockHandlerT>
static size_t handler_payload_chunk_size(const BlockHandlerT &handler) {
  return BlockParseHandlerTraits<BlockHandlerT>::payload_chunk_size(handler);
}

template <class BlockHandlerT>
static std::byte *handler_payload_chunk_buffer(BlockHandlerT &handler) {
  return BlockParseHandlerTraits<BlockHandlerT>::payload_chunk_buffer(handler);
}

template <class BlockHandlerT>
static void handle_int_param(BlockHandlerT &handler, const char *name,
                             long value, size_t bytes_width) {
  BlockParseHandlerTraits<BlockHandlerT>::int_param(handler, name, value,
                                                    bytes_width);
}

template <class BlockHandlerT>
static void handle_string_param(BlockHandlerT &handler, const char *name,
                                const char *value) {
  BlockParseHandlerTraits<BlockHandlerT>::string_param(handler, name, value);
}

template <class BlockHandlerT>
static void handle_float_param(BlockHandlerT &handler, const char *name,
                               float value) {
  BlockParseHandlerTraits<BlockHandlerT>::float_param(handler, name, value);
}

template <class BlockHandlerT>
static void handle_double_param(BlockHandlerT &handler, const char *name,
                                double value) {
  BlockParseHandlerTraits<BlockHandlerT>::double_param(handler, name, value);
}

template <class BlockHandlerT>
static void handle_payload(BlockHandlerT &handler, const std::byte *data_bytes,
                           size_t bytes_count) {
  BlockParseHandlerTraits<BlockHandlerT>::payload(handler, data_bytes,
                                                  bytes_count);
}

template <class BlockHandlerT>
static void handle_checksum(BlockHandlerT &handler,
                            const std::byte *checksum_bytes,
                            size_t bytes_count) {
  BlockParseHandlerTraits<BlockHandlerT>::checksum(handler, checksum_bytes,
                                                   bytes_count);
}

} // namespace core
} // namespace bgcode

#endif // BGCODE_CXX_TRAITS_HPP
