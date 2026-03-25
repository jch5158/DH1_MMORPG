#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

namespace AuthWidgetStyle
{
	// Primary action button (Login, SignUp, Confirm, Verify)
	inline void ApplyPrimaryButtonStyle(UButton* Button)
	{
		if (!Button) return;

		Button->SetBackgroundColor(FLinearColor(0.15f, 0.45f, 0.85f, 1.0f));
	}

	// Secondary action button (Back to Login, Send Code, Resend)
	inline void ApplySecondaryButtonStyle(UButton* Button)
	{
		if (!Button) return;

		Button->SetBackgroundColor(FLinearColor(0.25f, 0.25f, 0.28f, 1.0f));
	}

	// Status text default style
	inline void ApplyStatusTextStyle(UTextBlock* Text)
	{
		if (!Text) return;

		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f)));
	}
}
