#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START

#pragma warning(disable: 4125)
#pragma warning(disable: 4668)
#pragma warning(disable: 4800)
#pragma warning(disable: 4582)
#pragma warning(disable: 4583)

#pragma push_macro("check")
#pragma push_macro("verify")
#pragma push_macro("cast")
#undef check
#undef verify
#undef cast

#include "Enum.pb.h"
#include "PacketOption.pb.h"
#include "Struct.pb.h"
#include "Login.pb.h"
#include "Heartbeat.pb.h"
#include "Echo.pb.h"

#pragma pop_macro("cast")
#pragma pop_macro("verify")
#pragma pop_macro("check")

THIRD_PARTY_INCLUDES_END

#include "PacketId/PacketId.h"