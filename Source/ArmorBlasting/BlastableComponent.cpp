// Fill out your copyright notice in the Description page of Project Settings.

#include "BlastableComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Character.h"

// Sets default values for this component's properties
UBlastableComponent::UBlastableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Set up components
	SceneCaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent2D"));
	SceneCaptureComponent2D->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	SetUnwrapMaterial(UnwrapMaterial);
	SetFadingMaterial(FadingMaterial);

	SetUpSceneRender2D();
}


// Called when the game starts
void UBlastableComponent::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("CALLING BEGIN PLAY FROM %s"), *(GetOwner()->GetName()));

	// Set up render targets:
	DamageRenderTarget = NewObject<UTextureRenderTarget2D>();
	DamageRenderTarget->ResizeTarget(1024, 1024);
	TimeDamageRenderTarget = NewObject<UTextureRenderTarget2D>();
	TimeDamageRenderTarget->ResizeTarget(1024, 1024);
	DamageRenderTarget->ClearColor = FColor::Black;
	TimeDamageRenderTarget->ClearColor = FColor::Black;

	CheckComponentConsistency();
	// Find Component tagged with "BlastableMesh"
	auto const Owner = GetOwner();

	// Set up Blastable mesh
	if (Owner != nullptr)
	{
		auto const& Components = GetOwner()->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), FName("BlastableMesh"));

		// Check if everything went fine
		if (Components.Num() < 1)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not find any skeletal mesh component tagged with 'BlastableMesh', and therefore I could not set up UBlastableComponent"));
		}
		else
		{
			if (Components.Num() > 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("More than one mesh tagged with 'BlastableMesh', defaulting to the first one"));
			}

			BlastableMesh = Cast<USkeletalMeshComponent>(Components[0]);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not set up blastable mesh"));
	}

	// Set up material arguments for all possible sub materials
	auto Mesh = GetBlastableMesh();
	if (Mesh != nullptr)
	{
		auto const Materials = Mesh->GetMaterials();
		for (size_t i = 0; i < Materials.Num(); i++)
		{
			auto& Material = Materials[i];

			// Check if this is a dynamic material instance:
			if (Cast<UMaterialInstanceDynamic>(Material) != nullptr)
				continue;

			UE_LOG(LogTemp, Warning, TEXT("Creating dynamic material instances for blastable mesh"));
			auto DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
			// Set the texture where this material instance will sample for damage
			DynamicMaterial->SetTextureParameterValue(FName("RT_UnwrapDamage"), DamageRenderTarget);
			if (DynamicMaterial != nullptr)
				Mesh->SetMaterial(i, DynamicMaterial);
		}
	}
}


// Called every frame
void UBlastableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UBlastableComponent::UnwrapToRenderTarget(FVector HitLocation, float Radius)
{
	if (!IsValid(GetBlastableMesh()))
	{
		UE_LOG(LogTemp, Error, TEXT("Error trying to unwrap to render target: Blastable Mesh not properly configured"));
		return;
	}

	if (!IsValid(UnwrapMaterialInstance) || !UnwrapMaterialInstance->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Error, TEXT("Error trying to unwrap to render target: UnwrapMaterialInstance not properly set"));
		return;
	}

	// Store old material to restore it afterwards
	USkeletalMeshComponent* MeshComponent = GetBlastableMesh();
	if (MeshComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't unwrap to render target since couldn't retrieve Actor's skeletal mesh."));
		return;
	}

	// Original materials
	auto const& Materials = MeshComponent->GetMaterials();

	for (size_t i = 0; i < Materials.Num(); i++)
	{
		MeshComponent->SetMaterial(i, UnwrapMaterialInstance);

		// Set the right material for every submaterial
		UnwrapMaterialInstance->SetScalarParameterValue(TEXT("DamageRadius"), Radius);
		UnwrapMaterialInstance->SetVectorParameterValue(TEXT("HitLocation"), HitLocation);
	}

	// Capture Scene with just the unwrapped material and hit locations
	auto const Mesh = GetMeshComponent();
	if (Mesh != nullptr)
		Mesh->SetVisibility(false);

	// Capture scene in the right render target
	SceneCaptureComponent2D->TextureTarget = DamageRenderTarget;
	SceneCaptureComponent2D->CaptureScene();
	
	// Now repeat for the secondary render target
	SceneCaptureComponent2D->TextureTarget = TimeDamageRenderTarget;
	SceneCaptureComponent2D->CaptureScene();

	if (Mesh != nullptr)
		Mesh->SetVisibility(true);

	// Restore old materials
	for (size_t i = 0; i < Materials.Num(); i++)
	{
		auto const OldMaterial = Materials[i];
		if (IsValid(OldMaterial))
			MeshComponent->SetMaterial(i, OldMaterial);
	}
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
	SceneCaptureComponent2D->CompositeMode = SCCM_Additive;
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
	if (IsValid(Material) && IsValid(this))
	{
		UnwrapMaterialInstance = UMaterialInstanceDynamic::Create(Material, this, TEXT("UnwrapMaterialInstace"));
		if (UnwrapMaterialInstance == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not set up material instance for unwrap material"));
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Could not set up material instance for unwrap material"));
	}
}

void UBlastableComponent::SetFadingMaterial(UMaterial* Material)
{
	if (IsValid(Material) && IsValid(this))
	{
		UnwrapFadingMaterialInstance = UMaterialInstanceDynamic::Create(Material, this, TEXT("FadingMaterialInstace"));
		UnwrapFadingMaterialInstance->SetTextureParameterValue(FName("RT_FadingDamage"), TimeDamageRenderTarget);
		if (UnwrapFadingMaterialInstance == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not set up material instance for fading material"));
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Could not set up material instance for fading material"));
	}
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

