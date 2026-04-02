#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SButton;
class SEditableTextBox;
class STextBlock;

DECLARE_DELEGATE_OneParam(FOnCreateCharacterRequested, const FString& /*CharacterName*/);

class SCharacterCreatePanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SCharacterCreatePanel) {}
		SLATE_EVENT(FOnCreateCharacterRequested, OnCreateRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetStatusMessage(const FString& Message, bool bIsError = true);
	void SetCreateEnabled(bool bEnabled);
	void ClearInput();

private:

	FReply HandleCreateClicked();
	void HandleNameCommitted(const FText& Text, ETextCommit::Type CommitType);

	FOnCreateCharacterRequested OnCreateRequested;

	TSharedPtr<SEditableTextBox> NameInputBox;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SButton> CreateButton;
};
