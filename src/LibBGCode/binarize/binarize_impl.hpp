#ifndef BINARIZE_IMPL_HPP
#define BINARIZE_IMPL_HPP

#include <variant>

#include "core/bgcode_impl.hpp"

#include <zlib.h>

namespace bgcode { namespace binarize {

using namespace core;

class DeflateDecompressor {
  z_stream m_zstream;
  unsigned char *m_workbuf;
  size_t m_workbuf_len;
  bool m_finished;

public:
  DeflateDecompressor(unsigned char *workbuf, size_t workbuf_len)
      : m_workbuf{workbuf}, m_workbuf_len{workbuf_len},
        m_finished{false} {
    m_zstream.next_in = nullptr;
    m_zstream.avail_in = 0;
    m_zstream.next_out = m_workbuf;
    m_zstream.avail_out = m_workbuf_len;
  }

  template<class SinkFn>
  bool append(SinkFn &&sink, z_const unsigned char *source, size_t source_len) {
    m_zstream.next_in = source;
    m_zstream.avail_in = static_cast<uInt>(source_len);
    m_zstream.next_out = m_workbuf;
    m_zstream.avail_out = m_workbuf_len;

    int res = Z_OK;

    if (m_finished && (res = inflateInit(&m_zstream)) != Z_OK) {
      return false;
    }

    m_finished = false;

    while (m_zstream.avail_in > 0 || res == Z_OK) {
      int flush_flags = m_zstream.avail_in > 0 ? Z_NO_FLUSH : Z_FINISH;
      res = inflate(&m_zstream, Z_NO_FLUSH);
      if (res != Z_OK && res != Z_STREAM_END) {
        inflateEnd(&m_zstream);
        m_finished = true;
        return false;
      }
      if (m_zstream.avail_out == 0) {
        sink(m_workbuf, m_workbuf_len);
        m_zstream.next_out = m_workbuf;
        m_zstream.avail_out = m_workbuf_len;
      }
    }

    return res == Z_OK;
  }

  template<class SinkFn>
  bool finish(SinkFn &&sink, z_const unsigned char *source, size_t source_len) {
    bool ret = true;

    if (source && source_len > 0)
      ret = append(sink, source, source_len);

    if (ret) {
      inflateEnd(&m_zstream);
      m_finished = true;
    }

    return ret;
  }

  size_t uncompressed_bytes() const noexcept { return m_zstream.total_out; }

  ~DeflateDecompressor() {
    if (!m_finished) {
      inflateEnd(&m_zstream);
    }
  }
};

class DummyDecompressor {
  size_t m_bytes = 0;

public:
  template <class Fn>
  bool finish(Fn &&, const unsigned char *source, size_t source_len) {
    m_bytes += source_len;
    return true;
  }

  template<class Fn>
  bool append(Fn &&, const unsigned char *source, size_t source_len) {
    m_bytes += source_len;
    return true;
  }

  size_t uncompressed_bytes() const noexcept { return m_bytes; }
};

class Decompressor {
  std::variant<DummyDecompressor, DeflateDecompressor> m_decomp;

public:
  void reset(bgcode_compression_type_t compression_type, unsigned char *workbuf,
             size_t workbuf_len) {
    switch(compression_type) {
    case bgcode_ECompressionType_Deflate:
      m_decomp = DeflateDecompressor{workbuf, workbuf_len};
      break;
    default:
        ;
    }
  }

  size_t uncompressed_bytes() const noexcept {
    size_t ret = 0;
    std::visit([&ret](auto &decomp) { ret = decomp.uncompressed_bytes(); },
               m_decomp);

    return ret;
  }

  template <class Fn>
  bool finish(Fn &&, const unsigned char *source, size_t source_len) {

    return true;
  }

  template<class Fn>
  bool append(Fn &&, const unsigned char *source, size_t source_len) {

    return true;
  }
};

template<class BlockParseHandlerT>
class DecompBlockParseHandler {
  core::ReferenceType<BlockParseHandlerT> m_inner;
  Decompressor m_decomp;
  size_t m_uncomp_size;

  bool operator() (const unsigned char *uncompressed_buf, size_t len) {
    return m_inner.payload(uncompressed_buf, len);
  }

public:
  DecompBlockParseHandler(BlockParseHandlerT &inner)
      : m_inner{inner} {}

  size_t payload_chunk_size() const { return handler_payload_chunk_size(m_inner); }
  std::byte *payload_chunk_buffer() { return handler_payload_chunk_buffer(m_inner); }

  void int_param(const char *name, long value, size_t bytes_width) {
    handle_int_param(m_inner, name, value, bytes_width);
  }

  void string_param(const char *name, const char *value) {
    handle_string_param(m_inner, name, value);
  }

  void float_param(const char *name, float value) {
    handle_float_param(m_inner, name, value);
  }

  void double_param(const char *name, double value) {
    handle_double_param(m_inner, name, value);
  }

  void payload(const std::byte *compressed_buf, size_t bytes_count) {
    auto *buf = reinterpret_cast<const unsigned char *>(compressed_buf);
    if (m_decomp.uncompressed_bytes() < bytes_count + m_uncomp_size)
      m_decomp.append(*this, buf, bytes_count);
    else
      m_decomp.finish(*this, buf, bytes_count);
  }

  void checksum(const std::byte *checksum_bytes, size_t bytes_count) {
    handle_checksum(m_inner, checksum_bytes, bytes_count);
  }

  void block_start(const bgcode_block_header_t &header) {
    m_uncomp_size = header.uncompressed_size;
    auto *buf = reinterpret_cast<unsigned char *>(
        handler_payload_chunk_buffer(m_inner));
    m_decomp.reset(header.type, buf, handler_payload_chunk_size(m_inner));
  }
};

template<class IStreamT, class BlockParseHandlerT>
bgcode_result_t parse_block_decompressed(
    IStreamT &&stream, const bgcode_block_header_t &block_header,
    BlockParseHandlerT &&block_handler)
{
  DecompBlockParseHandler decomp_handler{block_handler};
  return core::parse_block(stream, block_header, decomp_handler);
}

} // namespace binarize
} // namespace bgcode

#endif // BINARIZE_IMPL_HPP
