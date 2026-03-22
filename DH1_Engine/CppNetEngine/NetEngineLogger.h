#pragma once

enum class eNetLogLevel { Trace, Debug, Info, Warn, Error, Fatal };

class NetEngineLogger
{
public:
	template<typename Mutex> 
	friend class UnrealCallbackSink;

	using LogCallback = std::function<void(eNetLogLevel, const char*)>;

	NetEngineLogger() = delete;
	~NetEngineLogger() = delete;

	static void SetLogCallback(LogCallback callback);
	static void Init(const bool bIsUnrealClient = false);
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