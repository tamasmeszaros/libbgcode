#ifndef BGCODE_IMPL_HPP
#define BGCODE_IMPL_HPP

#include <array>
#include <cassert>
#include <climits>
#include <cstdint>
#include <iterator>
#include <memory_resource>
#include <optional>
#include <type_traits>
#include <vector>

#include "core/bgcode_cxx_traits.hpp"

struct bgcode_block_header_t {
  bgcode_block_type_t type;
  bgcode_compression_type_t compression;
  bgcode_size_t uncompressed_size;
  bgcode_size_t compressed_size;
};

struct bgcode_stream_header_t {
  std::array<char, 4> magic;
  bgcode_version_t version;
  bgcode_checksum_type_t checksum_type;

  bgcode_stream_header_t() : allocator{nullptr, nullptr} {}

  explicit bgcode_stream_header_t(bgcode_allocator_ref_t alloc)
      : allocator{alloc} {}

  bgcode_stream_header_t &set_magic(const std::array<char, 4> &mg) {
    magic = mg;
    return *this;
  }
  bgcode_stream_header_t &set_version(bgcode_version_t val) {
    version = val;
    return *this;
  }
  bgcode_stream_header_t &set_checksum_type(bgcode_checksum_type_t val) {
    checksum_type = val;
    return *this;
  }

  bgcode_allocator_ref_t allocator;
};

