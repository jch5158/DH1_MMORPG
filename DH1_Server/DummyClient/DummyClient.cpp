#include "pch.h"

#include "GameSession.h"
#include "Service.h"
#include "ThreadManager.h"

#include "Generated/PacketServiceTypeHandler.h"

int32 main()
{
	CrashReporter::Init(L"DummyClient", L"1.0.0", L"");

	NetEngineInit netEngineInit;

	PacketServiceTypeHandler::Init();

	ClientServiceRef pService = cpp_net_engine::MakeShared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		cpp_net_engine::MakeShared<IocpCore>(),
		cpp_net_engine::MakeShared<ActorScheduler>([](const uint32 errorCode)->void
			{
				NET_ENGINE_LOG_ERROR("ActorScheduler Error, errorCode : {}", errorCode);
			}),
		cpp_net_engine::MakeShared<GameSession>,
		cpp_net_engine::MakeShared<SessionManager>(5000)
	);

	if (pService->Start() == false)
	{
		NET_ENGINE_LOG_INFO("DummyClient start Failed");
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

	return 0;
}
