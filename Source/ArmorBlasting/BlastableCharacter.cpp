// Fill out your copyright notice in the Description page of Project Settings.

#include "BlastableCharacter.h"
#include "BlastableComponent.h"

// Sets default values
ABlastableCharacter::ABlastableCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// BlastableComponent = CreateDefaultSubobject<UBlastableComponent>(TEXT("BlastableComponent"));
	// BlastableComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	Armor = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Armor"));
	Armor->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	// Use this tag to identify this mesh as the one to be blasted
	Armor->ComponentTags.Add("BlastableMesh");
	Armor->SetCollisionProfileName(FName("CharacterMesh"));

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

