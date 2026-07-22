// The Connected Sprawl - Reliable on-foot melee input routing.

#include "Characters/SprawlMeleeInput.h"

#include "Characters/ZarriCharacter.h"
#include "Components/InputComponent.h"

const TArray<FKey>& FSprawlMeleeInput::GetOnFootKeys()
{
	static const TArray<FKey> Keys = {
		EKeys::X,
		EKeys::Gamepad_FaceButton_Left
	};
	return Keys;
}

void FSprawlMeleeInput::BindOnFoot(UInputComponent* InputComponent,
	AZarriCharacter* Character)
{
	if (!InputComponent || !Character)
	{
		return;
	}

	for (const FKey& Key : GetOnFootKeys())
	{
		InputComponent->BindKey(
			Key, IE_Pressed, Character, &AZarriCharacter::OnMeleeKey);
	}
}

bool FSprawlMeleeInput::IsOnFootKey(const FKey& Key)
{
	return GetOnFootKeys().Contains(Key);
}
