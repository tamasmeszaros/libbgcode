#include "core/block_writer.h"

#include "core/capi_adaptor.hpp"
#include "core/bgcode_impl.hpp"

struct bgcode_block_writer_t
    : public bgcode::core::BlockWriter<bgcode_output_stream_ref_t> {
  using Base = bgcode::core::BlockWriter<bgcode_output_stream_ref_t>;

  static const constexpr bgcode_stream_vtable_t StreamVTable = {
      .last_error_description =
          [](const void *self) {
            return bgcode::core::last_error_description(
                *static_cast<const Base *>(self));
          },
      .version =
          [](const void *self) {
            return bgcode::core::stream_bgcode_version(
                *static_cast<const Base *>(self));
          },
      .checksum_type =
          [](const void *self) {
            return bgcode::core::stream_checksum_type(
                *static_cast<const Base *>(self));
          }};

  static const constexpr bgcode_raw_output_stream_vtable_t RawOStreamVTable = {
      .write =
          [](void *self, const unsigned char *buf, size_t len) {
            return bgcode::core::write_to_stream(
                *static_cast<Base *>(self),
                reinterpret_cast<const std::byte *>(buf), len);
          },
  };

  static const constexpr bgcode_output_stream_vtable_t VTable{
      .stream_vtable = &StreamVTable,
      .raw_ostream_vtable = &RawOStreamVTable,
  };

  bgcode_output_stream_ref_t get_stream() {
    return {.vtable = &VTable, .self = this};
  }

  bgcode_allocator_ref_t allocator;

  bgcode_block_writer_t(bgcode_allocator_ref_t alloc,
                        bgcode_output_stream_ref_t ostream)
      : Base{ostream}, allocator{alloc} {}
};

bgcode_block_writer_t *
bgcode_init_block_writer(bgcode_output_stream_ref_t ostream) {
  return bgcode_alloc_block_writer(bgcode_default_allocator(), ostream);
}

namespace bgcode {

template <class ParamsWriterFn>
static bgcode_result_t
start_block(bgcode_block_writer_t *writer, bgcode_block_type_t block_type,
            bgcode_compression_type_t compression_type,
            bgcode_size_t uncompressed_size, bgcode_size_t compressed_size,
            ParamsWriterFn &&pwriter) {
  bgcode_result_t ret = bgcode_EResult_WriteError;

  if (writer)
    try {
      bgcode_block_header_t header{.type = block_type,
                                   .compression = compression_type,
                                   .uncompressed_size = uncompressed_size,
                                   .compressed_size = compressed_size};
      ret = writer->start_block(header, pwriter);
    } catch (...) {
      ret = bgcode_EResult_UnknownError;
    }

  return ret;
}

} // namespace bgcode

bgcode_output_stream_ref_t
bgcode_get_output_stream(bgcode_block_writer_t *writer) {
  return writer->get_stream();
}

bgcode_result_t bgcode_start_block(bgcode_block_writer_t *w,
                                   bgcode_block_type_t bt,
                                   bgcode_compression_type_t ct,
                                   bgcode_size_t uncompr_sz,
                                   bgcode_size_t compr_sz) {
  return bgcode::start_block(w, bt, ct, uncompr_sz, compr_sz,
                             [](auto &) { return true; });
}

bgcode_result_t
bgcode_start_metadata_block(bgcode_block_writer_t *w, bgcode_block_type_t bt,
                            bgcode_compression_type_t ct,
                            bgcode_size_t uncompr_sz, bgcode_size_t compr_sz,
                            bgcode_metadata_encoding_type_t encoding) {
  return bgcode::start_block(w, bt, ct, uncompr_sz, compr_sz,
                             bgcode::core::MetadataParamsWriter{encoding});
}

bgcode_result_t
bgcode_start_gcode_block(bgcode_block_writer_t *w, bgcode_compression_type_t ct,
                         bgcode_size_t uncompr_sz, bgcode_size_t compr_sz,
                         bgcode_gcode_encoding_type_t encoding) {
  return bgcode::start_block(w, bgcode_EBlockType_GCode, ct, uncompr_sz,
                             compr_sz,
                             bgcode::core::GCodeParamsWriter{encoding});
}

bgcode_result_t bgcode_finish_block(bgcode_block_writer_t *writer) {
  bgcode_result_t ret = bgcode_EResult_WriteError;

  if (writer)
    try {
      ret = writer->finish_block();
    } catch (...) {
      ret = bgcode_EResult_WriteError;
    }

  return ret;
}

bgcode_result_t bgcode_write_block_data(bgcode_block_writer_t *writer,
                                        const unsigned char *data, size_t len) {
  bgcode_result_t ret = bgcode_EResult_WriteError;

  if (writer)
    try {
      ret = writer->write_data(reinterpret_cast<const std::byte *>(data), len);
    } catch (...) {
      ret = bgcode_EResult_WriteError;
    }

  return ret;
}

const unsigned char *
bgcode_block_writer_get_checksum(bgcode_block_writer_t *writer) {
  return writer ? reinterpret_cast<const unsigned char *>(
                      writer->get_checksum().data())
                : nullptr;
}

bgcode_block_writer_t *
bgcode_alloc_block_writer(bgcode_allocator_ref_t alloc,
                          bgcode_output_stream_ref_t ostream) {
  using BlockWriter = bgcode::core::BlockWriter<bgcode_output_stream_ref_t>;

  if (!alloc.vtable)
    return nullptr;

  bgcode_block_writer_t *ret = nullptr;

  try {
    ret = bgcode::core::create_bgobj<bgcode_block_writer_t>(alloc, ostream);
  } catch (...) {
    ret = nullptr;
  }

  return ret;
}

void bgcode_free_block_writer(bgcode_block_writer_t *block_writer) {
  bgcode::core::free_bgobj(block_writer);
}
