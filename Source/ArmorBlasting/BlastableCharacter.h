// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlastableCharacter.generated.h"

class UBlastableComponent;

UCLASS()
class ARMORBLASTING_API ABlastableCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlastableCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	UBlastableComponent* GetBlastableComponent() const { return BlastableComponent; }

protected:

	UPROPERTY(EditAnywhere, Category = "Armor Blasting")
	UBlastableComponent* BlastableComponent;

	// UPROPERTY(EditDefaultsOnly, Category = "Armor Blasting")
	// USkeletalMeshComponent* Armor;
};
