// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArmorBlastingHUD.generated.h"

UCLASS()
class AArmorBlastingHUD : public AHUD
{
	GENERATED_BODY()

public:
	AArmorBlastingHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

