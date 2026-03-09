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
	static std::filesystem::path GetExeDirectory();
	static bool Init(const Wstring& appName, const Wstring& appVersion, const Wstring& url);

private:
	
	static std::string toStdU8String(const Wstring& wStr);
};
