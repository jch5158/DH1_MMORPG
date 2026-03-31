#include "AuthLocalPreferences.h"
#include "Misc/ConfigCacheIni.h"

namespace AuthLocalPreferences_Local
{
	static constexpr TCHAR Section[] = TEXT("DH1Auth");
	static constexpr TCHAR KeyRemember[] = TEXT("bRememberEmail");
	static constexpr TCHAR KeyEmail[] = TEXT("SavedEmail");
}

void AuthLocalPreferences::LoadLoginRemember(bool& bOutRemember, FString& OutSavedEmail)
{
	using namespace AuthLocalPreferences_Local;
	bOutRemember = false;
	OutSavedEmail.Empty();
	if (!GConfig)
	{
		return;
	}

	bool bRead = false;
	if (GConfig->GetBool(Section, KeyRemember, bRead, GGameUserSettingsIni))
	{
		bOutRemember = bRead;
	}

	FString EmailRead;
	if (GConfig->GetString(Section, KeyEmail, EmailRead, GGameUserSettingsIni))
	{
		OutSavedEmail = MoveTemp(EmailRead);
	}
}

void AuthLocalPreferences::SaveLoginRemember(const bool bRemember, const FString& NormalizedEmail)
{
	using namespace AuthLocalPreferences_Local;
	if (!GConfig)
	{
		return;
	}

	GConfig->SetBool(Section, KeyRemember, bRemember, GGameUserSettingsIni);
	GConfig->SetString(Section, KeyEmail, bRemember ? *NormalizedEmail : TEXT(""), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}
