#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START

#pragma warning(disable: 4668)
#pragma warning(disable: 4125)

#pragma push_macro("verify")
#pragma push_macro("check")
#pragma push_macro("cast")
#undef verify
#undef check
#undef cast

// ---------------------------------------------------------
#include "Enum.pb.h"
#include "PacketId/PacketId.h"
#include "PacketOption.pb.h"
#include "Struct.pb.h"
#include "Echo.pb.h"
#include "Login.pb.h"
#include "Echo.pb.h"
// ---------------------------------------------------------

#pragma pop_macro("cast")
#pragma pop_macro("check")
#pragma pop_macro("verify")


THIRD_PARTY_INCLUDES_END