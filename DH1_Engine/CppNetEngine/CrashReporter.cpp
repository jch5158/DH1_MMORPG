#include "pch.h"

#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include "CrashReporter.h"

namespace fs = std::filesystem;

void CrashReporter::Crash()
{
	volatile uint32* pCrash = nullptr;

#pragma warning(suppress: 6011)
	* pCrash = 0xDEAFBEFF;
}

void CrashReporter::CrashIf(const bool bCrash)
{
	if (bCrash)
	{
		Crash();
	}
}

fs::path CrashReporter::GetExeDirectory()
{
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    if (GetModuleFileNameW(nullptr, buffer, MAX_PATH) > 0)
    {
        const fs::path exePath(buffer);
        return exePath.parent_path();
    }
#endif

    return fs::current_path();
}

bool CrashReporter::Init(const Wstring& appName, const Wstring& appVersion, const Wstring& url)
{
    static bool sbInitialized = false;
    if (sbInitialized)
    {
        return true;
    }

    const fs::path currentDir = GetExeDirectory();
    const fs::path handlerPath = currentDir / LR"(crashpad_handler.exe)";
    const fs::path dbPath = currentDir / L"crashes";
    const fs::path metricsPath = currentDir / L"metrics";

    if (!fs::exists(handlerPath))
    {
        fmt::print(L"[Error] crashpad_handler not found at: {}\n", handlerPath.wstring());
        return false;
    }

    // 2. 메타데이터 변환 (wstring -> string UTF-8)
    std::map<std::string, std::string> annotations;
    annotations["format"] = "minidump";
    annotations["prod"] = toStdU8String(appName);
    annotations["ver"] = toStdU8String(appVersion);

    // 3. 인자 설정
    std::vector<std::string> arguments;
    arguments.emplace_back("--no-rate-limit");

    static crashpad::CrashpadClient client;

    // 5. 핸들러 시작 (Windows에서는 StartHandler가 wstring 경로를 받음)
    sbInitialized = client.StartHandler(
        base::FilePath(handlerPath.wstring()), // 경로: wstring
        base::FilePath(dbPath.wstring()),      // 경로: wstring
        base::FilePath(metricsPath.wstring()), // 경로: wstring
        toStdU8String(url),                    // URL: string (UTF-8)
        annotations,                           // 메타데이터: map<string, string>
        arguments,
        true,
        false
    );

    if (sbInitialized)
    {
        fmt::print(L"[Crashpad] Initialized. Dump Path: {}\n", dbPath.wstring());
    }
    else
    {
        fmt::print(L"[Crashpad] Initialization Failed.\n");
    }

    return sbInitialized;
}

std::string CrashReporter::toStdU8String(const Wstring& wStr)
{
	const std::u8string u8Str = fs::path(wStr).u8string();

    std::string str(reinterpret_cast<const char*>(u8Str.c_str()));

	return str;
}
