#pragma once

#include "CoreMinimal.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "Widgets/SCompoundWidget.h"

class SRealmSelectWidget : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SRealmSelectWidget) {}
		SLATE_ARGUMENT(TArray<FRealmServerInfo>, RealmList)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	void OnRealmClicked(int32 RealmId);

	TArray<FRealmServerInfo> RealmList;
};
