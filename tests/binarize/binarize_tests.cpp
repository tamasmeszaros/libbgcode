#include <catch_main.hpp>

#include "core/cfile_stream.h"
#include "binarize/parse_decomp.h"

#include <boost/nowide/cstdio.hpp>

class DecompHandler : public bgcode_handler_ref_t {
  std::array<unsigned char, 1024> m_buf;

  static size_t payload_chunk_size(const void *self) {
    return static_cast<const DecompHandler *>(self)->m_buf.size();
  }

  static unsigned char * payload_chunk_buffer(void *self) {
    return static_cast<DecompHandler *>(self)->m_buf.data();
  }

  static const constexpr bgcode_block_parse_handler_vtable_t BlockHandlerVTable = {
      .payload_chunk_size = payload_chunk_size,
      .payload_chunk_buffer = payload_chunk_buffer,

      .int_param = [](void * /*self*/, const char *name, long value,
                      size_t /*bytes_width*/) {
        std::cout << "parameter " << name << " = " << value << "\n";
      },
      .string_param = [](void * /*self*/, const char *name,
                         const char *value) {
        std::cout << "parameter " << name << " = " << value << "\n";
      },
      .float_param = [](void * /*self*/, const char *name, double value) {
        std::cout << "parameter " << name << " = " << value << "\n";
      },
      .payload =
          [](void * /*self*/, const unsigned char *data_bytes,
             size_t bytes_count) {
            std::string str(data_bytes, data_bytes + bytes_count);
            std::cout << str << "\n";
          },
      .checksum = [](void *self, const unsigned char *checksum_bytes,
                     size_t bytes_count) {},
      .block_start = [](void *self, const bgcode_block_header_t *header){
      }
  };

  static const constexpr bgcode_handler_vtable_t VTable {
    .block_handler_vtable = &BlockHandlerVTable,
    .metadata_handler_vtable = nullptr,
    .gcode_handler_vtable = nullptr
  };

public:
  DecompHandler() : bgcode_handler_ref_t{&VTable, this} {}
};

class DecompParseHandler : public bgcode_parse_handler_ref_t {
  static const constexpr bgcode_parse_handler_vtable_t VTable = {
      .handle_block =
          [](void *self, bgcode_istream_ref_t stream,
             const bgcode_block_header_t *header) {
            bgcode_parse_handler_result_t res;
            DecompHandler decomp_handler;
            std::array<unsigned char, 2048> workbuf;
            res.result = bgcode_parse_block_decompressed(
                stream, header, decomp_handler, workbuf.data(), workbuf.size());
            res.handled = true;

            return res;
          },

      .can_continue = [](void *self) { return true; },
  };

public:
  DecompParseHandler() : bgcode_parse_handler_ref_t{&VTable, this} {}
};

TEST_CASE("Test deflate decompression")
{
  const std::string filename =
      std::string(TEST_DATA_DIR) + "/mini_cube_b.bgcode";

  FILE *fp = boost::nowide::fopen(filename.c_str(), "r");

  bgcode_cfile_stream_t *cfilestream =
      bgcode_init_cfile_input_stream(fp, nullptr);

  REQUIRE(cfilestream);
  bgcode_istream_ref_t stream = bgcode_get_cfile_input_stream(cfilestream);

  REQUIRE(stream.self);
  REQUIRE(stream.vtable);

  DecompParseHandler handler;

  auto res = bgcode_parse(stream, handler);

  bgcode_free_cfile_stream(cfilestream);
  std::fclose(fp);

  REQUIRE(res == bgcode_EResult_Success);
}

