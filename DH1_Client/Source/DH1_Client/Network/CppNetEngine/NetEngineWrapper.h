#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START

#pragma push_macro("check")
#pragma push_macro("verify")
#pragma push_macro("cast")
#undef check
#undef verify
#undef cast

#include "Windows/AllowWindowsPlatformTypes.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "EnginePch.h"
#include "Types.h"
#include "NetEngineInit.h"
#include "CrashReporter.h"
#include "NetSendBuffer.h"
#include "AllocatorUtils.h"
#include "NetEngineMacro.h"
#include "PacketSession.h"
#include "NetworkScheduler.h"
#include "Service.h"

#include "Windows/HideWindowsPlatformTypes.h"
#undef DrawText

#pragma pop_macro("cast")
#pragma pop_macro("verify")
#pragma pop_macro("check")

THIRD_PARTY_INCLUDES_END