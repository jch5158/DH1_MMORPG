#include "pch.h"
#include "EnginePch.h"
#include "NetAddress.h"
#include "ThreadManager.h"
#include "Service.h"
#include "GameSession.h"
#include "PacketServiceTypeHandler.h"
#include "RedisActor.h"

int main()
{
	CrashReporter::Initialize("GatewayServer", "1.0.0", "");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	const ServerServiceRef pService = cpp_net_engine::MakeShared<ServerService>(
		NetAddress("127.0.0.1", 9000),
		cpp_net_engine::MakeShared<GameSession>,
		cpp_net_engine::MakeShared<Listener>(10,
			[](const uint32 errorCode)->void
			{
			}),
			cpp_net_engine::MakeShared<NetworkScheduler>(16, [](const uint32 errorCode)->void
				{
				}),
				cpp_net_engine::MakeShared<SessionReaper>(15000),
				cpp_net_engine::MakeShared<SessionManager>(5000),
				cpp_net_engine::MakeShared<WaitQueueManager>(0));

	if (pService->Start() == false)
	{
		NET_ENGINE_LOG_INFO("GameServer start is failed\n");
		CrashReporter::Crash();
	}

	for (int32 i = 0; i < 5; ++i)
	{
		ThreadManager::GetInstance().Launch("Network Dispatch",[pService]()->void
			{
				while (true)
				{
					pService->GetIocpCore()->Dispatch();
				}
			});
	}

	ThreadManager::GetInstance().Launch("Monitor",[pService]()->void
		{
			while (true)
			{
				fmt::print("Current SessionCount : {}\n", pService->GetCurrentSessionCount());

				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			}
		});


	ThreadManager::GetInstance().JoinWithClear();
}
