#pragma once

#include "CoreMinimal.h"

namespace AuthLocalPreferences
{
	/** Load remember-email toggle and saved email from GGameUserSettingsIni [DH1Auth]. */
	void LoadLoginRemember(bool& bOutRemember, FString& OutSavedEmail);

	/** Persist or clear saved email. Call only after successful login. */
	void SaveLoginRemember(bool bRemember, const FString& NormalizedEmail);
}
