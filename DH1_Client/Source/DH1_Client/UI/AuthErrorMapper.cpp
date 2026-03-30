#include "UI/AuthErrorMapper.h"

#include "Engine/Engine.h"
#include "Misc/ScopeLock.h"

namespace
{
	FCriticalSection GUnknownCodeLock;
	TSet<FString> GReportedUnknownCodes;
	FCriticalSection GUnknownStatusLock;
	TSet<int32> GReportedUnknownStatuses;

	FString NormalizeCode(const FString& Code)
	{
		return Code.TrimStartAndEnd().ToUpper();
	}

	const TMap<FString, FString>& AuthCodeMessageMap()
	{
		static const TMap<FString, FString> Map =
		{
			{ AuthErrorCodes::EmailUnverified, TEXT("이메일 인증이 완료되지 않은 계정입니다. 인증 페이지로 이동합니다.") },
			{ AuthErrorCodes::RegisteredEmail, TEXT("이미 사용중인 이메일입니다.") },
			{ AuthErrorCodes::AccountSuspended, TEXT("일시 정지된 계정입니다.") },
			{ AuthErrorCodes::AccountBanned, TEXT("영구 정지된 계정입니다.") },
			{ AuthErrorCodes::AccountPendingUnregister, TEXT("탈퇴한 회원입니다.") },
		};
		return Map;
	}

	const TMap<int32, FString>& AuthStatusMessageMap()
	{
		static const TMap<int32, FString> Map =
		{
			{ 400, TEXT("요청 값이 올바르지 않습니다.") },
			{ 401, TEXT("인증에 실패했습니다.") },
			{ 403, TEXT("접근 권한이 없습니다.") },
			{ 404, TEXT("요청한 데이터를 찾을 수 없습니다.") },
			{ 409, TEXT("요청이 현재 상태와 충돌합니다.") },
			{ 429, TEXT("요청이 너무 많습니다. 잠시 후 다시 시도해주세요.") },
			{ 500, TEXT("서버 내부 오류가 발생했습니다.") },
			{ 503, TEXT("서버를 사용할 수 없습니다.") },
		};
		return Map;
	}

	void ShowDevToast(const FString& Text)
	{
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Text);
		}
#endif
	}
}

namespace AuthErrorMapper
{
	bool IsEmailUnverifiedCode(const FString& Code)
	{
		return NormalizeCode(Code) == AuthErrorCodes::EmailUnverified;
	}

	bool IsKnownErrorCode(const FString& Code)
	{
		const FString Normalized = NormalizeCode(Code);
		return !Normalized.IsEmpty() && AuthCodeMessageMap().Contains(Normalized);
	}

	bool IsKnownStatusCode(const int32 StatusCode)
	{
		return AuthStatusMessageMap().Contains(StatusCode);
	}

	void ReportUnknownCodeIfAny(const FString& Code, const TCHAR* ContextTag)
	{
		if (Code.IsEmpty() || IsKnownErrorCode(Code))
		{
			return;
		}

		FScopeLock Lock(&GUnknownCodeLock);
		if (GReportedUnknownCodes.Contains(Code))
		{
			return;
		}

		GReportedUnknownCodes.Add(Code);
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AuthErrorMapper] Unknown auth error code detected (%s): %s"),
			ContextTag ? ContextTag : TEXT("UnknownContext"),
			*Code);
		ShowDevToast(FString::Printf(
			TEXT("[DEV] Unknown auth code (%s): %s"),
			ContextTag ? ContextTag : TEXT("UnknownContext"),
			*Code));
	}

	void ReportUnknownStatusIfAny(const int32 StatusCode, const TCHAR* ContextTag)
	{
		if (IsKnownStatusCode(StatusCode))
		{
			return;
		}

		FScopeLock Lock(&GUnknownStatusLock);
		if (GReportedUnknownStatuses.Contains(StatusCode))
		{
			return;
		}

		GReportedUnknownStatuses.Add(StatusCode);
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AuthErrorMapper] Unknown auth status code detected (%s): %d"),
			ContextTag ? ContextTag : TEXT("UnknownContext"),
			StatusCode);
		ShowDevToast(FString::Printf(
			TEXT("[DEV] Unknown auth status (%s): %d"),
			ContextTag ? ContextTag : TEXT("UnknownContext"),
			StatusCode));
	}

	FString MessageForCode(const FString& Code, const FString& Fallback)
	{
		const FString Normalized = NormalizeCode(Code);
		if (const FString* Found = AuthCodeMessageMap().Find(Normalized))
		{
			return *Found;
		}
		return Fallback.IsEmpty() ? FString() : Fallback;
	}

	FString MessageForStatus(const int32 StatusCode)
	{
		if (const FString* Found = AuthStatusMessageMap().Find(StatusCode))
		{
			return *Found;
		}
		return TEXT("서버 처리 중 오류가 발생했습니다.");
	}

	FString ResolveMessage(const int32 StatusCode, const FString& Code, const FString& ServerMessage)
	{
		ReportUnknownCodeIfAny(Code, TEXT("ResolveMessage"));
		ReportUnknownStatusIfAny(StatusCode, TEXT("ResolveMessage"));

		if (!Code.IsEmpty())
		{
			const FString CodeMessage = MessageForCode(Code);
			if (!CodeMessage.IsEmpty())
			{
				return CodeMessage;
			}
		}

		if (!ServerMessage.IsEmpty())
		{
			return ServerMessage;
		}

		return MessageForStatus(StatusCode);
	}
}
