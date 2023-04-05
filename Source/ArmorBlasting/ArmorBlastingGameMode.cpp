// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArmorBlastingGameMode.h"
#include "ArmorBlastingHUD.h"
#include "ArmorBlastingCharacter.h"
#include "UObject/ConstructorHelpers.h"

AArmorBlastingGameMode::AArmorBlastingGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AArmorBlastingHUD::StaticClass();
}
