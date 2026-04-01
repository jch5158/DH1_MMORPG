#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverheadWidget.generated.h"

class STextBlock;
class SProgressBar;

/**
 * 인게임 캐릭터 머리 위 이름·레벨·체력 표시 (로그인 HUD와 별개).
 * 블루프린트 없이 Slate로 구성; WBP로 서브클래스화해 레이아웃만 바꿀 수 있음.
 */
UCLASS()
class DH1_CLIENT_API UCharacterOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Overhead")
	void SetDisplayName(const FString& InName);

	UFUNCTION(BlueprintCallable, Category = "Overhead")
	void SetLevel(int32 InLevel);

	UFUNCTION(BlueprintCallable, Category = "Overhead")
	void SetHealth(float Current, float Max);

	/** 이름·레벨·HP를 한 번에 반영 (서버 스냅샷용) */
	UFUNCTION(BlueprintCallable, Category = "Overhead")
	void SetOverheadData(const FString& InName, int32 InLevel, float CurrentHP, float MaxHP);

	UFUNCTION(BlueprintCallable, Category = "Overhead")
	void SetIsLocalPlayer(bool bLocal);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void RefreshOverheadSlate();
	float GetHealthPercent() const;

	FString CachedDisplayName = TEXT("Adventurer");
	int32 CachedLevel = 1;
	float CachedCurrentHP = 100.f;
	float CachedMaxHP = 100.f;
	bool bCachedIsLocal = false;

	TSharedPtr<STextBlock> SlateNameText;
	TSharedPtr<STextBlock> SlateLevelText;
	TSharedPtr<STextBlock> SlateHPText;
	TSharedPtr<SProgressBar> SlateHealthBar;
};
