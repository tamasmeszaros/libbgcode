#include <catch_main.hpp>

#include "core/block_writer.h"
#include "core/checksum_writer.h"
#include "core/cfile_stream.h"
#include "core/core.hpp"
#include "core/null_stream.h"

#include <boost/nowide/cstdio.hpp>

using namespace bgcode::core;

class ScopedFile
{
public:
  explicit ScopedFile(FILE* file) : m_file(file) {}
  ~ScopedFile() { if (m_file != nullptr) fclose(m_file); }
private:
  FILE* m_file{ nullptr };
};

static std::string checksum_type_as_string(EChecksumType type)
{
  switch (type)
  {
  case EChecksumType::None:  { return "None"; }
  case EChecksumType::CRC32: { return "CRC32"; }
  }
  return "";
};

static std::string block_type_as_string(EBlockType type)
{
  switch (type)
  {
  case EBlockType::FileMetadata:    { return "FileMetadata"; }
  case EBlockType::GCode:           { return "GCode"; }
  case EBlockType::SlicerMetadata:  { return "SlicerMetadata"; }
  case EBlockType::PrinterMetadata: { return "PrinterMetadata"; }
  case EBlockType::PrintMetadata:   { return "PrintMetadata"; }
  case EBlockType::Thumbnail:       { return "Thumbnail"; }
  }
  return "";
};

static std::string compression_type_as_string(ECompressionType type)
{
  switch (type)
  {
  case ECompressionType::None:            { return "None"; }
  case ECompressionType::Deflate:         { return "Deflate"; }
  case ECompressionType::Heatshrink_11_4: { return "Heatshrink 11,4"; }
  case ECompressionType::Heatshrink_12_4: { return "Heatshrink 12,4"; }
  }
  return "";
};

static std::string metadata_encoding_as_string(EMetadataEncodingType type)
{
  switch (type)
  {
  case EMetadataEncodingType::INI: { return "INI"; }
  }
  return "";
};

static std::string gcode_encoding_as_string(EGCodeEncodingType type)
{
  switch (type)
  {
  case EGCodeEncodingType::None:             { return "None"; }
  case EGCodeEncodingType::MeatPack:         { return "MeatPack"; }
  case EGCodeEncodingType::MeatPackComments: { return "MeatPackComments"; }
  }
  return "";
};

static std::string thumbnail_format_as_string(EThumbnailFormat type)
{
  switch (type)
  {
  case EThumbnailFormat::JPG: { return "JPG"; }
  case EThumbnailFormat::PNG: { return "PNG"; }
  case EThumbnailFormat::QOI: { return "QOI"; }
  }
  return "";
};

