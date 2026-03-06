#pragma once

#include <cassert>

#define NET_ASSERT(exp, msg) assert((exp) && (msg))

#define SIZE_OF_16(val) static_cast<int16>(sizeof(val))
#define SIZE_OF_32(val) static_cast<int32>(sizeof(val))
#define ARRAY_LEN_16(arr) static_cast<int16>(sizeof(arr)/sizeof((arr)[0]))
#define ARRAY_LEN_32(arr) static_cast<int32>(sizeof(arr)/sizeof((arr)[0]))