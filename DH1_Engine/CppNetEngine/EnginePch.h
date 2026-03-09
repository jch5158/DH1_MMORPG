#pragma once

// Windows 매크로 충돌 방지
#define NOMINMAX

#include <atomic>
#include <mutex>
#include <thread>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

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
#include <fmt/std.h>
#include <fmt/xchar.h>

#include "NetEngineMacro.h"
#include "Types.h"
#include "StlTypes.h"
#include "SharedPtrUtils.h"
#include "UniquePtrUtils.h"
#include "AllocatorUtils.h"

#include "NetEngineInit.h"
#include "NetEngineLogger.h"
#include "CrashReporter.h"
//#include "IocpCore.h"
//
//#include "Job.h"
//#include "Actor.h"
//#include "ActorScheduler.h"
//#include "TimingWheel.h"