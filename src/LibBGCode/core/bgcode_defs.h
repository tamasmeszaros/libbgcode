#ifndef BGCODE_DEFS_H
#define BGCODE_DEFS_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t bgcode_version_t;

typedef uint32_t bgcode_result_t;

typedef uint32_t bgcode_size_t;

typedef uint16_t bgcode_checksum_type_t;

typedef uint16_t bgcode_block_type_t;

typedef uint16_t bgcode_compression_type_t;

typedef uint16_t bgcode_metadata_encoding_type_t;

typedef uint16_t bgcode_gcode_encoding_type_t;

typedef uint16_t bgcode_thumbnail_format_t;

typedef uint16_t bgcode_thumbnail_size_t;

enum bgcode_EResult : bgcode_result_t {
  bgcode_EResult_Success,
  bgcode_EResult_ReadError,
  bgcode_EResult_WriteError,
  bgcode_EResult_InvalidMagicNumber,
  bgcode_EResult_InvalidVersionNumber,
  bgcode_EResult_InvalidChecksumType,
  bgcode_EResult_InvalidBlockType,
  bgcode_EResult_InvalidCompressionType,
  bgcode_EResult_InvalidMetadataEncodingType,
  bgcode_EResult_InvalidGCodeEncodingType,
  bgcode_EResult_DataCompressionError,
  bgcode_EResult_DataUncompressionError,
  bgcode_EResult_MetadataEncodingError,
  bgcode_EResult_MetadataDecodingError,
  bgcode_EResult_GCodeEncodingError,
  bgcode_EResult_GCodeDecodingError,
  bgcode_EResult_BlockNotFound,
  bgcode_EResult_InvalidChecksum,
  bgcode_EResult_InvalidThumbnailFormat,
  bgcode_EResult_InvalidThumbnailWidth,
  bgcode_EResult_InvalidThumbnailHeight,
  bgcode_EResult_InvalidThumbnailDataSize,
  bgcode_EResult_InvalidBinaryGCodeFile,
  bgcode_EResult_InvalidAsciiGCodeFile,
  bgcode_EResult_InvalidSequenceOfBlocks,
  bgcode_EResult_InvalidBuffer,
  bgcode_EResult_AlreadyBinarized,
  bgcode_EResult_MissingPrinterMetadata,
  bgcode_EResult_MissingPrintMetadata,
  bgcode_EResult_MissingSlicerMetadata,
  bgcode_EResult_OutOfMemory,
  bgcode_EResult_UnknownError
};

enum bgcode_EChecksumType : bgcode_checksum_type_t {
  bgcode_EChecksumType_None,
  bgcode_EChecksumType_CRC32,
};

enum bgcode_EBlockType : bgcode_block_type_t {
  bgcode_EBlockType_FileMetadata,
  bgcode_EBlockType_GCode,
  bgcode_EBlockType_SlicerMetadata,
  bgcode_EBlockType_PrinterMetadata,
  bgcode_EBlockType_PrintMetadata,
  bgcode_EBlockType_Thumbnail,
};

enum bgcode_ECompressionType : bgcode_compression_type_t {
  bgcode_ECompressionType_None,
  bgcode_ECompressionType_Deflate,
  bgcode_ECompressionType_Heatshrink_11_4,
  bgcode_ECompressionType_Heatshrink_12_4,
};

enum bgcode_EMetadataEncodingType : bgcode_metadata_encoding_type_t {
  bgcode_EMetadataEncodingType_INI
};

enum bgcode_EGCodeEncodingType : bgcode_gcode_encoding_type_t {
  bgcode_EGCodeEncodingType_None,
  bgcode_EGCodeEncodingType_MeatPack,
  bgcode_EGCodeEncodingType_MeatPackComments,
};

enum bgcode_EThumbnailFormat : bgcode_thumbnail_format_t {
  bgcode_EThumbnailFormat_PNG,
  bgcode_EThumbnailFormat_JPG,
  bgcode_EThumbnailFormat_QOI,
};

struct bgcode_block_header_t;

typedef struct bgcode_block_header_t bgcode_block_header_t;

struct bgcode_stream_header_t;

typedef struct bgcode_stream_header_t bgcode_stream_header_t;

typedef struct {
  bool handled;
  bgcode_result_t result;
} bgcode_parse_handler_result_t;

typedef struct {
  void *(*const allocate)(void *self, size_t bytes, size_t alignment);
  void (*const deallocate)(void *self, void *ptr, size_t bytes,
                           size_t alignment);
} bgcode_allocator_vtable_t;

typedef struct {
  const bgcode_allocator_vtable_t *const vtable;
  void *const self;
} bgcode_allocator_ref_t;

#endif // BGCODE_DEFS_H
