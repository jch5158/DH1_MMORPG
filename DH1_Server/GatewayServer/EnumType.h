#pragma once

enum class eGatewayStatus : uint8 
{
    Online = 0,       // 일반 유저 접속 가능
    Maintenance = 1,  // 점검 중
    Full = 2,         // 수용 인원 초과
    ShuttingDown = 3  // 패치/종료 예정
};