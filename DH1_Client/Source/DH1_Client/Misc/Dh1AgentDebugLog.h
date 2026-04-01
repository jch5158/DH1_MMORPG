#pragma once

#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"

/** Session ab4bf2: append one NDJSON line to workspace root (repo) log. No secrets/PII. */
inline void Dh1AgentAppendNdjson(const TCHAR* HypothesisId, const TCHAR* Location, const TCHAR* Message, const TCHAR* DataJsonObject)
{
	const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("../debug-ab4bf2.log")));
	const int64 Ts = FDateTime::UtcNow().GetTicks() / ETimespan::TicksPerMillisecond;
	const FString Line = FString::Printf(
		TEXT("{\"sessionId\":\"ab4bf2\",\"timestamp\":%lld,\"location\":\"%s\",\"message\":\"%s\",\"hypothesisId\":\"%s\",\"data\":%s}\n"),
		Ts, Location, Message, HypothesisId, DataJsonObject);
	FFileHelper::SaveStringToFile(Line, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}