namespace bgcode {
namespace core {

// Max size of checksum buffer data, in bytes
// Increase this value if you implement a checksum algorithm needing a bigger
// buffer
static constexpr const size_t MAX_CHECKSUM_SIZE = 4;

static constexpr const std::array<char, 4> MAGIC{'G', 'C', 'D', 'E'};

// Highest binary gcode file version supported.
static constexpr const bgcode_version_t VERSION = 1;

template <class I, class T = I>
using IntegerOnly = std::enable_if_t<std::is_integral_v<I>, T>;

template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <class BufT>
constexpr bool IsBufferType =
    std::is_convertible_v<remove_cvref_t<BufT>, std::byte> ||
    std::is_convertible_v<remove_cvref_t<BufT>, unsigned char> ||
    std::is_convertible_v<remove_cvref_t<BufT>, char>;

template <class T, class En = void>
struct IsBufferIterator_s : public std::false_type {};

template <class It>
static constexpr bool IsBufferIterator = IsBufferIterator_s<It>::value;

template <class BufT, class T = BufT>
using BufferTypeOnly = std::enable_if_t<IsBufferType<BufT>, T>;

template <class It>
struct IsBufferIterator_s<
    It, BufferTypeOnly<typename std::iterator_traits<It>::value_type, void>> {
  static constexpr bool value = true;
};

template <class It, class T = It>
using BufferIteratorOnly = std::enable_if_t<IsBufferIterator<It>, T>;

template <class Enum> constexpr auto to_underlying(Enum enumval) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(enumval);
}

// For LE byte sequences only
template <class IntT, class It, class = BufferIteratorOnly<It>>
constexpr IntegerOnly<IntT> load_integer(It from, It to) noexcept {
  IntT result{};

  size_t i = 0;
  for (It it = from; it != to && i < sizeof(IntT); ++it) {
    result |= (static_cast<IntT>(*it) << (i++ * sizeof(std::byte) * CHAR_BIT));
  }

  return result;
}

template <class IntT, class OutIt>
constexpr BufferIteratorOnly<OutIt, void>
store_integer_le(IntT value, OutIt out, size_t sz = sizeof(IntT)) {
  for (size_t i = 0; i < std::min(sizeof(IntT), sz); ++i) {
    *out++ = static_cast<typename std::iterator_traits<OutIt>::value_type>(
        (value >> (i * CHAR_BIT)) & UCHAR_MAX);
  }
}

template <class IntT, class OutIt>
std::enable_if_t<!IsBufferIterator<OutIt>, void>
store_integer_le(IntT value, OutIt out, size_t sz = sizeof(IntT)) {
  for (size_t i = 0; i < std::min(sizeof(IntT), sz); ++i) {
    *out++ = (value >> (i * CHAR_BIT)) & UCHAR_MAX;
  }
}

static constexpr auto MAGICi32 =
    load_integer<uint32_t>(std::begin(MAGIC), std::end(MAGIC));

template <class RawIStreamT, class IntType, class = IntegerOnly<IntType>>
bool read_integer(RawIStreamT &&stream, IntType &outval,
                  size_t intsize = sizeof(IntType)) {
  std::array<std::byte, sizeof(IntType)> buf;
  bool ret = read_from_stream(stream, buf.data(), intsize);
  if (ret)
    outval = load_integer<IntType>(buf.begin(), buf.begin() + intsize);

  return ret;
}

template <class RawOStreamT, class IntType, class = IntegerOnly<IntType>>
bool write_integer(RawOStreamT &stream, IntType val,
                   size_t intsize = sizeof(IntType)) {
  std::array<std::byte, sizeof(IntType)> buf;
  store_integer_le(val, buf.begin(), intsize);

  bool ret = write_to_stream(stream, buf.data(), intsize);

  return ret;
}

template <class It, class = BufferIteratorOnly<It>>
static constexpr uint32_t crc32_sw(It from, It to, uint32_t crc) {
  constexpr uint32_t ui32Max = 0xFFFFFFFF;
  constexpr uint32_t crcMagic = 0xEDB88320;

  uint32_t value = crc ^ ui32Max;
  for (auto it = from; it != to; ++it) {
    value ^= load_integer<uint32_t>(it, std::next(it));
    for (int bit = 0; bit < CHAR_BIT; bit++) {
      if (value & 1)
        value = (value >> 1) ^ crcMagic;
      else
        value >>= 1;
    }
  }
  value ^= ui32Max;

  return value;
}

constexpr auto checksum_types_count() noexcept {
  auto v = to_underlying(bgcode_EChecksumType_CRC32);
  ++v;
  return v;
}
constexpr auto block_types_count() noexcept {
  auto v = to_underlying(bgcode_EBlockType_Thumbnail);
  ++v;
  return v;
}
constexpr auto compression_types_count() noexcept {
  auto v = to_underlying(bgcode_ECompressionType_Heatshrink_12_4);
  ++v;
  return v;
}

template <class RawInStreamT>
bgcode_result_t
read_header(RawInStreamT &&stream, bgcode_stream_header_t &header,
            const bgcode_version_t *const max_version = nullptr) {
  if (!read_from_stream(stream, header.magic.data(), header.magic.size()))
    return bgcode_EResult_ReadError;

  if (header.magic != MAGIC)
    return bgcode_EResult_InvalidMagicNumber;

  if (!read_integer(stream, header.version, sizeof(header.version)))
    return bgcode_EResult_ReadError;

  if (max_version != nullptr && header.version > *max_version)
    return bgcode_EResult_InvalidVersionNumber;

  if (!read_integer(stream, header.checksum_type, sizeof(header.checksum_type)))
    return bgcode_EResult_ReadError;

  if (header.checksum_type >= checksum_types_count())
    return bgcode_EResult_InvalidChecksumType;

  return bgcode_EResult_Success;
}

template <class RawOStreamT>
bgcode_result_t write_header(RawOStreamT &&stream,
                             const bgcode_stream_header_t &header) {
  if (header.magic != MAGIC)
    return bgcode_EResult_InvalidMagicNumber;

  if (header.checksum_type >= checksum_types_count())
    return bgcode_EResult_InvalidChecksumType;

  if (!write_to_stream(stream, header.magic.data(), header.magic.size()))
    return bgcode_EResult_WriteError;
  if (!write_integer(stream, header.version, sizeof(header.version)))
    return bgcode_EResult_WriteError;
  if (!write_integer(stream, header.checksum_type,
                     sizeof(header.checksum_type)))
    return bgcode_EResult_WriteError;

  return bgcode_EResult_Success;
}

inline bgcode_block_type_t
get_block_type(const bgcode_block_header_t &header) noexcept {
  return header.type;
}

inline bgcode_compression_type_t
get_compression_type(const bgcode_block_header_t &header) noexcept {
  return header.compression;
}

inline bgcode_size_t
get_uncompressed_length(const bgcode_block_header_t &header) noexcept {
  return header.uncompressed_size;
}

inline bgcode_size_t
get_compressed_length(const bgcode_block_header_t &header) noexcept {
  return header.compressed_size;
}

inline constexpr size_t
block_parameters_length(bgcode_block_type_t type) noexcept {
  switch (type) {
  case bgcode_EBlockType_FileMetadata:
  case bgcode_EBlockType_GCode:
  case bgcode_EBlockType_SlicerMetadata:
  case bgcode_EBlockType_PrinterMetadata:
  case bgcode_EBlockType_PrintMetadata: {
    return sizeof(bgcode_metadata_encoding_type_t);
  } /* encoding_type */
  case bgcode_EBlockType_Thumbnail: {
    return sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t);
  } /* format, width, height */
  }

  return 0;
}

inline size_t
block_payload_length(const bgcode_block_header_t &block_header) noexcept {
  size_t ret = block_parameters_length(block_header.type);
  ret += (block_header.compression == bgcode_ECompressionType_None)
             ? block_header.uncompressed_size
             : block_header.compressed_size;

  return ret;
}

constexpr inline size_t checksum_length(bgcode_checksum_type_t type) noexcept {
  switch (type) {
  case bgcode_EChecksumType_None: {
    return 0;
  }
  case bgcode_EChecksumType_CRC32: {
    return 4;
  }
  }

  return 0;
}

inline bgcode_version_t max_bgcode_format_version() noexcept {
  return bgcode::core::VERSION;
}

