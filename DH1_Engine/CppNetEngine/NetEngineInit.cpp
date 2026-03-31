#include "pch.h"
#include "NetEngineInit.h"

#include <cstdio>
#include <optional>

namespace
{
	std::string GetProcessStemName()
	{
		wchar_t buffer[MAX_PATH];
		if (GetModuleFileNameW(nullptr, buffer, MAX_PATH) > 0)
		{
			return std::filesystem::path(buffer).stem().string();
		}
		return "Unknown";
	}

	void TrimInPlace(std::string& s)
	{
		while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
		{
			s.pop_back();
		}
		size_t i = 0;
		while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n'))
		{
			++i;
		}
		if (i > 0)
		{
			s.erase(0, i);
		}
	}

	/** 파일 로그 경로는 DH1_LOG_DIR만 사용 (미설정 시 파일 싱크 비활성). */
	std::optional<std::string> ResolveLogDirectoryFromEnv()
	{
		char* pValue = nullptr;
		size_t size = 0;
		if (_dupenv_s(&pValue, &size, "DH1_LOG_DIR") != 0 || pValue == nullptr)
		{
			return std::nullopt;
		}

		std::string result(pValue);
		free(pValue);
		TrimInPlace(result);
		if (result.empty())
		{
			return std::nullopt;
		}

		while (!result.empty() && (result.back() == '/' || result.back() == '\\'))
		{
			result.pop_back();
		}

		return result + "/" + GetProcessStemName();
	}
}

NetEngineInit::NetEngineInit(const NetEngineConfig& config)
{
	SocketUtils::Init();

	NetEngineConfig resolved = config;

	if (resolved.logger.bEnableFile && !resolved.logger.bIsUnrealClient)
	{
		if (const std::optional<std::string> dir = ResolveLogDirectoryFromEnv())
		{
			resolved.logger.logDirectory = *dir;
		}
		else
		{
			resolved.logger.bEnableFile = false;
			(void)std::fprintf(stderr,
				"[NetEngine] DH1_LOG_DIR is not set or is empty; file logging disabled (console only).\n");
		}
	}

	NetEngineLogger::Init(resolved.logger);
}

NetEngineInit::~NetEngineInit()
{
	SocketUtils::Clear();
}
