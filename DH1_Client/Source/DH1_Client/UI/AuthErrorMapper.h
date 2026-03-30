#pragma once

#include "CoreMinimal.h"

namespace AuthErrorCodes
{
	inline constexpr TCHAR EmailUnverified[] = TEXT("EMAIL_UNVERIFIED");
	inline constexpr TCHAR RegisteredEmail[] = TEXT("REGISTERED_EMAIL");
	inline constexpr TCHAR AccountSuspended[] = TEXT("ACCOUNT_SUSPENDED");
	inline constexpr TCHAR AccountBanned[] = TEXT("ACCOUNT_BANNED");
	inline constexpr TCHAR AccountPendingUnregister[] = TEXT("ACCOUNT_PENDING_UNREGISTER");
}

namespace AuthErrorMapper
{
	bool IsEmailUnverifiedCode(const FString& Code);
	bool IsKnownErrorCode(const FString& Code);
	bool IsKnownStatusCode(int32 StatusCode);
	void ReportUnknownCodeIfAny(const FString& Code, const TCHAR* ContextTag);
	void ReportUnknownStatusIfAny(int32 StatusCode, const TCHAR* ContextTag);
	FString MessageForCode(const FString& Code, const FString& Fallback = TEXT(""));
	FString MessageForStatus(int32 StatusCode);
	FString ResolveMessage(int32 StatusCode, const FString& Code, const FString& ServerMessage);
}