inline constexpr const char *translate_result_code(bgcode_result_t result) {
  switch (result) {
  case bgcode_EResult_Success: {
    return "Success";
  }
  case bgcode_EResult_ReadError: {
    return "Read error";
  }
  case bgcode_EResult_WriteError: {
    return "Write error";
  }
  case bgcode_EResult_InvalidMagicNumber: {
    return "Invalid magic number";
  }
  case bgcode_EResult_InvalidVersionNumber: {
    return "Invalid version number";
  }
  case bgcode_EResult_InvalidChecksumType: {
    return "Invalid checksum type";
  }
  case bgcode_EResult_InvalidBlockType: {
    return "Invalid block type";
  }
  case bgcode_EResult_InvalidCompressionType: {
    return "Invalid compression type";
  }
  case bgcode_EResult_InvalidMetadataEncodingType: {
    return "Invalid metadata encoding type";
  }
  case bgcode_EResult_InvalidGCodeEncodingType: {
    return "Invalid gcode encoding type";
  }
  case bgcode_EResult_DataCompressionError: {
    return "Data compression error";
  }
  case bgcode_EResult_DataUncompressionError: {
    return "Data uncompression error";
  }
  case bgcode_EResult_MetadataEncodingError: {
    return "Metadata encoding error";
  }
  case bgcode_EResult_MetadataDecodingError: {
    return "Metadata decoding error";
  }
  case bgcode_EResult_GCodeEncodingError: {
    return "GCode encoding error";
  }
  case bgcode_EResult_GCodeDecodingError: {
    return "GCode decoding error";
  }
  case bgcode_EResult_BlockNotFound: {
    return "Block not found";
  }
  case bgcode_EResult_InvalidChecksum: {
    return "Invalid checksum";
  }
  case bgcode_EResult_InvalidThumbnailFormat: {
    return "Invalid thumbnail format";
  }
  case bgcode_EResult_InvalidThumbnailWidth: {
    return "Invalid thumbnail width";
  }
  case bgcode_EResult_InvalidThumbnailHeight: {
    return "Invalid thumbnail height";
  }
  case bgcode_EResult_InvalidThumbnailDataSize: {
    return "Invalid thumbnail data size";
  }
  case bgcode_EResult_InvalidBinaryGCodeFile: {
    return "Invalid binary GCode file";
  }
  case bgcode_EResult_InvalidAsciiGCodeFile: {
    return "Invalid ascii GCode file";
  }
  case bgcode_EResult_InvalidSequenceOfBlocks: {
    return "Invalid sequence of blocks";
  }
  case bgcode_EResult_InvalidBuffer: {
    return "Invalid buffer";
  }
  case bgcode_EResult_AlreadyBinarized: {
    return "Already binarized";
  }
  case bgcode_EResult_MissingPrinterMetadata: {
    return "Missing printer metadata";
  }
  case bgcode_EResult_MissingPrintMetadata: {
    return "Missing print metadata";
  }
  case bgcode_EResult_MissingSlicerMetadata: {
    return "Missing slicer metadata";
  }
  case bgcode_EResult_OutOfMemory: {
    return "Out of memory";
  }
  case bgcode_EResult_UnknownError: {
    return "Unknown error";
  }
  }

  return "";
}

inline size_t block_content_length(bgcode_checksum_type_t checksum_type,
                                   const bgcode_block_header_t &header) {
  return block_payload_length(header) + checksum_length(checksum_type);
}

class Checksum {
public:
  // Constructs a checksum of the given type.
  // The checksum data are sized accordingly.
  explicit Checksum(bgcode_checksum_type_t type)
      : m_type(type), m_size(checksum_length(type)) {
    m_checksum.fill(std::byte{0});
  }

  bgcode_checksum_type_t get_type() const noexcept { return m_type; }

  // Append vector of data to checksum
  void append(const std::vector<std::byte> &data) {
    append(data.data(), data.size());
  }

  // Append data to the checksum
  template <class BufT> void append(const BufT *data, size_t size) {
    if (data == nullptr || size == 0)
      return;

    switch (m_type) {
    case bgcode_EChecksumType_None: {
      break;
    }
    case bgcode_EChecksumType_CRC32: {
      static_assert(sizeof(m_checksum) >= sizeof(uint32_t),
                    "CRC32 checksum requires at least 4 bytes");
      const auto old_crc = load_integer<uint32_t>(
          m_checksum.begin(),
          m_checksum.end());
      const uint32_t new_crc = crc32_sw(data, data + size, old_crc);
      store_integer_le(new_crc, m_checksum.begin(), m_checksum.size());
      break;
    }
    }
  }

  // Append any arithmetic data to the checksum (shorthand for aritmetic types)
  template <typename T,
            typename = std::enable_if_t<
                std::is_integral_v<std::remove_reference_t<T>>, T>>
  void append(T &&data) {
    std::array<std::byte, sizeof(T)> valbuf;
    store_integer_le(data, valbuf.begin());
    append(valbuf.data(), sizeof(data));
  }

  // Returns true if the given checksum is equal to this one
  bool matches(const Checksum &other) const noexcept {
    return m_checksum == other.m_checksum;
  }

