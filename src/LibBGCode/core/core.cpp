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
    bgcode_block_header_t blkheader{block_header.type, block_header.compression,
                                    block_header.uncompressed_size,
                                    block_header.compressed_size};
    ChecksumCheckingIStream chk_istream{istream, blkheader, buffer, buffer_size};

    EResult ret = EResult::Success;

    // skips the bytes, but calculates checksum along the way
    if (!chk_istream.skip(block_payload_size(block_header)))
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

//    if (magic != MAGICi32)
//        return EResult::InvalidMagicNumber;
//    if (checksum_type >= checksum_types_count())
//        return EResult::InvalidChecksumType;

//    if (!write_to_file(file, &magic, sizeof(magic)))
//       return EResult::WriteError;
//    if (!write_to_file(file, &version, sizeof(version)))
//        return EResult::WriteError;
//    if (!write_to_file(file, &checksum_type, sizeof(checksum_type)))
//        return EResult::WriteError;

    return EResult::Success;
}

EResult FileHeader::read(FILE& file, const uint32_t* const max_version)
{
    if (!read_from_file(file, &magic, sizeof(magic)))
        return EResult::ReadError;
    if (magic != MAGICi32)
        return EResult::InvalidMagicNumber;

    if (!read_from_file(file, &version, sizeof(version)))
        return EResult::ReadError;
    if (max_version != nullptr && version > *max_version)
        return EResult::InvalidVersionNumber;

    if (!read_from_file(file, &checksum_type, sizeof(checksum_type)))
        return EResult::ReadError;
    if (checksum_type >= checksum_types_count())
        return EResult::InvalidChecksumType;

    return EResult::Success;
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
    if (!write_to_file(file, &type, sizeof(type)))
        return EResult::WriteError;
    if (!write_to_file(file, &compression, sizeof(compression)))
        return EResult::WriteError;
    if (!write_to_file(file, &uncompressed_size, sizeof(uncompressed_size)))
        return EResult::WriteError;
    if (compression != (uint16_t)ECompressionType::None) {
        if (!write_to_file(file, &compressed_size, sizeof(compressed_size)))
            return EResult::WriteError;
    }
    return EResult::Success;
}

EResult BlockHeader::read(FILE& file)
{
    m_position = ftell(&file);
    if (!read_from_file(file, &type, sizeof(type)))
        return EResult::ReadError;
    if (type >= block_types_count())
        return EResult::InvalidBlockType;

    if (!read_from_file(file, &compression, sizeof(compression)))
        return EResult::ReadError;
    if (compression >= compression_types_count())
        return EResult::InvalidCompressionType;

    if (!read_from_file(file, &uncompressed_size, sizeof(uncompressed_size)))
        return EResult::ReadError;
    if (compression != (uint16_t)ECompressionType::None) {
        if (!read_from_file(file, &compressed_size, sizeof(compressed_size)))
            return EResult::ReadError;
    }

    return EResult::Success;
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

BGCODE_CORE_EXPORT EResult is_valid_binary_gcode(FILE& file, bool check_contents, std::byte* cs_buffer, size_t cs_buffer_size)
{
    // cache file position
    const long curr_pos = ftell(&file);
    rewind(&file);

    // check magic number
    std::array<char, 4> magic;
    const size_t rsize = fread((void*)magic.data(), 1, magic.size(), &file);
    if (ferror(&file) && rsize != magic.size())
        return EResult::ReadError;
    else if (magic != MAGIC) {
        // restore file position
        fseek(&file, curr_pos, SEEK_SET);
        return EResult::InvalidMagicNumber;
    }

    // check contents
    if (check_contents) {
        fseek(&file, 0, SEEK_END);
        const long file_size = ftell(&file);
        rewind(&file);

        // read header
        FileHeader file_header;
        EResult res = read_header(file, file_header, nullptr);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        BlockHeader block_header;
        // read file metadata block header, if present
        res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        if ((EBlockType)block_header.type != EBlockType::FileMetadata &&
            (EBlockType)block_header.type != EBlockType::PrinterMetadata) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            return EResult::InvalidBlockType;
        }

        // read printer metadata block header, if file metadata block is present
        if ((EBlockType)block_header.type == EBlockType::FileMetadata) {
            res = skip_block(file, file_header, block_header);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
        }
        if ((EBlockType)block_header.type != EBlockType::PrinterMetadata) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            return EResult::InvalidBlockType;
        }

        // read thumbnails block headers, if present
        res = skip_block(file, file_header, block_header);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        while ((EBlockType)block_header.type == EBlockType::Thumbnail) {
            res = skip_block(file, file_header, block_header);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
        }

        // read print metadata block header
        if ((EBlockType)block_header.type != EBlockType::PrintMetadata) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            return EResult::InvalidBlockType;
        }

        // read slicer metadata block header
        res = skip_block(file, file_header, block_header);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        if ((EBlockType)block_header.type != EBlockType::SlicerMetadata) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            return EResult::InvalidBlockType;
        }

        // read gcode block headers
        do {
            res = skip_block(file, file_header, block_header);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            if (ftell(&file) == file_size)
                break;
            res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            if ((EBlockType)block_header.type != EBlockType::GCode) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                return EResult::InvalidBlockType;
            }
        } while (!feof(&file));
    }

    fseek(&file, curr_pos, SEEK_SET);
    return EResult::Success;
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

BGCODE_CORE_EXPORT EResult read_next_block_header(FILE& file, const FileHeader& file_header, BlockHeader& block_header, EBlockType type,
    std::byte* cs_buffer, size_t cs_buffer_size)
{
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
