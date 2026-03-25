#pragma once

#include "CoreMinimal.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "Widgets/SCompoundWidget.h"

class SWorldSelectWidget : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SWorldSelectWidget) {}
		SLATE_ARGUMENT(TArray<FWorldServerInfo>, WorldList)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	void OnWorldClicked(int32 WorldId);

	TArray<FWorldServerInfo> WorldList;
};