  bool
  matches(const std::array<std::byte, MAX_CHECKSUM_SIZE> &buf) const noexcept {
    return m_checksum == buf;
  }

  const std::byte *data() const noexcept { return m_checksum.data(); }
  std::byte *data() noexcept { return m_checksum.data(); }
  size_t size() const noexcept { return m_size; }

private:
  bgcode_checksum_type_t m_type;
  // actual size of checksum buffer, type dependent
  size_t m_size;
  std::array<std::byte, MAX_CHECKSUM_SIZE> m_checksum;
};

template <class RIStreamT>
bgcode_result_t read(RIStreamT &&stream, Checksum &checksum) {
  if (checksum.get_type() != bgcode_EChecksumType_None) {
    if (!read_from_stream(stream, checksum.data(), checksum.size()))
      return bgcode_EResult_ReadError;
  }

  return bgcode_EResult_Success;
}

template <class ROStreamT>
bgcode_EResult write(ROStreamT &&stream, const Checksum &checksum) {
  if (checksum.get_type() != bgcode_EChecksumType_None) {
    if (!write_to_stream(stream, checksum.data(), checksum.size()))
      return bgcode_EResult_WriteError;
  }

  return bgcode_EResult_Success;
}

// Updates the given checksum with the data of this BlockHeader
inline void update_checksum(Checksum &checksum,
                            const bgcode_block_header_t &block_header) {
  checksum.append(block_header.type);
  checksum.append(block_header.compression);
  checksum.append(block_header.uncompressed_size);
  if (block_header.compression != bgcode_ECompressionType_None)
    checksum.append(block_header.compressed_size);
}

template <class RIStreamT>
bgcode_EResult read(RIStreamT &&stream, bgcode_block_header_t &header) {
  bgcode_block_type_t blocktype = 0;
  if (!read_integer(stream, blocktype, sizeof(bgcode_block_type_t)))
    return bgcode_EResult_ReadError;

  if (blocktype >= 0 && blocktype < block_types_count())
    header.type = blocktype;
  else
    return bgcode_EResult_InvalidBlockType;

  bgcode_compression_type_t compression = 0;
  if (!read_integer(stream, compression, sizeof(bgcode_compression_type_t)))
    return bgcode_EResult_ReadError;

  if (compression >= 0 && compression < compression_types_count())
    header.compression = compression;
  else
    return bgcode_EResult_InvalidCompressionType;

  if (!read_integer(stream, header.uncompressed_size,
                    sizeof(header.uncompressed_size)))
    return bgcode_EResult_ReadError;

  if (header.compression != bgcode_ECompressionType_None) {
    if (!read_integer(stream, header.compressed_size,
                      sizeof(header.compressed_size)))
      return bgcode_EResult_ReadError;
  } else {
    header.compressed_size = header.uncompressed_size;
  }

  return bgcode_EResult_Success;
}

template <class ROStreamT>
bgcode_EResult write(ROStreamT &&stream, const bgcode_block_header_t &header) {
  if (!write_integer(stream, header.type, sizeof(header.type)))
    return bgcode_EResult_WriteError;
  if (!write_integer(stream, header.compression, sizeof(header.compression)))
    return bgcode_EResult_WriteError;
  if (!write_integer(stream, header.uncompressed_size,
                     sizeof(header.uncompressed_size)))
    return bgcode_EResult_WriteError;
  if (header.compression != bgcode_ECompressionType_None) {
    if (!write_integer(stream, header.compressed_size,
                       sizeof(header.compressed_size)))
      return bgcode_EResult_WriteError;
  }

  return bgcode_EResult_Success;
}

