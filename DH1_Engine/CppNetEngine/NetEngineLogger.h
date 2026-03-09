#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

class NetEngineLogger
{
public:
	NetEngineLogger() = delete;
	~NetEngineLogger() = delete;

	static void Init();
	static std::shared_ptr<spdlog::logger> GetLogger() { return mpLogger; }

private:

	static std::shared_ptr<spdlog::logger> mpLogger;
};

#define NET_ENGINE_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_INFO(...) SPDLOG_LOGGER_INFO(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_WARN(...) SPDLOG_LOGGER_WARN(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(NetEngineLogger::GetLogger(), __VA_ARGS__)
#define NET_ENGINE_LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(NetEngineLogger::GetLogger(), __VA_ARGS__)