TEST_CASE("File transversal", "[Core]")
{
  const std::string filename = std::string(TEST_DATA_DIR) + "/mini_cube_b.bgcode";
  std::cout << "\nTEST: File transversal\n";
  std::cout << "File:" << filename << "\n";

  const size_t MAX_CHECKSUM_CACHE_SIZE = 2048;
  std::byte checksum_verify_buffer[MAX_CHECKSUM_CACHE_SIZE];

  FILE* file = boost::nowide::fopen(filename.c_str(), "rb");
  REQUIRE(file != nullptr);
  ScopedFile scoped_file(file);
  REQUIRE(is_valid_binary_gcode(*file, true, checksum_verify_buffer, sizeof(checksum_verify_buffer)) == EResult::Success);

  fseek(file, 0, SEEK_END);
  const long file_size = ftell(file);
  rewind(file);

  FileHeader file_header;
  REQUIRE(read_header(*file, file_header, nullptr) == EResult::Success);
  std::cout << "Checksum type: " << checksum_type_as_string((EChecksumType)file_header.checksum_type) << "\n";

  BlockHeader block_header;

  do
  {
    // read block header
    REQUIRE(read_next_block_header(*file, file_header, block_header, checksum_verify_buffer, sizeof(checksum_verify_buffer)) == EResult::Success);
    std::cout << "Block: " << block_type_as_string((EBlockType)block_header.type);
    std::cout << " - compression: " << compression_type_as_string((ECompressionType)block_header.compression);
    switch ((EBlockType)block_header.type)
    {
    case EBlockType::FileMetadata:
    case EBlockType::PrinterMetadata:
    case EBlockType::PrintMetadata:
    case EBlockType::SlicerMetadata:
    {
      const long curr_pos = ftell(file);
      uint16_t encoding;
      const size_t rsize = fread(&encoding, 1, sizeof(encoding), file);
      REQUIRE((ferror(file) == 0 && rsize == sizeof(encoding)));
      fseek(file, curr_pos, SEEK_SET);
      std::cout << " - encoding: " << metadata_encoding_as_string((EMetadataEncodingType)encoding);
      break;
    }
    case EBlockType::GCode:
    {
      const long curr_pos = ftell(file);
      uint16_t encoding;
      const size_t rsize = fread(&encoding, 1, sizeof(encoding), file);
      REQUIRE((ferror(file) == 0 && rsize == sizeof(encoding)));
      fseek(file, curr_pos, SEEK_SET);
      std::cout << " - encoding: " << gcode_encoding_as_string((EGCodeEncodingType)encoding);
      break;
    }
    case EBlockType::Thumbnail:
    {
      const long curr_pos = ftell(file);
      ThumbnailParams thumbnail_params;
      REQUIRE(thumbnail_params.read(*file) == EResult::Success);
      fseek(file, curr_pos, SEEK_SET);
      std::cout << " - format: " << thumbnail_format_as_string((EThumbnailFormat)thumbnail_params.format);
      std::cout << " (size: " << thumbnail_params.width << "x" << thumbnail_params.height << ")";
      break;
    }
    default: { break; }
    }
    std::cout << " - data size: " << ((block_header.compressed_size == 0) ? block_header.uncompressed_size : block_header.compressed_size);
    std::cout << "\n";

           // move to next block header
    REQUIRE(skip_block(*file, file_header, block_header) == EResult::Success);
    if (ftell(file) == file_size)
      break;
  } while (true);
}

TEST_CASE("Search for GCode blocks", "[Core]")
{
  const std::string filename = std::string(TEST_DATA_DIR) + "/mini_cube_b.bgcode";
  std::cout << "\nTEST: Search for GCode blocks\n";
  std::cout << "File:" << filename << "\n";

  const size_t MAX_CHECKSUM_CACHE_SIZE = 2048;
  std::byte checksum_verify_buffer[MAX_CHECKSUM_CACHE_SIZE];

  FILE* file = boost::nowide::fopen(filename.c_str(), "rb");
  REQUIRE(file != nullptr);
  ScopedFile scoped_file(file);
  REQUIRE(is_valid_binary_gcode(*file, true, checksum_verify_buffer, sizeof(checksum_verify_buffer)) == EResult::Success);

  fseek(file, 0, SEEK_END);
  const long file_size = ftell(file);
  rewind(file);

  FileHeader file_header;
  REQUIRE(read_header(*file, file_header, nullptr) == EResult::Success);

  BlockHeader block_header;

  do
  {
    // search and read block header by type
    REQUIRE(read_next_block_header(*file, file_header, block_header, EBlockType::GCode, checksum_verify_buffer, sizeof(checksum_verify_buffer)) == EResult::Success);
    std::cout << "Block type: " << block_type_as_string((EBlockType)block_header.type) << "\n";

           // move to next block header
    REQUIRE(skip_block(*file, file_header, block_header) == EResult::Success);
    if (ftell(file) == file_size)
      break;
  } while (true);
}

class SkipperParseHandler : public bgcode_parse_handler_ref_t {
  static const constexpr bgcode_parse_handler_vtable_t VTable =

      {
      .handle_block =
          [](void *self, bgcode_istream_ref_t stream,
             const bgcode_block_header_t *header) {
            bgcode_parse_handler_result_t res;
            res.result = bgcode_skip_block(stream, header);
            res.handled = true;

            return res;
          },

      .can_continue = [](void *self) { return true; },
  };

public:
  SkipperParseHandler() : bgcode_parse_handler_ref_t{&VTable, this} {}
};

static constexpr size_t MemSize = 100;