template <class IStreamT> class ChecksumCheckingIStream {
  ReferenceType<IStreamT> m_parent;
  std::byte *m_buf;
  Checksum m_checksum;
  size_t m_buf_len;
  size_t m_block_payload_size;
  size_t m_bytes_read;
  size_t m_chk_buf_pos;
  std::array<std::byte, MAX_CHECKSUM_SIZE> m_chk_buf;

  // Read from the stored stream, copy it into buf but also append it into
  // "appendable" which is either the checksum calculator or the block checksum
  // buffer (m_chk_buf)
  template <class Appendable>
  bool read_internal(std::byte *buf, size_t sz, Appendable &&appendable) {
    bool ret = true;

    size_t rounds = sz / m_buf_len;
    size_t last_round_cnt = sz % m_buf_len;

    for (size_t r = 0; ret && r < rounds && !is_stream_finished(m_parent);
         ++r) {
      ret = read_from_stream(m_parent, m_buf, m_buf_len);
      appendable.append(m_buf, m_buf_len);
      std::copy(m_buf, m_buf + m_buf_len, buf + r * m_buf_len);
      m_bytes_read += m_buf_len;
    }

    if (ret && last_round_cnt && !is_stream_finished(m_parent)) {
      ret = read_from_stream(m_parent, m_buf, last_round_cnt);
      appendable.append(m_buf, last_round_cnt);
      std::copy(m_buf, m_buf + last_round_cnt, buf + rounds * m_buf_len);
      m_bytes_read += last_round_cnt;
    }

    return ret;
  }

public:
  // The buffer buf must be a valid memory location and len must indicate the
  // size of the buffer which must be greater than zero. The bigger the buffer,
  // the faster the checksum calculation will be. The validity of the buffer
  // will not be checked.
  explicit ChecksumCheckingIStream(IStreamT &parent,
                                   const bgcode_block_header_t &blk_header,
                                   std::byte *buf, size_t len)
      : m_parent{parent}, m_buf{buf},
        m_checksum{stream_checksum_type(m_parent)}, m_buf_len{len},
        m_block_payload_size{block_payload_length(blk_header)}, m_bytes_read{0},
        m_chk_buf_pos{0} {

    assert(buf && len > 0);

    update_checksum(m_checksum, blk_header);
    m_chk_buf.fill(std::byte{0});
  }

  // the read buffer bytes shall go into
  // 1) fully into calculator
  // 2) partly into calculator and partly into block checksum store
  // 3) fully into block checksum store
  //
  // In all cases, the content of what has been read must be copied into outbuf,
  // as if it was directly read into it (from the caller's perspective)
  //
  // This does not address cases when the stream is read past the block. That is
  // an invalid use case (should it be prevented?)
  bool read(std::byte *outbuf, size_t sz) {
    size_t block_full_size = m_block_payload_size + m_checksum.size();

    // Just refuse to read if it would be past the block
    if (sz + m_bytes_read > block_full_size)
      return false;

    bool ret = true;

    size_t bytes_read_after = sz + m_bytes_read;

    size_t to_buf_len =
        0; // Amount that from the block payload which is checksum checked
    size_t to_chk_len =
        0; // Amount that is read from the block checksum section

    if (bytes_read_after <= m_block_payload_size)
      to_buf_len = sz;
    else if (m_bytes_read < m_block_payload_size &&
             bytes_read_after > m_block_payload_size) {
      to_buf_len = m_block_payload_size - m_bytes_read;
      to_chk_len = bytes_read_after - m_block_payload_size;
    } else {
      to_chk_len = block_full_size - m_bytes_read;
    }

    // Appendable object from the block checksum buffer
    struct AppendableBlockChecksum {
      ChecksumCheckingIStream *self;
      void append(const std::byte *from_buf, size_t len) {
        std::copy(from_buf, from_buf + len,
                  self->m_chk_buf.data() + self->m_chk_buf_pos);
        self->m_chk_buf_pos += len;
      }
    };

    if ((ret = read_internal(outbuf, to_buf_len, m_checksum)))
      ret = read_internal(outbuf + to_buf_len, to_chk_len,
                          AppendableBlockChecksum{this});

    return ret;
  }

  bool skip(size_t bytes) {
    size_t bytes_rem = bytes;
    size_t chunk_sz = std::min(bytes, m_buf_len);
    bool ret = true;

    while (ret && bytes_rem && !this->is_finished()) {
      size_t to_read = std::min(bytes_rem, chunk_sz);
      ret = this->read(m_buf, to_read);
      bytes_rem -= to_read;
    }

    return ret;
  }

  bool is_finished() const { return is_stream_finished(m_parent); }

  const char *last_error_description() const {
    return core::last_error_description(m_parent);
  }

  bgcode_version_t version() const { return stream_bgcode_version(m_parent); };
  bgcode_checksum_type_t checksum_type() const {
    return stream_checksum_type(m_parent);
  };

  bool is_checksum_correct() const { return m_checksum.matches(m_chk_buf); }
};

template <class IStreamT>
bgcode_result_t skip_block(IStreamT &stream,
                           const bgcode_block_header_t &block_header) {
  bgcode_result_t ret = bgcode_EResult_Success;

  if (is_stream_finished(stream))
    ret = bgcode_EResult_Success;

  else if (!skip_stream(stream,
                        block_content_length(stream_checksum_type(stream),
                                             block_header)))
    ret = bgcode_EResult_ReadError;

  return ret;
}

template <class ParseHandlerT> struct ChecksumCheckingParseHandler {
  ReferenceType<ParseHandlerT> handler;
  std::byte *buf = nullptr;
  size_t buf_len = 0;

  ChecksumCheckingParseHandler(ParseHandlerT &p, std::byte *buffer, size_t len)
      : handler{p}, buf{buffer}, buf_len{len} {}

  template <class IStreamT>
  bgcode_parse_handler_result_t
  handle_block(IStreamT &stream, const bgcode_block_header_t &block_header) {
    ChecksumCheckingIStream chkstream{stream, block_header, buf, buf_len};

    bgcode_parse_handler_result_t res;
    res = core::handle_block(handler, chkstream, block_header);

    if (res.result == bgcode_EResult_Success && !res.handled) {
      res.result = skip_block(chkstream, block_header);
      res.handled = true;
    }

    if (res.result == bgcode_EResult_Success && !chkstream.is_checksum_correct())
      res.result = bgcode_EResult_InvalidChecksum;

    return res;
  }

  bool can_continue() { return handler_can_continue(handler); }
};

