#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Network/CppNetEngine/NetEngineWrapper.h"
#include "Network/CppNetEngine/NetSession.h"

#include "NetEngineInit.h"

#include "ClientNetSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGatewayLoginResultDelegate, int32 /*eLoginResult*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHttpLoginErrorDelegate, int32 /*HttpStatusCode*/, const FString& /*Message*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEmailVerificationRequiredDelegate, const FString& /*Message*/, const FString& /*Email*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRealmListReceivedDelegate, const TArray<FRealmServerInfo>& /*RealmList*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRealmSelectResultDelegate, int32 /*eRealmSelectResult*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSpawnPositionReceivedDelegate, const FVector& /*Position*/, float /*Yaw*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnMovePathReceivedDelegate, uint32 /*SeqId*/, const TArray<FVector>& /*Waypoints*/, float /*MoveSpeed*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPositionCorrectionDelegate, const FVector& /*CorrectedPosition*/);

USTRUCT(BlueprintType)
struct FRealmServerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 RealmId = 0;

	UPROPERTY(BlueprintReadOnly)
	FString RealmName;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Status = 0;
};

UCLASS(Config = Game)
class DH1_CLIENT_API UClientNetSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
    // ---------------------------------------------------
    // 1. 서브시스템 생명주기 (UGameInstanceSubsystem 오버라이드)
    // ---------------------------------------------------
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---------------------------------------------------
    // 2. 틱 프레임워크 (FTickableGameObject 오버라이드)
    // ---------------------------------------------------
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    // ---------------------------------------------------
    // 3. 커스텀 네트워크 API
    // ---------------------------------------------------

    // HTTP 로그인 요청 (LoginServer로 전송)
    UFUNCTION(BlueprintCallable, Category = "Network")
    void RequestLogin(const FString& Email, const FString& Password);

    // 게이트웨이 서버 TCP 연결
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool ConnectToServer(const FString& IPAddress, int32 Port);

    // 서버 연결 종료
    UFUNCTION(BlueprintCallable, Category = "Network")
    void Disconnect();

    // 패킷 전송
    void SendPacket(const uint8* PacketData, int32 Size);

    void SetAuthData(const FString& ArgTicket, const FString& ArgAccountId);
    AuthData GetAuthData() const;
    void NotifyLoginResult(int32 Result);
    void NotifyRealmList(const TArray<FRealmServerInfo>& RealmList);
    void NotifyRealmSelectResult(int32 Result);
    void NotifySpawnPosition(const FVector& Position, float Yaw);
    void NotifyMovePath(uint32 SeqId, const TArray<FVector>& Waypoints, float MoveSpeed);
    void NotifyPositionCorrection(const FVector& CorrectedPosition);

    void RequestRealmList();
    void RequestRealmSelect(int32 RealmId);

    // 이동 패킷 전송
    void SendMoveInput(const FVector& Direction, float RotationYaw, float PositionZ);
    void SendMoveStop(float CurrentYaw);

    // 스폰 위치 요청
    void RequestSpawnPosition();

    // 클릭 이동 (목적지 → 서버 경로 계산)
    void SendMoveToPosition(const FVector& Destination);

    // 델리게이트
    FOnGatewayLoginResultDelegate OnGatewayLoginResult;
    FOnHttpLoginErrorDelegate OnHttpLoginError;
    FOnEmailVerificationRequiredDelegate OnEmailVerificationRequired;
    FOnRealmListReceivedDelegate OnRealmListReceived;
    FOnRealmSelectResultDelegate OnRealmSelectResult;
    FOnSpawnPositionReceivedDelegate OnSpawnPositionReceived;
    FOnMovePathReceivedDelegate OnMovePathReceived;
    FOnPositionCorrectionDelegate OnPositionCorrection;

private:
    void OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    uint32 MoveSequenceId = 0;
    bool bIsConnected = false;
    TUniquePtr<NetEngineInit> EngineInit;
    ClientServiceRef ServiceRef;
    AuthData ClientAuthData;

    UPROPERTY(Config)
    FString LoginServerHost;

    UPROPERTY(Config)
    int32 LoginServerPort = 5001;

    UPROPERTY(Config)
    bool bUseHttps = true;
};
