#include "Network/Subsystem/ClientNetSubsystem.h"

#include "Network/PacketHandler/PacketServiceTypeHandler.h"


void UClientNetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	NetEngineInit NetEngineInit;

	PacketServiceTypeHandler::Init();

	NetworkSchedulerConfig SchedulerConfig;
	SchedulerConfig.waitTimeoutMs = 16;
	SchedulerConfig.tickIntervalMs = 16;
	SchedulerConfig.runningThreadCount = 0;
	SchedulerConfig.onHandleError = [](const uint32)->void {};

	//ClientServiceConfig Config;
	//Config.netAddress = NetAddress("127.0.0.1", 7000);
	//Config.maxSessionCount = 1;
	//Config.sessionFactory = []()->NetSessionRef
	//	{
	//		return cpp_net_engine::MakeShared<NetSession>(8192, 4096);
	//	};
	//Config.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(SchedulerConfig);
	//
	//ClientService = cpp_net_engine::MakeShared<ClientService>(Config);
}

void UClientNetSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UClientNetSubsystem::Tick(float DeltaTime)
{
}

TStatId UClientNetSubsystem::GetStatId() const
{
	return TStatId();
}

bool UClientNetSubsystem::IsTickable() const
{
	return FTickableGameObject::IsTickable();
}

bool UClientNetSubsystem::ConnectToServer(const FString& IPAddress, int32 Port)
{
	if (!ClientService)
	{
		return false;
	}

	// 주의: Initialize에서 Config.netAddress에 127.0.0.1, 7000을 하드코딩했음.
	// 인자로 받은 IPAddress와 Port를 사용하려면 여기서 연결 대상 주소를 덮어씌우거나 연결 함수 인자로 넘겨야 함.

	// FString을 std::string 또는 엔진 호환 타입으로 변환 (예: UTF8)
	// std::string IPStr = TCHAR_TO_UTF8(*IPAddress);

	// 엔진의 연결 함수 호출 (함수명은 실제 API에 맞게 수정)
	//return ClientService->Connect(IPStr, Port);
	return true;
}

void UClientNetSubsystem::Disconnect()
{
}

void UClientNetSubsystem::SendPacket(const uint8* PacketData, int32 Size)
{
}
