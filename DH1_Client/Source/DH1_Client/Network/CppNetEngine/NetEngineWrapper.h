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

#include "EnginePch.h"
#include "PacketSession.h"

#include "Windows/HideWindowsPlatformTypes.h"

#pragma pop_macro("cast")
#pragma pop_macro("verify")
#pragma pop_macro("check")

THIRD_PARTY_INCLUDES_END