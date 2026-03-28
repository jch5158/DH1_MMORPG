#include "pch.h"
#include "NetEngineInit.h"

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

	std::string ResolveLogDirectory(const std::string& configDir)
	{
		char* pValue = nullptr;
		size_t size = 0;
		if (_dupenv_s(&pValue, &size, "DH1_LOG_DIR") == 0 && pValue != nullptr)
		{
			std::string result(pValue);
			free(pValue);
			return result + "/" + GetProcessStemName();
		}
		return configDir;
	}
}

NetEngineInit::NetEngineInit(const NetEngineConfig& config)
{
	SocketUtils::Init();

	NetEngineConfig resolved = config;
	resolved.logger.logDirectory = ResolveLogDirectory(config.logger.logDirectory);

	NetEngineLogger::Init(resolved.logger);
}

NetEngineInit::~NetEngineInit()
{
	SocketUtils::Clear();
}
