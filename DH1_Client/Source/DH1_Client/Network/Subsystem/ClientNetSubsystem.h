#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Network/CppNetEngine/NetEngineWrapper.h"
#include "Network/CppNetEngine/NetSession.h"
#include "Templates/Function.h"

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
DECLARE_MULTICAST_DELEGATE_FourParams(FOnCharacterOverheadDataDelegate, const FString& /*DisplayName*/, int32 /*Level*/, float /*CurrentHP*/, float /*MaxHP*/);
DECLARE_MULTICAST_DELEGATE(FOnCharacterCreateRequiredDelegate);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCharacterCreateResultDelegate, int32 /*eCreateCharacterResult*/, const FString& /*Message*/);

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

class UUserWidget;
class UButton;
class UComboBoxString;
class UEditableTextBox;
class UScrollBox;
class UVerticalBox;
class UWidget;

/** 서버 S2C_ENTITY_ENTER / 스냅샷용 — 월드에 스폰할 원격 엔터티 한 줄 */
struct FNetworkEntitySpawnData
{
	uint64 EntityId = 0;
	FVector Position = FVector::ZeroVector;
	float YawDegrees = 0.f;
	FString DisplayName;
	int32 Level = 0;
	float CurrentHP = 0.f;
	float MaxHP = 0.f;
};

struct FNetworkEntityMoveState
{
	FVector TargetPosition = FVector::ZeroVector;
	float TargetYaw = 0.f;
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

    UFUNCTION(BlueprintCallable, Category = "Network")
    FString GetLoginServerApiBaseUrl() const;

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

    /** 서버(패킷 핸들러)에서 캐릭터 오버헤드 갱신 시 호출 — 클라 UI만 갱신 */
    void NotifyCharacterOverheadData(const FString& DisplayName, int32 Level, float CurrentHP, float MaxHP);

    /**
     * S2C_SPAWN_POSITION_RES 한 번 처리 시 사용. 게임 스레드에서 오버헤드 → 스폰 순으로 동기 브로드캐스트
     * (별도 AsyncTask 두 번이면 델리게이트 순서가 뒤바뀌어 기본값/깨진 값이 잠깐 또는 계속 보일 수 있음).
     */
    void NotifySpawnPositionWithCharacterSheet(const FVector& Position, float Yaw, const FString& DisplayName, int32 Level, float CurrentHP, float MaxHP);

    void RequestRealmList();
    void RequestRealmSelect(int32 RealmId);

    void NotifyCharacterCreateRequired();
    void NotifyCharacterCreateResult(int32 Result, const FString& Message);
    void SendCreateCharacterRequest(const FString& CharacterName);

    /** 렐름 진입 직후·스폰 요청 시 월드 로딩 오버레이 (뷰포트 전역, GameMode와 무관) */
    void ShowEnterWorldLoadingOverlay();
    void HideEnterWorldLoadingOverlay();

    /**
     * PIE/Game 플레이 월드의 NetSubsystem만 순회 (에디터 등 다른 WorldContext가 먼저 오면 break로 게임 쪽을 건너뛰던 문제 방지).
     * 동일 GameInstance는 한 번만 호출.
     */
    static void ForEachPlayClientNetSubsystem(TFunction<void(UClientNetSubsystem*)> Fn);

    // 스폰 위치 요청
    void RequestSpawnPosition();

    /**
     * S2C_SPAWN_POSITION_RES 직후 OnSpawnPositionReceived에서 한 번 호출.
     * 스폰 패킷에 실린 캐릭터 시트를 적용한다. (델리게이트만 쓰면 IsLocallyControlled 타이밍에 이름이 안 바뀌는 경우가 있음)
     */
    bool ConsumePendingSpawnCharacterSheet(FString& OutDisplayName, int32& OutLevel, float& OutCurrentHP, float& OutMaxHP);

    // 클릭 이동 (목적지 → 서버 경로 계산, 현재 위치도 함께 전송)
    void SendMoveToPosition(const FVector& CurrentPosition, const FVector& Destination);

    /** Flop MCP로 만든 WBP_Chat(UserWidget) 등 — 이름으로 자식 위젯을 찾아 송수신 바인딩 */
    void RegisterChatUi(UUserWidget* ChatRoot);
    void UnregisterChatUi();

    /** @return true if a non-empty message was queued on the session */
    bool SendChatRequest(int32 ChannelEnumValue, const FString& Message);