template <class IStreamT, class BlockHandlerT>
static bgcode_result_t
read_block_params(IStreamT &stream, const bgcode_block_header_t &block_header,
                  BlockHandlerT &params_handler) {
  auto ret = to_underlying(bgcode_EResult_Success);

  switch (block_header.type) {
  case bgcode_EBlockType_FileMetadata:
  case bgcode_EBlockType_PrintMetadata:
  case bgcode_EBlockType_SlicerMetadata:
  case bgcode_EBlockType_PrinterMetadata:
  case bgcode_EBlockType_GCode: {
    std::array<std::byte, block_parameters_length(bgcode_EBlockType_GCode)>
        bytes;
    if (read_from_stream(stream, bytes.data(), bytes.size())) {
      auto encoding_type = load_integer<bgcode_metadata_encoding_type_t>(
          bytes.begin(), bytes.end());
      handle_int_param(params_handler, "encoding_type",
                       static_cast<long>(encoding_type),
                       sizeof(bgcode_metadata_encoding_type_t));
    } else {
      ret = bgcode_EResult_ReadError;
    }
    break;
  }
  case bgcode_EBlockType_Thumbnail: {
    std::array<std::byte, block_parameters_length(bgcode_EBlockType_Thumbnail)>
        bytes;
    if (read_from_stream(stream, bytes.data(), bytes.size())) {
      auto from = bytes.begin();
      auto to = bytes.begin() + sizeof(bgcode_thumbnail_format_t);
      auto thumbnail_format = load_integer<bgcode_thumbnail_format_t>(from, to);
      handle_int_param(params_handler, "thumbnail_format",
                       static_cast<long>(thumbnail_format),
                       sizeof(bgcode_thumbnail_format_t));

      from = to;
      to += sizeof(bgcode_thumbnail_size_t);
      auto thumbnail_width = load_integer<bgcode_thumbnail_size_t>(from, to);
      handle_int_param(params_handler, "thumbnail_width",
                       static_cast<long>(thumbnail_width),
                       sizeof(bgcode_thumbnail_size_t));

      from = to;
      to += sizeof(bgcode_thumbnail_size_t);
      auto thumbnail_height = load_integer<bgcode_thumbnail_size_t>(from, to);
      handle_int_param(params_handler, "thumbnail_height",
                       static_cast<long>(thumbnail_height),
                       sizeof(bgcode_thumbnail_size_t));
    } else {
      ret = bgcode_EResult_ReadError;
    }
    break;
  }
  default:
    ret = bgcode_EResult_InvalidBlockType;
  }

  return ret;
}

template <class IStreamT, class ParseHandlerT>
static bgcode_result_t parse_stream(IStreamT &&stream, ParseHandlerT &&rhandler) {
  static constexpr bgcode_result_t Success = bgcode_EResult_Success;

  bgcode_result_t res = Success;

  bool read_result = true;

  while (read_result && res == Success && !is_stream_finished(stream) &&
         handler_can_continue(rhandler)) {
    bgcode_block_header_t block_header;
    res = core::read(stream, block_header);

    if (res != Success && is_stream_finished(stream))
      res = Success;
    else if (res == Success) {
      auto cbres = handle_block(rhandler, stream, block_header);
      if (cbres.result == Success && !cbres.handled)
        res = skip_block(stream, block_header);
      else
        res = cbres.result;
    }
  }

  return res;
}

template <class IStreamT, class BlockHandler>
bgcode_result_t consume_checksum(IStreamT &&stream,
                                 BlockHandler &bhandler) {
  bgcode_result_t ret = bgcode_EResult_Success;
  auto checksum_type = stream_checksum_type(stream);
  if (checksum_type != bgcode_EChecksumType_None && checksum_type >= 0 &&
      checksum_type < checksum_types_count()) {
    // read block checksum
    Checksum cs(static_cast<bgcode_EChecksumType>(checksum_type));
    ret = core::read(stream, cs);
    handle_checksum(bhandler, cs.data(), cs.size());
  }

  return ret;
}

