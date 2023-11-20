#ifndef BINARIZE_IMPL_HPP
#define BINARIZE_IMPL_HPP

#include <zlib.h>

namespace bgcode { namespace binarize {

template<class SinkFn>
class DeflateDecompressor {
  z_stream m_zstream;
  unsigned char *m_workbuf;
  size_t m_workbuf_len;
  SinkFn   m_sinkfn;

public:
  DeflateDecompressor(SinkFn &&sinkfn, unsigned char *workbuf, size_t workbuf_len)
      : m_sinkfn{sinkfn}, m_workbuf{workbuf}, m_workbuf_len{workbuf_len} {

    m_zstream.next_in = nullptr;
    m_zstream.avail_in = 0;
    m_zstream.next_out = m_workbuf;
    m_zstream.avail_out = m_workbuf_len;
    int res = inflateInit(&m_zstream);
  }

  bool process(z_const unsigned char *source, size_t source_len) {
    m_zstream.next_in = source;
    m_zstream.avail_in = static_cast<uInt>(source_len);
    m_zstream.next_out = m_workbuf;
    m_zstream.avail_out = m_workbuf_len;
    int res = inflateInit(&m_zstream);
    if (res != Z_OK)
      return false;

    while (m_zstream.avail_in > 0) {
      res = inflate(&m_zstream, Z_NO_FLUSH);
      if (res != Z_OK && res != Z_STREAM_END) {
        inflateEnd(&m_zstream);
        return false;
      }
      if (m_zstream.avail_out == 0) {
        dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE);
        m_zstream.next_out = temp_buffer.data();
        m_zstream.avail_out = BUFSIZE;
      }
    }

    int inflate_res = Z_OK;
    while (inflate_res == Z_OK) {
      if (m_zstream.avail_out == 0) {
        dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE);
        m_zstream.next_out = temp_buffer.data();
        m_zstream.avail_out = BUFSIZE;
      }
      inflate_res = inflate(&strm, Z_FINISH);
    }

    if (inflate_res != Z_STREAM_END) {
      inflateEnd(&m_zstream);
      return false;
    }

    dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE - strm.avail_out);
    inflateEnd(&m_zstream);
  }
};

} // namespace binarize
} // namespace bgcode

#endif // BINARIZE_IMPL_HPP
