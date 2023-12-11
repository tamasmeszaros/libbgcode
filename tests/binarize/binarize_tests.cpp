#include <catch_main.hpp>

#include "core/cfile_stream.h"
#include "binarize/unpacker.h"
#include "core/block_reader.h"
#include "core/checksum_reader.h"
#include "core/order_checking_reader.h"

#include <array>
#include <boost/nowide/cstdio.hpp>

class UnpackHandler: public bgcode_block_parse_handler_ref_t {
  std::array<unsigned char, 1024> m_buf;
  static const bgcode_block_parse_handler_vtable_t VTable;

public:
  UnpackHandler(): bgcode_block_parse_handler_ref_t{&VTable, this} {}
};

const bgcode_block_parse_handler_vtable_t UnpackHandler::VTable =
    bgcode_init_block_parse_handler_vtable(
        {.payload_chunk_size =
             [](const void *self) {
               return static_cast<const UnpackHandler *>(self)->m_buf.size();
             },
         .payload_chunk_buffer =
             [](void *self) {
               return static_cast<UnpackHandler *>(self)->m_buf.data();
             },
         .payload =
             [](void * /*self*/, const unsigned char *data_bytes,
                size_t bytes_count) {
               std::string str(data_bytes, data_bytes + bytes_count);
               std::cout << str << "\n";
             }
        });

TEST_CASE("Test deflate decompression")
{
  const std::string filename =
      std::string(TEST_DATA_DIR) + "/mini_cube_b.bgcode";

  FILE *fp = boost::nowide::fopen(filename.c_str(), "rb");

  std::array<unsigned char, 1024> membuf;
  auto alloc = bgcode_init_static_allocator(membuf.data(), membuf.size());

  bgcode_cfile_stream_t *cfilestream =
      bgcode_alloc_cfile_input_stream(alloc, fp, nullptr);

  REQUIRE(cfilestream);
  bgcode_istream_ref_t stream = bgcode_get_cfile_input_stream(cfilestream);

  REQUIRE(stream.self);
  REQUIRE(stream.vtable);

  UnpackHandler handler;

  auto *unpacker =
      bgcode_alloc_unpacker(alloc, handler, bgcode_get_empty_metadata_handler(),
                            bgcode_get_empty_gcode_handler(), 0);

  REQUIRE(unpacker);

  bgcode_block_reader_t *block_reader = bgcode_alloc_block_reader(
      alloc, bgcode_get_unpacking_block_parse_handler(unpacker));

  REQUIRE(block_reader);

  bgcode_checksum_reader_t *checksum_reader = bgcode_alloc_checksum_reader(
      alloc, bgcode_get_block_reader_parse_handler(block_reader), 0);

  REQUIRE(checksum_reader);

  bgcode_order_checking_reader_t *order_checking_reader =
      bgcode_alloc_order_checking_reader(
          alloc, bgcode_get_checksum_checking_parse_handler(checksum_reader));

  REQUIRE(order_checking_reader);

  auto res = bgcode_parse(
      stream, bgcode_get_order_checking_parse_handler(order_checking_reader));

  bgcode_free_unpacker(unpacker);
  bgcode_free_checksum_reader(checksum_reader);
  bgcode_free_order_checking_reader(order_checking_reader);
  bgcode_free_cfile_stream(cfilestream);
  std::fclose(fp);

  REQUIRE(res == bgcode_EResult_Success);
}

