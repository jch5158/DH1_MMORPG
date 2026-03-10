#pragma once

#include "CoreMinimal.h"

// 1. 언리얼 서드파티 통합 매크로 시작 (기본적인 언리얼 경고 무시)
THIRD_PARTY_INCLUDES_START

#pragma warning(disable: 4668)
#pragma warning(disable: 4125)

// 3. 언리얼 매크로 일시 해제 (C2062 에러의 원인인 verify 충돌 방지)
#pragma push_macro("verify")
#pragma push_macro("check")
#pragma push_macro("cast")
#undef verify
#undef check
#undef cast

// ---------------------------------------------------------
// 4. Shared 폴더에 있는 실제 프로토버퍼 헤더들을 여기서 일괄 인클루드
// (새로운 패킷 헤더가 추가되면 여기에만 추가하시면 됩니다.)
#include "PacketId.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"
#include "Echo.pb.h"
#include "Login.pb.h"
// ---------------------------------------------------------

// 5. 언리얼 매크로 복구
#pragma pop_macro("cast")
#pragma pop_macro("check")
#pragma pop_macro("verify")


// 7. 언리얼 서드파티 통합 매크로 끝
THIRD_PARTY_INCLUDES_END