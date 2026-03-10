#pragma once

using byte = unsigned char;

#ifndef _UNREAL_
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;
#endif

using Mutex = std::mutex;
using ConditionVar = std::condition_variable;
using UniqueLock = std::unique_lock<std::mutex>;
using LockGuard = std::scoped_lock<std::mutex>;