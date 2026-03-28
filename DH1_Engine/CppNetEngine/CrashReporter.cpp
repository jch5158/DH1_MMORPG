#include "pch.h"
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

    auto GetEnv = [](const char* name) -> std::string
    {
        char* pValue = nullptr;
        size_t size = 0;
        if (_dupenv_s(&pValue, &size, name) == 0 && pValue != nullptr)
        {
            std::string result(pValue);
            free(pValue);
            return result;
        }
        return {};
    };

    // DH1_CRASH_HANDLER_PATH 환경변수 → 없으면 exe 디렉토리 폴백
    const std::string handlerEnv = GetEnv("DH1_CRASH_HANDLER_PATH");
    const fs::path handlerPath = handlerEnv.empty()
        ? currentDir / "crashpad_handler.exe"
        : fs::path(handlerEnv);

    // DH1_CRASH_DIR 환경변수 → 없으면 exe 디렉토리/crashes 폴백
    const std::string crashDirEnv = GetEnv("DH1_CRASH_DIR");
    const fs::path dbPath = crashDirEnv.empty()
        ? currentDir / "crashes"
        : fs::path(crashDirEnv) / appName;

    const fs::path metricsPath = dbPath / "metrics";

    fs::create_directories(dbPath);
    fs::create_directories(metricsPath);

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