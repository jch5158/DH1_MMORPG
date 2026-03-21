#pragma once

#include "CoreMinimal.h"
#include "Network/CppNetEngine/NetEngineWrapper.h"
#include "ClientNetSubsystem.generated.h"

UCLASS()
class DH1_CLIENT_API UClientNetSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
    // ---------------------------------------------------
    // 1. 서브시스템 생명주기 (UGameInstanceSubsystem 오버라이드)
    // ---------------------------------------------------

    // 서브시스템이 생성될 때 1회 호출. (소켓 초기화, 스레드 생성, 서버 연결)
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // 서브시스템이 파괴될 때 1회 호출. (스레드 종료, 소켓 닫기, 자원 해제)
    virtual void Deinitialize() override;

    // ---------------------------------------------------
    // 2. 틱 프레임워크 (FTickableGameObject 오버라이드)
    // ---------------------------------------------------

    // 매 프레임 호출. (수신 큐에서 패킷을 꺼내와 메인 스레드에서 디스패치)
    virtual void Tick(float DeltaTime) override;

    // (필수 구현) 언리얼 프로파일러에서 이 객체의 틱 비용을 추적하기 위한 ID 반환
    virtual TStatId GetStatId() const override;

    // 틱 활성화 여부. true를 반환해야 Tick()이 호출됨
    virtual bool IsTickable() const override;

    // ---------------------------------------------------
    // 3. 커스텀 네트워크 API (외부 클래스에서 호출할 함수들)
    // ---------------------------------------------------

    // 서버 연결 요청
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool ConnectToServer(const FString& IPAddress, int32 Port);

    // 서버 연결 종료
    UFUNCTION(BlueprintCallable, Category = "Network")
    void Disconnect();

    // 패킷 전송 (직렬화된 바이트 배열을 엔진 계층으로 전달)
    void SendPacket(const uint8* PacketData, int32 Size);

private:
    // 내부적으로 연결 상태를 관리할 변수
    bool bIsConnected = false;
    NetEngineInit EnginInit;
    ClientServiceRef ServiceRef;
};
