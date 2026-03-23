#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <WinSock2.h>
#include <MSWSock.h>
#include <mstcpip.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <new>
#include <queue>
#include <set>
#include <shared_mutex>
#include <stack>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <client/crash_report_database.h>
#include <client/crashpad_client.h>
#include <client/settings.h>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <fmt/xchar.h>

#include <mimalloc.h>

#include <spdlog/async.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <utf8cpp/utf8.h>

#include "Types.h"
#include "StringUtils.h"
#include "CrashReporter.h"
#include "ISingleton.h"
#include "SocketUtils.h"
#include "NetEngineMacro.h"
#include "NetEngineLogger.h"

#include "ObjectPool.h"
#include "MemoryPool.h"
#include "ObjectAllocator.h"
#include "MemoryAllocator.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"

#include "StlTypes.h"
#include "UniquePtrAllocator.h"
#include "SharedPtrAllocator.h"
#include "SharedPtrTypes.h"
#include "SendBufferAllocator.h"

#include "NetEngineInit.h"