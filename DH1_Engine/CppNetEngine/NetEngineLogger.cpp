#include "pch.h"
#include "NetEngineLogger.h"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h> // [핵심] 데일리 싱크 헤더로 변경

void NetEngineLogger::Init()
{
	spdlog::init_thread_pool(8192, 1);

	Vector<spdlog::sink_ptr> sinks;

	const auto fileSink = cpp_net_engine::MakeShared<spdlog::sinks::daily_file_sink_mt>("logs/NetEngine.log", 0, 0);
	sinks.emplace_back(fileSink);

#ifdef _DEBUG
	const auto consoleSink = cpp_net_engine::MakeShared<spdlog::sinks::stdout_color_sink_mt>();
	sinks.emplace_back(consoleSink);
#endif

	mpLogger = cpp_net_engine::MakeShared<spdlog::async_logger>(
		"NetEngine",
		sinks.begin(),
		sinks.end(),
		spdlog::thread_pool(),
		spdlog::async_overflow_policy::overrun_oldest // 서버 멈춤 방지
	);

#ifdef _DEBUG
	mpLogger->set_level(spdlog::level::trace);
#else
	mpLogger->set_level(spdlog::level::info);
#endif

	mpLogger->flush_on(spdlog::level::err);

	mpLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%t] [%s:%#] [%^%l%$] %v");

	spdlog::register_logger(mpLogger);
	spdlog::set_default_logger(mpLogger);

	spdlog::flush_every(std::chrono::seconds(3));
}

std::shared_ptr<spdlog::logger> NetEngineLogger::mpLogger = nullptr;