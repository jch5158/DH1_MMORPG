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

    // 델리게이트
    FOnGatewayLoginResultDelegate OnGatewayLoginResult;
    FOnHttpLoginErrorDelegate OnHttpLoginError;
    FOnEmailVerificationRequiredDelegate OnEmailVerificationRequired;

private:
    void OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

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
