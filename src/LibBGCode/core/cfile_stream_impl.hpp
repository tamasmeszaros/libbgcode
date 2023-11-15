#ifndef BGCODE_CFILE_STREAM_IMPL_HPP
#define BGCODE_CFILE_STREAM_IMPL_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>

#include "core/bgcode_impl.hpp"

namespace bgcode {
namespace core {

using namespace core;

struct FILEInputStream {
  FILE *m_fileptr;

public:
  FILEInputStream(FILE *fp) : m_fileptr{fp} {}

  bool read(std::byte *buf, size_t len) {
    bool ret = false;
    if (m_fileptr) {
      size_t rsize = std::fread(static_cast<void *>(buf), 1, len, m_fileptr);
      ret = !std::ferror(m_fileptr) && rsize == len;
    }

    return ret;
  }
};

struct FILEOutputStream {
  FILE *m_fileptr;

public:
  FILEOutputStream(FILE *fp) : m_fileptr{fp} {}

  bool write(const std::byte *buf, size_t len) {
    bool ret = false;
    if (m_fileptr) {
      size_t wsize =
          std::fwrite(static_cast<const void *>(buf), 1, len, m_fileptr);
      ret = !std::ferror(m_fileptr) && wsize == len;
    }

    return ret;
  }
};

struct CFileStream {
  bgcode_checksum_type_t m_checksum_type;
  bgcode_version_t m_version;
  FILE *m_fp;

public:
  CFileStream(FILE *file_ptr, bgcode_checksum_type_t chk_type,
              bgcode_version_t ver)
      : m_fp{file_ptr}, m_checksum_type{chk_type}, m_version{ver} {}

  bgcode_checksum_type_t checksum_type() const noexcept {
    return m_checksum_type;
  }
  const char *last_error_description() const noexcept {
    return "File IO Error";
  }
  bgcode_version_t version() const noexcept { return m_version; }

  bool read(std::byte *buf, size_t len) {
    FILEInputStream s{m_fp};
    return s.read(buf, len);
  }

  bool write(const std::byte *buf, size_t len) {
    FILEOutputStream s{m_fp};
    return s.write(buf, len);
  }

  bool skip(size_t bytes) {
    auto real_bytes = static_cast<long>(bytes);
    real_bytes = std::max(0l, real_bytes - 1);

    bool ret = !std::fseek(m_fp, real_bytes, SEEK_CUR);
    std::fgetc(m_fp);

    return ret;
  }

  bool finished() const { return std::feof(m_fp); }
};

} // namespace core
} // namespace bgcode

#endif
