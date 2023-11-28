#ifndef BINARIZE_IMPL_HPP
#define BINARIZE_IMPL_HPP

#include <variant>

#include "core/bgcode_impl.hpp"

#include <zlib.h>

namespace bgcode { namespace binarize {

using namespace core;

class InflatorZLIB {
  z_stream m_zstream;
  std::byte *m_workbuf;
  size_t m_workbuf_len;
  bool m_finished;

public:
  InflatorZLIB(std::byte *workbuf, size_t workbuf_len)
      : m_zstream{}, m_workbuf{workbuf}, m_workbuf_len{workbuf_len},
        m_finished{true} {
    m_zstream.next_in = nullptr;
    m_zstream.avail_in = 0;
    m_zstream.next_out = reinterpret_cast<Bytef*>(m_workbuf);
    m_zstream.avail_out = static_cast<uInt>(m_workbuf_len);
  }

  template<class SinkFn>
  bool append(SinkFn &&sink, const std::byte *source, size_t source_len) {
    m_zstream.next_in = reinterpret_cast<Bytef*>(const_cast<std::byte *>(source));
    m_zstream.avail_in = static_cast<uInt>(source_len);
    m_zstream.next_out = reinterpret_cast<Bytef*>(m_workbuf);
    m_zstream.avail_out = static_cast<uInt>(m_workbuf_len);

    int res = Z_OK;

    if (m_finished) {
      res = inflateInit(&m_zstream);
      m_finished = false;
    }

    while (m_zstream.avail_in > 0 && res == Z_OK) {
      res = inflate(&m_zstream, Z_SYNC_FLUSH);
      if (res != Z_OK && res != Z_STREAM_END) {
        inflateEnd(&m_zstream);
        m_finished = true;
        return false;
      }
      if (m_zstream.avail_out < m_workbuf_len) {
        sink(m_workbuf, m_workbuf_len - m_zstream.avail_out);
        m_zstream.next_out = reinterpret_cast<Bytef*>(m_workbuf);
        m_zstream.avail_out = static_cast<uInt>(m_workbuf_len);
      }
    }

    return res == Z_OK;
  }

  template<class SinkFn>
  bool finish(SinkFn &&sink, const std::byte *source, size_t source_len) {
    bool ret = true;

    if (source && source_len > 0)
      ret = append(sink, source, source_len);

    if (ret) {
      inflateEnd(&m_zstream);
      m_finished = true;
    }

    return ret;
  }

  size_t processed_input_count() const noexcept { return m_zstream.total_in; }
  size_t processed_output_count() const noexcept { return m_zstream.total_out; }

  ~InflatorZLIB() {
    if (!m_finished) {
      inflateEnd(&m_zstream);
    }
  }
};

class DummyUnpacker {
  size_t m_bytes = 0;

public:
  template <class Fn>
  bool finish(Fn &&, const std::byte */*source*/, size_t source_len) {
    m_bytes += source_len;
    return true;
  }

  template<class Fn>
  bool append(Fn &&, const std::byte */*source*/, size_t source_len) {
    m_bytes += source_len;
    return true;
  }

  size_t processed_input_count() const noexcept { return m_bytes; }
  size_t processed_output_count() const noexcept { return m_bytes; }
};

class Unpacker {
  std::variant<DummyUnpacker, InflatorZLIB> m_decomp;

public:
  void reset(bgcode_compression_type_t compression_type, std::byte *workbuf,
             size_t workbuf_len) {
    switch(compression_type) {
    case bgcode_ECompressionType_None:
      m_decomp = DummyUnpacker{};
      break;
    case bgcode_ECompressionType_Deflate:
      m_decomp = InflatorZLIB{workbuf, workbuf_len};
      break;
    default:
        ;
    }
  }

  size_t processed_input_count() const noexcept {
    size_t ret = 0;
    std::visit([&ret](auto &decomp) { ret = decomp.processed_input_count(); },
               m_decomp);

    return ret;
  }

