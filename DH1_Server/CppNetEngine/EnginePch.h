#pragma once

#define NOMINMAX

#include <iostream>
#include <stack>
#include <queue>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <thread>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <mstcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <mimalloc.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <fmt/std.h>

#include "Types.h"
#include "NetEngineMacro.h"
#include "NetEngineInit.h"
#include "StlTypes.h"
#include "UniquePtrUtils.h"
#include "SharedPtrUtils.h"
#include "AllocatorUtils.h"
#include "CrashReporter.h"
#include "NetEngineLogger.h"
#include "IocpCore.h"
#include "Job.h"
#include "Actor.h"
#include "TimingWheel.h"
#include "ActorScheduler.h"
