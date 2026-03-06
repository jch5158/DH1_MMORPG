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

	const ServerServiceRef pService = cpp_net_engine::MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		cpp_net_engine::MakeShared<Listener>(10,
			[](const uint32 errorCode)->void
			{
				NET_ENGINE_LOG_ERROR("Listener Error Handle, errorCode : {}", errorCode);
			}),
		cpp_net_engine::MakeShared<IocpCore>(),
		cpp_net_engine::MakeShared<ActorScheduler>([](const uint32 errorCode)->void
			{
				NET_ENGINE_LOG_ERROR("ActorScheduler Error, errorCode : {}", errorCode);
			}),
			cpp_net_engine::MakeShared<GameSession>,
			cpp_net_engine::MakeShared<SessionManager>(1),
			cpp_net_engine::MakeShared<SessionReaper>(10000),
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

	ThreadManager::GetInstance().JoinWithClear();
}