    void FocusChatInput();
    void CycleChatChannel();

    void NotifyChatMessageReceived(int32 ChannelEnumValue, uint64 SenderAccountId, const FString& SenderDisplayName,
        const FString& Message, int64 ServerTimestampMs);

    /** S2C_ENTITY_ENTER_NOT / SNAPSHOT — 주변 엔터티 메시(큐브 프록시) 스폰·위치 갱신 */
    void ApplyNetworkEntitiesEntered(const TArray<FNetworkEntitySpawnData>& Entities);
    void ApplyNetworkEntitiesLeft(const TArray<uint64>& EntityIds);
    void ClearNetworkSpawnedEntities();

    void SendJumpNotify();
    void ApplyRemoteJump(uint64 EntityId);

    // 델리게이트
    FOnGatewayLoginResultDelegate OnGatewayLoginResult;
    FOnHttpLoginErrorDelegate OnHttpLoginError;
    FOnEmailVerificationRequiredDelegate OnEmailVerificationRequired;
    FOnRealmListReceivedDelegate OnRealmListReceived;
    FOnRealmSelectResultDelegate OnRealmSelectResult;
    FOnSpawnPositionReceivedDelegate OnSpawnPositionReceived;
    FOnMovePathReceivedDelegate OnMovePathReceived;
    FOnPositionCorrectionDelegate OnPositionCorrection;
    FOnCharacterOverheadDataDelegate OnCharacterOverheadData;
    FOnCharacterCreateRequiredDelegate OnCharacterCreateRequired;
    FOnCharacterCreateResultDelegate OnCharacterCreateResult;

    bool IsCharacterCreatePending() const { return bCharacterCreatePending; }
    void ClearCharacterCreatePending() { bCharacterCreatePending = false; }

private:
    void StartHeartbeatTimer();
    void StopHeartbeatTimer();
    void OnHeartbeatTick();
    void MarkSessionLocalDisconnect();

    FTimerHandle HeartbeatTimerHandle;
    static constexpr float HeartbeatIntervalSec = 5.0f;

    bool bCharacterCreatePending = false;
    void OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    UFUNCTION()
    void HandleChatSendClicked();

    UFUNCTION()
    void HandleChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    /** 채널 콤보 드롭다운을 앵커 위쪽으로 열기 (콘텐츠 브라우저 가림 방지) */
    UFUNCTION()
    void HandleChatChannelComboOpening();

    UFUNCTION()
    UWidget* HandleChatComboGenerateItem(FString Item);

    /** GetWorld()가 비어 있을 때(틱/패킷 타이밍) PIE·Game 월드 해상도 */
    UWorld* ResolveGameWorldForSpawning() const;

    void TrySendChatFromInput();
    void AppendChatLine(int32 ChannelEnumValue, uint64 SenderAccountId, const FString& SenderDisplayName,
        const FString& Message, int64 ServerTimestampMs);

    /** HTTP 로그인 후 설정된 AccountId 문자열 → uint64 (실패·빈 값이면 0) */
    uint64 GetParsedLocalAccountId() const;

    TWeakObjectPtr<UUserWidget> ChatRootWidget;
    TWeakObjectPtr<UButton> ChatSendButton;
    TWeakObjectPtr<UComboBoxString> ChatChannelCombo;
    TWeakObjectPtr<UEditableTextBox> ChatMessageEdit;
    TWeakObjectPtr<UScrollBox> ChatScrollLog;
    TWeakObjectPtr<UVerticalBox> ChatLogLinesBox;

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

    bool bHasPendingSpawnCharacterSheet = false;
    FString PendingSpawnDisplayName;
    int32 PendingSpawnLevel = 1;
    float PendingSpawnCurrentHP = 100.f;
    float PendingSpawnMaxHP = 100.f;

    /** 로컬 채팅 에코용 (스폰 시트에서 설정) */
    FString CachedLocalChatDisplayName;

    /**
     * 게이트웨이 Chat Validate(IsInWorld)와 맞춤: 스폰 위치 응답 전에는 C2S_CHAT_REQ를 보내지 않음(B1).
     * 연결·렐름 재선택 시 false로 리셋.
     */
    bool bClientWorldChatAllowed = false;

    TMap<uint64, TWeakObjectPtr<AActor>> NetworkEntityActors;
	TMap<uint64, FNetworkEntityMoveState> NetworkEntityMoveStates;
	TMap<uint64, double> RecentlyLeftEntities;
};
