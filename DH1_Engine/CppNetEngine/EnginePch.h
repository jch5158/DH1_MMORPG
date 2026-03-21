#pragma once

// Windows 매크로 충돌 방지
#define NOMINMAX

#include <atomic>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <shared_mutex>
#include <stack>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>
#include <tuple>
#include <array>

#include <WinSock2.h>
#include <MSWSock.h>
#include <mstcpip.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <mimalloc.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <fmt/xchar.h>
