#pragma once

#include "CoreMinimal.h"
#include "Containers/StringConv.h"
#include <string>

/**
 * 서버·Protobuf가 넘기는 UTF-8 바이트 시퀀스를 FString으로 변환합니다.
 * Win64 TCHAR는 UTF-16이며, FUTF8ToTCHAR가 UTF-8을 디코드합니다(바이트를 wchar로 벌리는 게 아님).
 */
inline FString Dh1Utf8StdStringToFString(const std::string& Utf8)
{
	if (Utf8.empty())
	{
		return FString();
	}

	const FUTF8ToTCHAR Conv(Utf8.data(), static_cast<int32>(Utf8.size()));
	return FString(Conv.Length(), Conv.Get());
}

/** 널 종료 UTF-8 C 문자열 → FString */
inline FString Dh1Utf8CStringToFString(const char* Utf8)
{
	if (Utf8 == nullptr || *Utf8 == '\0')
	{
		return FString();
	}

	const FUTF8ToTCHAR Conv(Utf8);
	return FString(Conv.Length(), Conv.Get());
}
