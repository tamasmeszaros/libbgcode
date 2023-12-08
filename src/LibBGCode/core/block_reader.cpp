#include "block_reader.h"
#include "bgcode_impl.hpp"
#include "capi_adaptor.hpp"

struct bgcode_block_reader_t : public bgcode::core::AllBlocksParseHandler<
                                   bgcode_block_parse_handler_ref_t> {

  bgcode_allocator_ref_t allocator;
};