TEST_CASE("Pass through a binary gcode file", "[streams][cfile]") {
  using namespace bgcode::core;

  const std::string filename =
      std::string(TEST_DATA_DIR) + "/mini_cube_b.bgcode";

  FILE *fp = boost::nowide::fopen(filename.c_str(), "r");

  std::array<unsigned char, MemSize> membuf;
  bgcode_allocator_ref_t allocator =
      bgcode_init_static_allocator(membuf.data(), membuf.size());

  bgcode_cfile_stream_t *cfilestream =
      bgcode_alloc_cfile_input_stream(allocator, fp, nullptr);
  REQUIRE(cfilestream);
  bgcode_istream_ref_t stream = bgcode_get_cfile_input_stream(cfilestream);

  REQUIRE(stream.self);
  REQUIRE(stream.vtable);

  auto res = bgcode_parse(stream, SkipperParseHandler{});

  bgcode_free_cfile_stream(cfilestream);
  std::fclose(fp);

  REQUIRE(res == bgcode_EResult_Success);
}

class CB : public bgcode_parse_handler_ref_t {
  static const constexpr bgcode_parse_handler_vtable_t VTable = {
      .handle_block =
      [](void *self, bgcode_istream_ref_t stream,
             const bgcode_block_header_t *header) {
            return bgcode_parse_handler_result_t{};
          },
      .can_continue = [](void *self) { return true; }};

public:
  CB() : bgcode_parse_handler_ref_t{&VTable, this} {}
};

TEST_CASE("Checksum of a binary gcode file", "[streams][cfile]") {
  using namespace bgcode::core;

  const std::string filename =
      std::string(TEST_DATA_DIR) + "/mini_cube_b.bgcode";

  FILE *fp = boost::nowide::fopen(filename.c_str(), "r");

  std::array<unsigned char, MemSize> membuf;
  bgcode_allocator_ref_t allocator =
      bgcode_init_static_allocator(membuf.data(), membuf.size());

  bgcode_cfile_stream_t *cfilestream =
      bgcode_alloc_cfile_input_stream(allocator, fp, nullptr);
  bgcode_istream_ref_t stream = bgcode_get_cfile_input_stream(cfilestream);

  REQUIRE(stream.self);
  REQUIRE(stream.vtable);

  CB handler;
  unsigned char chkbuf[64];
  auto res = bgcode_checksum_safe_parse(stream, handler, chkbuf, 64);

  std::fclose(fp);

  REQUIRE(res == bgcode_EResult_Success);
}

class GenericBlockParseHandler : public bgcode_block_parse_handler_ref_t {
  static const bgcode_block_parse_handler_vtable_t VTable;
public:
  GenericBlockParseHandler()
      : bgcode_block_parse_handler_ref_t{&VTable, this} {}
};

const bgcode_block_parse_handler_vtable_t GenericBlockParseHandler::VTable = bgcode_init_block_parse_handler_vtable({});

class TestBlockParseHandler : public bgcode_parse_handler_ref_t {
  static const constexpr bgcode_parse_handler_vtable_t VTable = {
      .handle_block =
      [](void *self, bgcode_istream_ref_t stream,
             const bgcode_block_header_t *header) {
            bgcode_parse_handler_result_t res;
            GenericBlockParseHandler block_handler;
            std::cout << "Parsing block with type: "
                      << bgcode_get_block_type(header) << "\n";
            res.result = bgcode_parse_block(stream, header, block_handler);
            res.handled = true;

            return res;
          },

      .can_continue = [](void *self) { return true; },
  };

public:
  TestBlockParseHandler() : bgcode_parse_handler_ref_t{&VTable, this} {}
};

TEST_CASE("Parsing binary gcode file blocks", "[streams][cfile]") {
  using namespace bgcode::core;

  const std::string filename =
      std::string(TEST_DATA_DIR) + "/mini_cube_b.bgcode";

  FILE *fp = boost::nowide::fopen(filename.c_str(), "r");
  std::array<unsigned char, MemSize> membuf;
  bgcode_allocator_ref_t allocator =
      bgcode_init_static_allocator(membuf.data(), membuf.size());

  bgcode_cfile_stream_t *cfilestream =
      bgcode_alloc_cfile_input_stream(allocator, fp, nullptr);
  REQUIRE(cfilestream);
  bgcode_istream_ref_t stream = bgcode_get_cfile_input_stream(cfilestream);

  REQUIRE(stream.self);
  REQUIRE(stream.vtable);

  TestBlockParseHandler parse_handler;
  unsigned char chkbuf[64];
  auto res = bgcode_checksum_safe_parse(stream, parse_handler, chkbuf, 64);

  bgcode_free_cfile_stream(cfilestream);
  std::fclose(fp);

  REQUIRE(res == bgcode_EResult_Success);
}

