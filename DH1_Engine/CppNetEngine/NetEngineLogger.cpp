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
		const std::string logStr = fmt::to_string(formatted);

		auto netLevel = eNetLogLevel::Info;
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
			break;
		}

		NetEngineLogger::spLogCallback(netLevel, logStr.c_str());
	}

	virtual void flush_() override {}
};

void NetEngineLogger::SetLogCallback(LogCallback callback)
{
	spLogCallback = std::move(callback);
}

void NetEngineLogger::Init(const bool bIsUnrealClient)
{
	// [수정 완료] 언리얼/서버 무관하게 비동기 로거 스레드 풀 초기화 보장
	spdlog::init_thread_pool(8192, 1);

	Vector<spdlog::sink_ptr> sinks;

	if (bIsUnrealClient)
	{
		// [수정 완료] 빈 껍데기 포인터 생성으로 인한 널 포인터 역참조 크래시 해결.
		// 실제 메모리를 할당하여 스마트 포인터에 전달합니다.
		const auto unrealSink = cpp_net_engine::MakeShared<UnrealCallbackSink<std::mutex>>();

		unrealSink->set_pattern("%v");
		sinks.push_back(unrealSink);
	}
	else
	{
		const auto fileSink = cpp_net_engine::MakeShared<spdlog::sinks::daily_file_sink_mt>("logs/NetEngine.log", 0, 0);
		fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%t] [%s:%#] [%^%l%$] %v");
		sinks.emplace_back(fileSink);

#ifdef _DEBUG
		const auto consoleSink = cpp_net_engine::MakeShared<spdlog::sinks::stdout_color_sink_mt>();
		consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%t] [%s:%#] [%^%l%$] %v");
		sinks.emplace_back(consoleSink);
#endif	
	}

	spLogger = cpp_net_engine::MakeShared<spdlog::async_logger>(
		"NetEngine",
		sinks.begin(),
		sinks.end(),
		spdlog::thread_pool(),
		spdlog::async_overflow_policy::overrun_oldest // 서버 멈춤 방지
	);

#ifdef _DEBUG
	spLogger->set_level(spdlog::level::trace);
#else
	spLogger->set_level(spdlog::level::info);
#endif

	spLogger->flush_on(spdlog::level::err);

	spdlog::register_logger(spLogger);
	spdlog::set_default_logger(spLogger);

	spdlog::flush_every(std::chrono::seconds(3));
}

std::shared_ptr<spdlog::logger> NetEngineLogger::GetLogger()
{
	return spLogger;
}