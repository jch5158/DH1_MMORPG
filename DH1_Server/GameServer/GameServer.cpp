#include "pch.h"

#include "GameSession.h"
#include "Service.h"
#include "ThreadManager.h"

#include "Generated/PacketServiceTypeHandler.h"

int main()
{
	CrashReporter::Init(L"GameServer", L"1.0.0", L"");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	IocpCoreRef pIocp = cpp_net_engine::MakeShared<IocpCore>();
	ActorSchedulerRef pScheduler = cpp_net_engine::MakeShared<ActorScheduler>([](const uint32 errorCode)->void
		{
			NET_ENGINE_LOG_ERROR("ActorScheduler Error, errorCode : {}", errorCode);
		});



	const ServerServiceRef pService = cpp_net_engine::MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		cpp_net_engine::MakeShared<Listener>(10,
			[](const uint32 errorCode)->void
			{
				NET_ENGINE_LOG_ERROR("Listener Error Handle, errorCode : {}", errorCode);
			}),
		pIocp,
		pScheduler,
		cpp_net_engine::MakeShared<GameSession>,
		cpp_net_engine::MakeShared<SessionManager>(5000),
		cpp_net_engine::MakeShared<SessionReaper>(ActorContext(pScheduler), 60000),
		cpp_net_engine::MakeShared<WaitQueueManager>(0));

	if (pService->Start() == false)
	{
		NET_ENGINE_LOG_INFO("GameServer start is failed\n");
		CrashReporter::Crash();
	}

	for (int32 i = 0; i < 5; ++i)
	{
		ThreadManager::GetInstance().Launch([pService]()->void
			{
				while (true)
				{
					pService->GetIocpCore()->Dispatch();
				}
			});

		ThreadManager::GetInstance().Launch([pService]()->void
			{
				while (true)
				{
					pService->GetActorScheduler()->Dispatch();
				}
			});
	}

	ThreadManager::GetInstance().Launch([pService]()->void
		{
			while (true)
			{
				fmt::print("Current SessionCount : {}\n", pService->GetCurrentSessionCount());

				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			}
		});

	ThreadManager::GetInstance().JoinWithClear();
}

