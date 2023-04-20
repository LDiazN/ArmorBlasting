// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Character.h"
#include "BlastableComponent.h"

// Sets default values for this component's properties
UBlastableComponent::UBlastableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Set up components
	SceneCaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent2D"));
	SceneCaptureComponent2D->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	// Create resources
	DamageRenderTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("DamageRenderTarget"));
	SetUnwrapMaterial(CreateDefaultSubobject<UMaterial>(TEXT("UnwrapMaterial")));

	SetUpSceneRender2D();
}


// Called when the game starts
void UBlastableComponent::BeginPlay()
{
	Super::BeginPlay();
	CheckComponentConsistency();
	// ...
	
}


// Called every frame
void UBlastableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UBlastableComponent::UnwrapToRenderTarget(FVector HitLocation, float Radius)
{
	if (!IsValid(UnwrapMaterialInstance))
		SetUnwrapMaterial(UnwrapMaterial);

	if (!IsValid(UnwrapMaterialInstance) || !UnwrapMaterialInstance->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Error, TEXT("Could not instance material"));
		return;
	}

	// Store old material to restore it afterwards
	USkeletalMeshComponent* MeshComponent = GetMeshComponent();
	if (MeshComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't unwrap to render target since couldn't retrieve Actor's skeletal mesh."));
		return;
	}

	auto OldMaterial = MeshComponent->GetMaterial(0);
	MeshComponent->SetMaterial(0, UnwrapMaterialInstance);

	UnwrapMaterialInstance->SetScalarParameterValue(TEXT("DamageRadius"), Radius);
	UnwrapMaterialInstance->SetVectorParameterValue(TEXT("HitLocation"), HitLocation);

	// Capture Scene with just the unwrapped material and hit locations
	SceneCaptureComponent2D->TextureTarget = DamageRenderTarget;
	SceneCaptureComponent2D->CaptureScene();

	if (IsValid(OldMaterial))
		MeshComponent->SetMaterial(0, OldMaterial);
}

void UBlastableComponent::Blast(FVector Location, float ImpactRadius)
{
	UnwrapToRenderTarget(Location, ImpactRadius);
}

void UBlastableComponent::CheckComponentConsistency() const
{
	auto Owner = GetOwner();
	if (Owner == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Owner of BlastableComponent improperly configured"));
		return;
	}

	auto CharacterOwner = Cast<ACharacter>(Owner);
	if (CharacterOwner == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Owner of BlastableComponent is not an instance of ACharacter"));
	}
}

void UBlastableComponent::SetUpSceneRender2D()
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

void UBlastableComponent::SetUnwrapMaterial(UMaterial* Material)
{
	UnwrapMaterial = Material;
	if (IsValid(UnwrapMaterial) && IsValid(this))
		UnwrapMaterialInstance = UMaterialInstanceDynamic::Create(UnwrapMaterial, this, TEXT("UnwrapMaterialInstace"));
}

USkeletalMeshComponent* UBlastableComponent::GetMeshComponent() const
{
	AActor* Owner = GetOwner();
	if (Owner != nullptr)
	{
		USkeletalMeshComponent* SkeletalMesh = Owner->FindComponentByClass <USkeletalMeshComponent>();
		return SkeletalMesh;
	}
	return nullptr;
}