  size_t processed_output_count() const noexcept {
    size_t ret = 0;
    std::visit([&ret](auto &decomp) { ret = decomp.processed_output_count(); },
               m_decomp);

    return ret;
  }

  template <class Fn>
  bool finish(Fn &&sinkfn, const std::byte *source, size_t source_len) {
    bool ret = 0;
    std::visit(
        [&](auto &decomp) { ret = decomp.finish(sinkfn, source, source_len); },
        m_decomp);

    return ret;
  }

  template<class Fn>
  bool append(Fn &&sinkfn, const std::byte *source, size_t source_len) {
    bool ret = 0;
    std::visit(
        [&](auto &decomp) { ret = decomp.append(sinkfn, source, source_len); },
        m_decomp);

    return ret;
  }
};

template<class HandlerT>
class UnpackingBlockParseHandler {
  core::ReferenceType<HandlerT> m_inner;
  Unpacker m_unpacker;
  size_t m_compressed_size, m_uncompressed_size;
  bool m_decomp_failed = false;
  std::byte *m_workbuf;
  size_t m_workbuf_len;

public:
  bool operator() (const std::byte *uncompressed_buf, size_t len) {
    handle_payload(m_inner, uncompressed_buf, len);

    return true;
  }

  UnpackingBlockParseHandler(HandlerT &inner, std::byte *workbuf, size_t workbuf_len)
      : m_inner{inner}, m_workbuf{workbuf}, m_workbuf_len{workbuf_len} {}

  size_t payload_chunk_size() const { return handler_payload_chunk_size(m_inner); }
  std::byte *payload_chunk_buffer() { return handler_payload_chunk_buffer(m_inner); }

  void int_param(const char *name, long value, size_t bytes_width) {
    handle_int_param(m_inner, name, value, bytes_width);
  }

  void string_param(const char *name, const char *value) {
    handle_string_param(m_inner, name, value);
  }

  void float_param(const char *name, double value) {
    handle_float_param(m_inner, name, value);
  }

  void payload(const std::byte *compressed_buf, size_t bytes_count) {
    if (m_decomp_failed)
      return;

    if (bytes_count + m_unpacker.processed_input_count() < m_compressed_size)
      m_decomp_failed = !m_unpacker.append(*this, compressed_buf, bytes_count);
    else {
      assert(bytes_count + m_unpacker.processed_input_count() == m_compressed_size);
      m_decomp_failed = !m_unpacker.finish(*this, compressed_buf, bytes_count);
      // assert(m_decomp.processed_output_count() == m_uncompressed_size);
    }
  }

  void checksum(const std::byte *checksum_bytes, size_t bytes_count) {
    handle_checksum(m_inner, checksum_bytes, bytes_count);
  }

  void block_start(const bgcode_block_header_t &header) {
    m_compressed_size = header.compressed_size;
    m_uncompressed_size = header.uncompressed_size;
    m_decomp_failed = false;
    m_unpacker.reset(header.compression, m_workbuf, m_workbuf_len);
    handle_block_start(m_inner, header);
  }

  ReferenceType<HandlerT> get_child_handler() { return m_inner; }

  std::byte * workbuffer() { return m_workbuf; }
  size_t workbuf_len() const { return m_workbuf_len; }

  bgcode_EBlockParseStatus status() const { return bgcode_BlockParse_OK; }
};

template<class IStreamT, class BlockParseHandlerT>
bgcode_result_t parse_block_decompressed(
    IStreamT &&stream, const bgcode_block_header_t &block_header,
    BlockParseHandlerT &&block_handler, std::byte *workbuf, size_t workbuf_len)
{
  UnpackingBlockParseHandler decomp_handler{block_handler, workbuf, workbuf_len};
  return core::parse_block(stream, block_header, decomp_handler);
}

} // namespace binarize
} // namespace bgcode

#endif // BINARIZE_IMPL_HPP