template <class IStreamT, class BlockHandlerT>
bgcode_result_t parse_block(IStreamT &&stream,
                            const bgcode_block_header_t &block_header,
                            BlockHandlerT &block_handler) {
  static constexpr size_t FallbackBufSize = 64;

  bgcode_result_t ret = 0;

  handle_block_start(block_handler, block_header);

  ret = read_block_params(stream, block_header, block_handler);

  if (ret != bgcode_EResult_Success)
    return ret;

  size_t rem_bytes = block_payload_length(block_header) -
                     block_parameters_length(get_block_type(block_header));

  size_t chunk_size = handler_payload_chunk_size(block_handler);
  std::byte *buf = handler_payload_chunk_buffer(block_handler);

  while (ret == to_underlying(bgcode_EResult_Success) && rem_bytes) {
    std::byte *rbuf = buf;
    if (!rbuf || chunk_size == 0) {
      std::byte fallback_buf[FallbackBufSize];

      size_t to_read = std::min(rem_bytes, FallbackBufSize);
      ret = read_from_stream(stream, fallback_buf, to_read)
                ? bgcode_EResult_Success
                : bgcode_EResult_ReadError;

      rem_bytes -= to_read;
      handle_payload(block_handler, fallback_buf, to_read);
    } else {
      size_t to_read = std::min(rem_bytes, chunk_size);
      ret = read_from_stream(stream, buf, to_read) ? bgcode_EResult_Success
                                                   : bgcode_EResult_ReadError;
      rem_bytes -= to_read;
      handle_payload(block_handler, buf, to_read);
    }
  }

  if (ret == bgcode_EResult_Success) {
    ret = consume_checksum(stream, block_handler);
  }

  return ret;
}

struct MetadataParamsWriter {
  bgcode_metadata_encoding_type_t encoding_type;

  template <class BlockParseHandlerT>
  bool operator()(BlockParseHandlerT &params_handler) {
    return params_handler.int_param("encoding_type", encoding_type,
                                    sizeof(encoding_type));
  }
};

struct GCodeParamsWriter {
  bgcode_gcode_encoding_type_t encoding_type;

  template <class BlockParseHandlerT>
  bool operator()(BlockParseHandlerT &params_handler) {
    return params_handler.int_param("encoding_type", encoding_type,
                                    sizeof(encoding_type));
  }
};

struct ThumbnailParamsWriter {
  bgcode_thumbnail_format_t format;
  bgcode_thumbnail_size_t width, height;

  template <class BlockParseHandlerT>
  bool operator()(BlockParseHandlerT &params_handler) {
    bool ret = true;
    ret && (ret = params_handler.int_param("format", format, sizeof(format)));
    ret && (ret = params_handler.int_param("width", width, sizeof(width)));
    ret && (ret = params_handler.int_param("height", height, sizeof(width)));

    return ret;
  }
};

template <class OStreamT> class ChecksumWriter {
  ReferenceType<OStreamT> m_stream;
  Checksum m_checksum;
  size_t m_counter;

protected:
  ReferenceType<OStreamT> inner_stream() { return m_stream; }

  void reset_counter() { m_counter = 0; }
  size_t get_counter_value() const noexcept { return m_counter; }

public:
  const char *last_error_description() const {
    return core::last_error_description(m_stream);
  }
  bgcode_version_t version() const { return stream_bgcode_version(m_stream); }
  bgcode_checksum_type_t checksum_type() const {
    return stream_checksum_type(m_stream);
  }

  const Checksum &get_checksum() const noexcept { return m_checksum; }

  bool write(const std::byte *buf, size_t len) {
    bool ret = write_to_stream(m_stream, buf, len);
    if (ret) {
      m_counter += len;
      m_checksum.append(buf, len);
    }

    return ret;
  }

  explicit ChecksumWriter(bgcode_checksum_type_t chktype,
                          ReferenceType<OStreamT> ostream)
      : m_stream{ostream},
        m_checksum{static_cast<bgcode_EChecksumType>(chktype)}, m_counter{0} {}

  explicit ChecksumWriter(ReferenceType<OStreamT> ostream)
      : m_stream{ostream}, m_checksum{static_cast<bgcode_EChecksumType>(
                               stream_checksum_type(ostream))},
        m_counter{0} {}
};

template <class OStreamT> class BlockWriter : public ChecksumWriter<OStreamT> {
  bgcode_block_header_t m_blockh;

public:
  bool int_param(const char * /*name*/, long val, size_t bytes) {
    return write_integer(*this, val, bytes);
  }

  explicit BlockWriter(ReferenceType<OStreamT> ostream)
      : ChecksumWriter<OStreamT>(ostream) {}

  template <class ParamsWriterFn>
  bgcode_result_t start_block(const bgcode_block_header_t &header,
                              ParamsWriterFn &&pwriter) {
    bgcode_result_t ret = core::write(this->inner_stream(), header);

    if (ret == bgcode_EResult_Success) {
      m_blockh = header;
      this->reset_counter();
      ret = pwriter(*this) ? bgcode_EResult_Success : bgcode_EResult_WriteError;
    }

    return ret;
  }

  bgcode_result_t finish_block() {
    auto ret = core::write(this->inner_stream(), this->get_checksum());
    this->reset_counter();

    return ret;
  }

  bgcode_result_t write_data(const std::byte *data, size_t len) {
    bgcode_result_t ret = bgcode_EResult_WriteError;

    if (this->get_counter_value() + len <=
        block_content_length(this->get_checksum().get_type(), m_blockh))
      ret = write_to_stream(*this, data, len);

    return ret;
  }
};

class MRes : public std::pmr::memory_resource {
  bgcode_allocator_ref_t bgalloc;

