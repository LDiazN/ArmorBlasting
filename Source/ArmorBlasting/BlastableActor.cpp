// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "BlastableActor.h"

// Sets default values
ABlastableActor::ABlastableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set up components
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	SceneCaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent2D"));

	SetRootComponent(StaticMeshComponent);
	SceneCaptureComponent2D->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	// Create resources
	DamageRenderTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("DamageRenderTarget"));
	SetUnwrapMaterial(CreateDefaultSubobject<UMaterial>(TEXT("UnwrapMaterial")));

	SetUpSceneRender2D();
}

// Called when the game starts or when spawned
void ABlastableActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABlastableActor::SetUpSceneRender2D()
{
	SceneCaptureComponent2D->bCaptureEveryFrame = false;
	SceneCaptureComponent2D->bCaptureOnMovement = false;
	SceneCaptureComponent2D->ShowOnlyActors.Add(SceneCaptureComponent2D->GetOwner());
	SceneCaptureComponent2D->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCaptureComponent2D->SetRelativeLocation({ 0,0,512 });
	SceneCaptureComponent2D->SetRelativeRotation(FRotator{ -90,-90,0 });
	SceneCaptureComponent2D->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCaptureComponent2D->OrthoWidth = 1024;
}

void ABlastableActor::SetUnwrapMaterial(UMaterial* Material)
{
	UnwrapMaterial = Material;
	if(IsValid(UnwrapMaterial) && IsValid(this))
		UnwrapMaterialInstance = UMaterialInstanceDynamic::Create(UnwrapMaterial, this, TEXT("UnwrapMaterialInstace"));
}

// Called every frame
void ABlastableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABlastableActor::UnwrapToRenderTarget(FVector HitLocation, float Radius)
{
	if (!IsValid(UnwrapMaterialInstance))
		SetUnwrapMaterial(UnwrapMaterial);

	if (!IsValid(UnwrapMaterialInstance) || !UnwrapMaterialInstance->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Error, TEXT("Could not instance material"));
		return;
	}

	// Store old material to restore it afterwards
	auto OldMaterial = StaticMeshComponent->GetMaterial(0);
	StaticMeshComponent->SetMaterial(0, UnwrapMaterialInstance);

	UnwrapMaterialInstance->SetScalarParameterValue(TEXT("DamageRadius"), Radius);
	UnwrapMaterialInstance->SetVectorParameterValue(TEXT("HitLocation"), HitLocation);

	// Capture Scene with just the unwrapped material and hit locations
	SceneCaptureComponent2D->TextureTarget = DamageRenderTarget;
	SceneCaptureComponent2D->CaptureScene();

	if (IsValid(OldMaterial))
		StaticMeshComponent->SetMaterial(0, OldMaterial);
}

void ABlastableActor::Blast(FVector Location, float ImpactRadius)
{
	UnwrapToRenderTarget(Location, ImpactRadius);
}

