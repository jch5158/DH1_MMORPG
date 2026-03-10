#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START

// 1. [핵심] 언리얼 매크로(check, verify 등)가 자체 엔진(fmt, STL) 코드를 오염시키는 것 방지
#pragma push_macro("check")
#pragma push_macro("verify")
#pragma push_macro("cast")
#undef check
#undef verify
#undef cast

// 3. 윈도우 네이티브 매크로 충돌 방지 가드 시작
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"

// ---------------------------------------------------------
// 4. 자체 네트워크 엔진 헤더 인클루드
#include "EnginePch.h"
#include "PacketSession.h"
// ---------------------------------------------------------

// 5. 윈도우 매크로 가드 종료
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

// 6. 언리얼 매크로 복구
#pragma pop_macro("cast")
#pragma pop_macro("verify")
#pragma pop_macro("check")

THIRD_PARTY_INCLUDES_END