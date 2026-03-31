#pragma once

enum class eNetLogLevel { Trace, Debug, Info, Warn, Error, Fatal };

struct NetEngineLoggerConfig
{
	bool bIsUnrealClient = false;
	bool bEnableConsole = true;
	bool bEnableFile = true;
	/** 파일 싱크 사용 시 NetEngineInit에서 DH1_LOG_DIR 기준으로 덮어씀 */
	std::string logDirectory;
	std::string logFileName = "NetEngine.log";
	uint32 asyncQueueSize = 8192;
	uint32 asyncThreadCount = 1;
	uint32 flushIntervalSeconds = 3;
};

class NetEngineLogger
{
public:
	template<typename Mutex>
	friend class UnrealCallbackSink;

	using LogCallback = std::function<void(eNetLogLevel, const char*)>;

	NetEngineLogger() = delete;
	~NetEngineLogger() = delete;

	static void SetLogCallback(LogCallback callback);
	static void Init(const NetEngineLoggerConfig& config = {});
	static std::shared_ptr<spdlog::logger> GetLogger();

private:
	inline static std::shared_ptr<spdlog::logger> spLogger = nullptr;
	inline static LogCallback spLogCallback = nullptr;
};

#define NET_ENGINE_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_INFO(...) SPDLOG_LOGGER_INFO(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_WARN(...) SPDLOG_LOGGER_WARN(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(NetEngineLogger::GetLogger(), __VA_ARGS__)
