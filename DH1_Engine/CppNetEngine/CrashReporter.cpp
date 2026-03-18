#include "pch.h"

#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include "CrashReporter.h"

namespace fs = std::filesystem;

void CrashReporter::Crash()
{
	__debugbreak();
}

void CrashReporter::CrashIf(const bool bCrash)
{
	if (bCrash)
	{
		Crash();
	}
}

fs::path CrashReporter::GetExeDirectoryPath()
{
    wchar_t buffer[MAX_PATH];
    if (GetModuleFileNameW(nullptr, buffer, MAX_PATH) > 0)
    {
        const fs::path exePath(buffer);
        return exePath.parent_path();
    }

    return fs::current_path();
}

bool CrashReporter::Initialize(const std::string& appName, const std::string& appVersion, const std::string& url)
{
    static bool sbInitialized = false;
    if (sbInitialized)
    {
        return true;
    }

    const fs::path currentDir = GetExeDirectoryPath();
    const fs::path handlerPath = currentDir / "crashpad_handler.exe";
    const fs::path dbPath = currentDir / "crashes";
    const fs::path metricsPath = currentDir / "metrics";

    if (!fs::exists(handlerPath))
    {
        fmt::print("[Error] crashpad_handler not found at: {}\n", handlerPath.string());
        return false;
    }

    std::map<std::string, std::string> annotations;
    annotations["format"] = "minidump";
    annotations["prod"] = appName;
    annotations["ver"] = appVersion;

    std::vector<std::string> arguments;
    arguments.emplace_back("--no-rate-limit");

    static crashpad::CrashpadClient client;

    const base::FilePath handlerFilePath(handlerPath.wstring());
    const base::FilePath dbFilePath(dbPath.wstring());
    const base::FilePath metricsFilePath(metricsPath.wstring());

    const bool initResult = client.StartHandler(
        handlerFilePath,
        dbFilePath,
        metricsFilePath,
        url,
        annotations,
        arguments,
        true,
        false
    );

    sbInitialized = initResult;
    if (sbInitialized)
    {
        fmt::print("[Crashpad] Initialized. Dump Path: {}\n", dbPath.string());
    }
    else
    {
        fmt::print("[Crashpad] Initialization Failed.\n");
    }

  
    return sbInitialized;
}