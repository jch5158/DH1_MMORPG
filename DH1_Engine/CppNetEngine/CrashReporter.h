#pragma once

class CrashReporter final
{
public:
	CrashReporter() = delete;
	~CrashReporter() = delete;
	CrashReporter(const CrashReporter&) = delete;
	CrashReporter& operator=(const CrashReporter&) = delete;
	CrashReporter(CrashReporter&&) = delete;
	CrashReporter& operator=(CrashReporter&&) = delete;

	static void Crash();
	static void CrashIf(const bool bCrash);
	static std::filesystem::path GetExeDirectoryPath();
	static bool Initialize(const std::string& appName, const std::string& appVersion, const std::string& url);
};
