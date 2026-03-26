#pragma once

enum class eSessionState : uint8
{
	Active = 0,
	Pending = 1,		// 중복 로그인으로 대기 중 (재연결 대기)
};

struct GameSessionInfo
{
	uint64 mAccountId = 0;
	uint64 mGatewaySessionId = 0;
	int32 mGatewayServerId = 0;
	eSessionState mState = eSessionState::Active;
	int64 mPendingStartMs = 0;		// Pending 전환 시점 (타임아웃 체크용)
};
