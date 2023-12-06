#include "checksum_reader.h"
#include "bgcode_impl.hpp"

struct bgcode_checksum_reader_t {
  bgcode::core::ChecksumCheckingParseHandler<bgcode_parse_handler_ref_t> handler;

  bgcode_checksum_reader_t(bgcode_parse_handler_ref_t hdl,
                           std::byte *buf, size_t len)
      : handler{hdl, buf, len} {}
};

