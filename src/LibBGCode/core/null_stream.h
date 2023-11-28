#ifndef BGCODE_NULL_STREAM_H
#define BGCODE_NULL_STREAM_H

#include "LibBGCode/core/bgcode_lowlevel.h"
#include "LibBGCode/core/export.h"

#ifdef __cplusplus
extern "C" {
#endif

BGCODE_CORE_EXPORT bgcode_istream_ref_t bgcode_get_null_input_stream();

BGCODE_CORE_EXPORT bgcode_ostream_ref_t bgcode_get_null_output_stream();

BGCODE_CORE_EXPORT bgcode_raw_istream_ref_t
bgcode_get_null_raw_input_stream();

BGCODE_CORE_EXPORT bgcode_raw_ostream_ref_t
bgcode_get_null_raw_output_stream();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // NULL_STREAM_H
