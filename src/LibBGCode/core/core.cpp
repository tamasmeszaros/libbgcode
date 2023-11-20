#include "core/core.hpp"
#include "core/core_impl.hpp"
#include <cstring>

namespace bgcode { namespace core {

template<class T>
static bool write_to_file(FILE& file, const T* data, size_t data_size)
{
    const size_t wsize = fwrite(static_cast<const void*>(data), 1, data_size, &file);
    return !ferror(&file) && wsize == data_size;
}

template<class T>
static bool read_from_file(FILE& file, T *data, size_t data_size)
{
    static_assert(!std::is_const_v<T>, "Type of output buffer cannot be const!");

    const size_t rsize = fread(static_cast<void *>(data), 1, data_size, &file);
    return !ferror(&file) && rsize == data_size;
}

EResult verify_block_checksum(FILE& file, const FileHeader& file_header,
                              const BlockHeader& block_header, std::byte* buffer, size_t buffer_size)
{
    CFileStream istream(&file, file_header.checksum_type, file_header.version);
    bgcode_block_header_t blkheader = core::to_bgcode_header(block_header);

    ChecksumCheckingIStream chk_istream{istream, blkheader, buffer, buffer_size};

    EResult ret = EResult::Success;

    // skips the bytes, but calculates checksum along the way
    if (!chk_istream.skip(block_content_length(file_header.checksum_type, blkheader)))
      ret = EResult::ReadError;

    if (!chk_istream.is_checksum_correct())
      ret = EResult::InvalidChecksum;

    return ret;
}

FileHeader::FileHeader()
    : magic{MAGICi32}
    , version{VERSION}
    , checksum_type{static_cast<uint16_t>(EChecksumType::None)}
{}

FileHeader::FileHeader(uint32_t mg, uint32_t ver, uint16_t chk_type)
    : magic{mg}, version{ver}, checksum_type{chk_type}
{}

EResult FileHeader::write(FILE& file) const
{
    FILEOutputStream ostream(&file);
    return static_cast<EResult>(
        core::write_header(ostream, to_bgcode_header(*this)));
}

EResult FileHeader::read(FILE& file, const uint32_t* const max_version)
{
    bgcode_stream_header_t sheader;
    bgcode_result_t res = core::read_header(FILEInputStream{&file},
                                            sheader, max_version);

    if (res == bgcode_EResult_Success) {
      version = sheader.version;
      checksum_type = sheader.checksum_type;
      magic = load_integer<decltype(this->magic)>(sheader.magic.begin(),
                                                  sheader.magic.end());
    }

    return static_cast<EResult>(res);
}

BlockHeader::BlockHeader(uint16_t type, uint16_t compression, uint32_t uncompressed_size, uint32_t compressed_size)
  : type(type)
  , compression(compression)
  , uncompressed_size(uncompressed_size)
  , compressed_size(compressed_size)
{}

long BlockHeader::get_position() const
{
    return m_position;
}

EResult BlockHeader::write(FILE& file)
{
    m_position = ftell(&file);

    auto bheader = core::to_bgcode_header(*this);
    return static_cast<EResult>(core::write(FILEOutputStream{&file}, bheader));
}

EResult BlockHeader::read(FILE& file)
{
    m_position = ftell(&file);

    bgcode_block_header_t bheader;
    bgcode_result_t res = core::read(FILEInputStream{&file}, bheader);
    if (res == bgcode_EResult_Success) {
      type = bheader.type;
      compression = bheader.compression;
      uncompressed_size = bheader.uncompressed_size;
      compressed_size = bheader.compressed_size;
    }

    return static_cast<EResult>(res);
}

size_t BlockHeader::get_size() const {
    return sizeof(type) + sizeof(compression) + sizeof(uncompressed_size) +
        ((compression == (uint16_t)ECompressionType::None)? 0 : sizeof(compressed_size));
}

EResult ThumbnailParams::write(FILE& file) const {
    if (!write_to_file(file, &format, sizeof(format)))
        return EResult::WriteError;
    if (!write_to_file(file, &width, sizeof(width)))
        return EResult::WriteError;
    if (!write_to_file(file, &height, sizeof(height)))
        return EResult::WriteError;
    return EResult::Success;
}

EResult ThumbnailParams::read(FILE& file){
    if (!read_from_file(file, &format, sizeof(format)))
        return EResult::ReadError;
    if (!read_from_file(file, &width, sizeof(width)))
        return EResult::ReadError;
    if (!read_from_file(file, &height, sizeof(height)))
        return EResult::ReadError;
    return EResult::Success;
}

BGCODE_CORE_EXPORT std::string_view translate_result(EResult result)
{
    return translate_result_code(to_underlying(result));
}

BGCODE_CORE_EXPORT EResult is_valid_binary_gcode(FILE &file,
                                                 bool check_contents,
                                                 std::byte *cs_buffer,
                                                 size_t cs_buffer_size) {
    const long curr_pos = ftell(&file);
    rewind(&file);

    bgcode_stream_header_t header;
    FILEInputStream raw_istream{&file};
    bgcode_result_t res = read_header(raw_istream, header, nullptr);

    if (check_contents && res == bgcode_EResult_Success) {
      CFileStream istream{&file, header.checksum_type, header.version};
      SkipperParseHandler skipper_handler;
      OrderCheckingParseHandler order_checking_parse_handler{skipper_handler};
      res = core::parse_stream_checksum_safe(
          istream, order_checking_parse_handler, cs_buffer, cs_buffer_size);
    }

    fseek(&file, curr_pos, SEEK_SET);

    return static_cast<EResult>(res);
}

BGCODE_CORE_EXPORT EResult read_header(FILE& file, FileHeader& header, const uint32_t* const max_version)
{
    rewind(&file);
    return header.read(file, max_version);
}

BGCODE_CORE_EXPORT EResult read_next_block_header(FILE& file, const FileHeader& file_header, BlockHeader& block_header,
    std::byte* cs_buffer, size_t cs_buffer_size)
{
    EResult res = block_header.read(file);
    if (res == EResult::Success && cs_buffer != nullptr && cs_buffer_size > 0) {
        res = verify_block_checksum(file, file_header, block_header, cs_buffer, cs_buffer_size);
        // return to payload position after checksum verification
        if (fseek(&file, block_header.get_position() + static_cast<long>(block_header.get_size()), SEEK_SET) != 0)
            res = EResult::ReadError;
    }

    return res;
}

BGCODE_CORE_EXPORT EResult read_next_block_header(
    FILE &file, const FileHeader &file_header, BlockHeader &block_header,
    EBlockType type, std::byte *cs_buffer, size_t cs_buffer_size) {
    // cache file position
    const long curr_pos = ftell(&file);

    do {
        EResult res = read_next_block_header(file, file_header, block_header, nullptr, 0); // intentionally skip checksum verification
        if (res != EResult::Success)
            // propagate error
            return res;
        else if (feof(&file)) {
            // block not found
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            return EResult::BlockNotFound;
        }
        else if ((EBlockType)block_header.type == type) {
            // block found
            if (cs_buffer != nullptr && cs_buffer_size > 0) {
                // checksum verification requested
                res = verify_block_checksum(file, file_header, block_header, cs_buffer, cs_buffer_size);
                // return to payload position after checksum verification
                if (fseek(&file, block_header.get_position() + (long)block_header.get_size(), SEEK_SET) != 0)
                    res = EResult::ReadError;
                return res; // propagate error or success
            }
            return EResult::Success;
        }

        if (!feof(&file)) {
            res = skip_block(file, file_header, block_header);
            if (res != EResult::Success)
                // propagate error
                return res;
        }
    } while (true);
}

BGCODE_CORE_EXPORT size_t block_parameters_size(EBlockType type)
{
    return block_parameters_length(to_underlying(type));
}

BGCODE_CORE_EXPORT EResult skip_block_content(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    fseek(&file, (long)block_content_size(file_header, block_header), SEEK_CUR);
    return ferror(&file) ? EResult::ReadError : EResult::Success;
}

BGCODE_CORE_EXPORT EResult skip_block(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    fseek(&file, block_header.get_position() + (long)block_header.get_size() + (long)block_content_size(file_header, block_header), SEEK_SET);
    return ferror(&file) ? EResult::ReadError : EResult::Success;
}

BGCODE_CORE_EXPORT size_t block_payload_size(const BlockHeader& block_header)
{
    size_t ret = block_parameters_size((EBlockType)block_header.type);
    ret += ((ECompressionType)block_header.compression == ECompressionType::None) ?
        block_header.uncompressed_size : block_header.compressed_size;
    return ret;
}

BGCODE_CORE_EXPORT size_t checksum_size(EChecksumType type)
{
  switch (type)
  {
  case EChecksumType::None:  { return 0; }
  case EChecksumType::CRC32: { return 4; }
  }
  return 0;
}

BGCODE_CORE_EXPORT size_t block_content_size(const FileHeader& file_header, const BlockHeader& block_header)
{
  return block_payload_size(block_header) + checksum_size((EChecksumType)file_header.checksum_type);
}

uint32_t bgcode_version() noexcept
{
    return VERSION;
}

const char *version() noexcept
{
    return LibBGCode_VERSION;
}

} // namespace core
} // namespace bgcode