  void *do_allocate(size_t bytes, size_t alignment) override {
    return bgalloc.vtable->allocate(bgalloc.self, bytes, alignment);
  }

  void do_deallocate(void *p, size_t bytes, size_t alignment) override {
    bgalloc.vtable->deallocate(bgalloc.self, p, bytes, alignment);
  }

  bool
  do_is_equal(const std::pmr::memory_resource &) const noexcept override {
    return false;
  }

public:
  MRes(bgcode_allocator_ref_t alloc) : bgalloc{alloc} {}
};

template <class BGObj, class... Args>
BGObj *create_bgobj(bgcode_allocator_ref_t alloc, Args &&...args) {
  bgcode::core::MRes mres{alloc};
  std::pmr::polymorphic_allocator<BGObj> salloc(&mres);

  auto *p = salloc.allocate(1);
  salloc.construct(p, alloc, std::forward<Args>(args)...);

  return p;
}

template <class BGObj> void free_bgobj(BGObj *obj) {
  bgcode::core::MRes mres{obj->allocator};
  using Alloc = std::pmr::polymorphic_allocator<BGObj>;
  Alloc salloc(&mres);

  std::allocator_traits<Alloc>::destroy(salloc, obj);
  salloc.deallocate(obj, 1);
}

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
  FILE *m_fp;
  bgcode_checksum_type_t m_checksum_type;
  bgcode_version_t m_version;

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

  bool is_finished() const { return std::feof(m_fp); }
};

constexpr auto block_types_following() {
  return std::array<bool, block_types_count()>{
  //   FM    GC    SM     PrM    PM    TM
      true, false, false, true, false, false
  };
}

constexpr std::array<bool, block_types_count()>
block_types_following(bgcode_block_type_t blk) {
  switch (blk) {
  case bgcode_EBlockType_FileMetadata:
    //       FM      GC     SM    PrM    PM    TM
    return {false, false, false, true, false, false};
  case bgcode_EBlockType_GCode:
    //       FM    GC    SM     PrM    PM    TM
    return {false, true, false, false, false, false};
  case bgcode_EBlockType_SlicerMetadata:
    //       FM    GC    SM     PrM    PM    TM
    return {false, true, false, false, false, false};
  case bgcode_EBlockType_PrinterMetadata:
    //       FM    GC    SM     PrM    PM    TM
    return {false, false, false, false, false, true};
  case bgcode_EBlockType_PrintMetadata:
    //       FM    GC    SM     PrM    PM    TM
    return {false, false, true, false, false, false};
  case bgcode_EBlockType_Thumbnail:
    //       FM    GC    SM     PrM    PM    TM
    return {false, false, false, false, true, true};
  }

  return {false, false, false, false, false, false};
}

constexpr bool can_follow_block(bgcode_block_type_t block,
                                bgcode_block_type_t prev_block) {
  return block_types_following(prev_block)[block];
}

struct SkipperParseHandler {
  template <class IStreamT>
  bgcode_parse_handler_result_t
  handle_block(IStreamT &&istream, const bgcode_block_header_t &bheader) {
    bgcode_parse_handler_result_t res;
    res.handled = true;
    res.result = skip_block(istream, bheader);

    return res;
  }

  bool can_continue() const { return true; }
};

template <class PHandlerT> class OrderCheckingParseHandler {
  ReferenceType<PHandlerT> m_handler;
  std::optional<bgcode_block_type_t> m_prev_block_type;

public:
  template <class IStreamT>
  bgcode_parse_handler_result_t
  handle_block(IStreamT &stream, const bgcode_block_header_t &block_header) {
    bgcode_parse_handler_result_t res;
    res.handled = true;
    res.result = bgcode_EResult_Success;

    if (m_prev_block_type.has_value() &&
        !can_follow_block(block_header.type, *m_prev_block_type)) {
      res.result = bgcode_EResult_InvalidSequenceOfBlocks;
      res.handled = false;
    } else {
      res = core::handle_block(m_handler, stream, block_header);
    }

    m_prev_block_type = block_header.type;

    return res;
  }

  bool can_continue() const { return true; }

  OrderCheckingParseHandler(PHandlerT &handler) : m_handler{handler} {}
};

template <class BlockParseHandlerT> class AllBlocksParseHandler {
  ReferenceType<BlockParseHandlerT> m_block_parse_handler;

public:
  AllBlocksParseHandler(BlockParseHandlerT &block_parse_handler)
      : m_block_parse_handler{block_parse_handler} {}

  template <class IStreamT>
  bgcode_parse_handler_result_t
  handle_block(IStreamT &&istream, const bgcode_block_header_t &header) {
    bgcode_parse_handler_result_t res;
    res.result =
        bgcode::core::parse_block(istream, header, m_block_parse_handler);
    res.handled = true;

    return res;
  }

  bool can_continue() {
    return handler_status(m_block_parse_handler) == bgcode_BlockParse_OK;
  }
};

} // namespace core
} // namespace bgcode

#endif // BGCODE_IMPL_HPP