class ChecksumCalcBlockParseHandler : public bgcode_block_parse_handler_ref_t {

  static void f_checksum(void *self, const unsigned char *checksum_bytes,
                         size_t bytes_count) {
    std::copy(checksum_bytes, checksum_bytes + bytes_count,
              static_cast<ChecksumCalcBlockParseHandler *>(self)
                  ->last_checksum.data());
  }

  static const bgcode_block_parse_handler_vtable_t VTable;

public:
  std::array<unsigned char, 4> last_checksum;

  ChecksumCalcBlockParseHandler()
      : bgcode_block_parse_handler_ref_t{&VTable, this} {}
};

const bgcode_block_parse_handler_vtable_t ChecksumCalcBlockParseHandler::VTable =
    bgcode_init_block_parse_handler_vtable({.checksum = f_checksum});

class ChecksumCalcParseHandler : public bgcode_parse_handler_ref_t {
  static const constexpr bgcode_parse_handler_vtable_t VTable = {
      .handle_block =
      [](void *self, bgcode_istream_ref_t stream,
             const bgcode_block_header_t *header) {
            bgcode_parse_handler_result_t res;

            unsigned char membuf[150];
            bgcode_allocator_ref_t alloc =
                bgcode_init_static_allocator(membuf, 150);

            auto *chkout = bgcode_alloc_checksum_writer(
                alloc, bgcode_EChecksumType_CRC32, header,
                bgcode_get_null_output_stream());
            bgcode_ostream_ref_t chk_ostream =
                bgcode_get_checksum_writer_ostream(chkout);

            size_t write_bytes = bgcode_block_payload_size(header);

            unsigned char rbuf[100];
            while (write_bytes > 0) {
              size_t bytes = std::min(write_bytes, size_t{100});
              bgcode_read_from_stream(stream, rbuf, bytes);
              bgcode_write_to_stream(chk_ostream, rbuf, bytes);
              write_bytes -= bytes;
            }

            std::array<unsigned char, 4> wchk;
            bgcode_get_checksum(chkout, wchk.data(), wchk.size());

            std::array<unsigned char, 4> schk;
            bgcode_read_from_stream(
                stream, schk.data(),
                bgcode_checksum_size(bgcode_get_stream_checksum_type(
                    bgcode_get_istream_base(stream))));

            if (wchk == schk)
              res.result = bgcode_EResult_Success;
            else
              res.result = bgcode_EResult_InvalidChecksum;

            bgcode_free_checksum_writer(chkout);

            res.handled = true;

            return res;
          },

      .can_continue = [](void */*self*/) { return true; },
  };

public:
  ChecksumCalcParseHandler() : bgcode_parse_handler_ref_t{&VTable, this} {}
};

TEST_CASE("Parsing manual checksum calc", "[streams][cfile]") {
  using namespace bgcode::core;

  const std::string filename =
      std::string(TEST_DATA_DIR) + "/mini_cube_b.bgcode";

  FILE *fp = boost::nowide::fopen(filename.c_str(), "r");

  unsigned char membuf[100];
  bgcode_allocator_ref_t alloc = bgcode_init_static_allocator(membuf,
  100);
  auto *cfilestream = bgcode_alloc_cfile_input_stream(alloc, fp, nullptr);
  bgcode_istream_ref_t stream = bgcode_get_cfile_input_stream(cfilestream);

  REQUIRE(stream.self);
  REQUIRE(stream.vtable);

  ChecksumCalcParseHandler parse_handler;
  auto res = bgcode_parse(stream, parse_handler);

  bgcode_free_cfile_stream(cfilestream);
  std::fclose(fp);

  REQUIRE(res == bgcode_EResult_Success);
}

TEST_CASE("Test allocation", "[alloc]") {
  std::array<unsigned char, 100> memory;

  bgcode_allocator_ref_t alloc =
      bgcode_init_static_allocator(memory.data(), memory.size());

  bgcode_stream_header_t *stream_header = bgcode_alloc_stream_header(alloc);

  bgcode_free_stream_header(stream_header);
}
