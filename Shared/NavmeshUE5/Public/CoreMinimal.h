#pragma once
// UE5 CoreMinimal.h stub for standalone server build (not UE5 engine context)
// Provides only the types and macros needed by UE5's Recast/Detour library.

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cstring>

// ---------------------------------------------------------------------------
// Basic UE types
// ---------------------------------------------------------------------------
typedef uint64_t	UEType_uint64;
typedef uint32_t	UEType_uint32;
typedef int64_t		UEType_int64;
typedef int32_t		UEType_int32;
typedef uint16_t	UEType_uint16;
typedef int16_t		UEType_int16;
typedef uint8_t		UEType_uint8;
typedef int8_t		UEType_int8;
typedef char		UEType_ANSICHAR;
typedef wchar_t		UEType_WIDECHAR;
typedef bool		UEType_bool;

// Short UE type aliases used directly in UE5 source files.
// Use __intN (MSVC built-in) to match engine Types.h and avoid redefinition errors.
#ifndef _UNREAL_
using int8   = __int8;
using int16  = __int16;
using int32  = __int32;
using int64  = __int64;
using uint8  = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;
#endif

// UE5 math constants
#ifndef KINDA_SMALL_NUMBER
#define KINDA_SMALL_NUMBER (1.e-4f)
#endif
#ifndef SMALL_NUMBER
#define SMALL_NUMBER (1.e-8f)
#endif
#ifndef BIG_NUMBER
#define BIG_NUMBER (3.4e+38f)
#endif

// ---------------------------------------------------------------------------
// NAVMESH_API: no DLL import/export needed for static server build
// ---------------------------------------------------------------------------
#ifndef NAVMESH_API
#define NAVMESH_API
#endif

// ---------------------------------------------------------------------------
// Disable UE-specific optional NavMesh features
// ---------------------------------------------------------------------------
#ifndef WITH_NAVMESH_SEGMENT_LINKS
#define WITH_NAVMESH_SEGMENT_LINKS 1
#endif
#ifndef WITH_NAVMESH_CLUSTER_LINKS
#define WITH_NAVMESH_CLUSTER_LINKS 1
#endif
#ifndef WITH_RECAST_QUANTIZED_NORMALS
#define WITH_RECAST_QUANTIZED_NORMALS 0
#endif

// ---------------------------------------------------------------------------
// Logging stubs
// ---------------------------------------------------------------------------
#ifndef UE_LOG
#define UE_LOG(CategoryName, Verbosity, Format, ...) ((void)0)
#endif

#ifndef DECLARE_LOG_CATEGORY_EXTERN
#define DECLARE_LOG_CATEGORY_EXTERN(CategoryName, DefaultVerbosity, CompileTimeVerbosity)
#endif
#ifndef DEFINE_LOG_CATEGORY
#define DEFINE_LOG_CATEGORY(CategoryName)
#endif
#ifndef DEFINE_LOG_CATEGORY_STATIC
#define DEFINE_LOG_CATEGORY_STATIC(CategoryName, DefaultVerbosity, CompileTimeVerbosity)
#endif

// ---------------------------------------------------------------------------
// Assertion stubs
// ---------------------------------------------------------------------------
#ifndef check
#define check(x) assert(x)
#endif
#ifndef checkf
#define checkf(x, ...) assert(x)
#endif
#ifndef ensure
#define ensure(x) (!!(x))
#endif
#ifndef ensureMsgf
#define ensureMsgf(x, ...) (!!(x))
#endif
#ifndef verify
#define verify(x) assert(x)
#endif

// CA_ASSUME / CA_SUPPRESS: static analysis hints, no-op for server
#ifndef CA_ASSUME
#define CA_ASSUME(x) ((void)0)
#endif
#ifndef CA_SUPPRESS
#define CA_SUPPRESS(x)
#endif

// ---------------------------------------------------------------------------
// TEXT macro
// ---------------------------------------------------------------------------
#ifndef TEXT
#define TEXT(x) (x)
#endif

// ---------------------------------------------------------------------------
// UE_DEPRECATED macro
// ---------------------------------------------------------------------------
#ifndef UE_DEPRECATED
#define UE_DEPRECATED(version, message)
#endif
