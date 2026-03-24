#include "pch.h"
#include "NetEngineLogger.h"

template<typename Mutex>
class UnrealCallbackSink : public spdlog::sinks::base_sink<Mutex>
{
protected:
	virtual void sink_it_(const spdlog::details::log_msg& msg) override
	{
		if (!NetEngineLogger::spLogCallback)
		{
			return;
		}

		spdlog::memory_buf_t formatted;
		this->formatter_->format(msg, formatted);

		formatted.push_back('\0');

		eNetLogLevel netLevel;
		switch (msg.level)
		{
		case spdlog::level::trace: netLevel = eNetLogLevel::Trace; break;
		case spdlog::level::debug: netLevel = eNetLogLevel::Debug; break;
		case spdlog::level::info:  netLevel = eNetLogLevel::Info;  break;
		case spdlog::level::warn:  netLevel = eNetLogLevel::Warn;  break;
		case spdlog::level::err:   netLevel = eNetLogLevel::Error; break;
		case spdlog::level::critical: netLevel = eNetLogLevel::Fatal; break;
		case spdlog::level::off:
		case spdlog::level::n_levels:
		default:  // NOLINT(clang-diagnostic-covered-switch-default)
			CrashReporter::Crash();
			return;
		}

		NetEngineLogger::spLogCallback(netLevel, formatted.data());
	}

	virtual void flush_() override {}
};

void NetEngineLogger::SetLogCallback(LogCallback callback)
{
	spLogCallback = std::move(callback);
}

void NetEngineLogger::Init(const NetEngineLoggerConfig& config)
{
	if (spLogger)
	{
		return;
	}

	spdlog::init_thread_pool(config.asyncQueueSize, config.asyncThreadCount);

	constexpr auto logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%t] [%s:%#] [%^%l%$] %v";

	Vector<spdlog::sink_ptr> sinks;

	if (config.bIsUnrealClient)
	{
		const auto unrealSink = cpp_net_engine::MakeShared<UnrealCallbackSink<std::mutex>>();
		unrealSink->set_pattern("%v");
		sinks.push_back(unrealSink);
	}
	else
	{
		if (config.bEnableFile)
		{
			const std::string logPath = config.logDirectory + "/" + config.logFileName;
			const auto fileSink = cpp_net_engine::MakeShared<spdlog::sinks::daily_file_sink_mt>(logPath, 0, 0);
			fileSink->set_pattern(logPattern);
			sinks.push_back(fileSink);
		}

		if (config.bEnableConsole)
		{
			const auto consoleSink = cpp_net_engine::MakeShared<spdlog::sinks::stdout_color_sink_mt>();
			consoleSink->set_pattern(logPattern);
			sinks.push_back(consoleSink);
		}
	}

	spLogger = cpp_net_engine::MakeShared<spdlog::async_logger>(
		"NetEngine",
		sinks.begin(),
		sinks.end(),
		spdlog::thread_pool(),
		spdlog::async_overflow_policy::overrun_oldest
	);

#ifdef _DEBUG
	spLogger->set_level(spdlog::level::trace);
#else
	spLogger->set_level(spdlog::level::info);
#endif

	spLogger->flush_on(spdlog::level::err);

	spdlog::register_logger(spLogger);
	spdlog::set_default_logger(spLogger);

	spdlog::flush_every(std::chrono::seconds(config.flushIntervalSeconds));
}

std::shared_ptr<spdlog::logger> NetEngineLogger::GetLogger()
{
	return spLogger;
}
