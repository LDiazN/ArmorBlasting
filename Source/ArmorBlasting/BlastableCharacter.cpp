// Fill out your copyright notice in the Description page of Project Settings.

#include "BlastableComponent.h"
#include "BlastableCharacter.h"

// Sets default values
ABlastableCharacter::ABlastableCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BlastableComponent = CreateDefaultSubobject<UBlastableComponent>(TEXT("BlastableComponent"));
	BlastableComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

}

// Called when the game starts or when spawned
void ABlastableCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABlastableCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABlastableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

