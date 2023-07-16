// Fill out your copyright notice in the Description page of Project Settings.

#include "BlastableComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "ArmorBlasting.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "GameFramework/Character.h"

// Sets default values for this component's properties
UBlastableComponent::UBlastableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Set up components
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetUpSceneRender2D();
}


// Called when the game starts
void UBlastableComponent::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("CALLING BEGIN PLAY FROM %s"), *(GetOwner()->GetName()));


	// Set up render targets:
	DamageRenderTarget = NewObject<UTextureRenderTarget2D>();
	DamageRenderTarget->Rename(TEXT("DamageRenderTarget"));
	DamageRenderTarget->ResizeTarget(1024, 1024);
	DamageRenderTarget->ClearColor = FColor::Black;

	// Set up TimeDamageRenderTarget. Since we're using it to fade the damage using canvas, then we create it as 
	// a CanvasRenderTarget
	TimeDamageRenderTarget = NewObject<UTextureRenderTarget2D>();
	TimeDamageRenderTarget->Rename(TEXT("TimeDamageRenderTarget"));
	TimeDamageRenderTarget->ResizeTarget(1024, 1024);
	TimeDamageRenderTarget->ClearColor = FColor::Black;

	TimeDamageRenderTargetBackup = NewObject<UTextureRenderTarget2D>();
	TimeDamageRenderTargetBackup->Rename(TEXT("TimeDamageBackupRenderTarget"));
	TimeDamageRenderTargetBackup->ResizeTarget(1024, 1024);
	TimeDamageRenderTargetBackup->ClearColor = FColor::Black;

	// Set up dynamic materials
	SetUnwrapMaterial(UnwrapMaterial);
	SetFadingMaterial(FadingMaterial);

	CheckComponentConsistency();
	// Find Component tagged with "BlastableMesh"
	auto const Owner = GetOwner();

	// Set up Blastable mesh
	if (Owner != nullptr)
	{
		BlastableMeshes = GetBlastableMeshSet();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not set up blastable mesh"));
	}

	// Set up material arguments for all possible sub materials
	for (auto const Mesh : BlastableMeshes)
	{
		if (Mesh == nullptr)
			continue;

		Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Enemy, ECollisionResponse::ECR_Block);

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
			DynamicMaterial->SetTextureParameterValue(FName("RT_FadingDamage"), TimeDamageRenderTarget);

			if (DynamicMaterial != nullptr)
				Mesh->SetMaterial(i, DynamicMaterial);
		}
	}

	// Start timer to update fading
	auto World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(DamageFadingTimerHandle, this, &UBlastableComponent::UpdateFadingDamageRenderTarget, 0.1, true, 0);
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
	if (BlastableMeshes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Error trying to unwrap to render target: Blastable Mesh not properly configured"));
		return;
	}

	if (!IsValid(UnwrapMaterialInstance) || !UnwrapMaterialInstance->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Error, TEXT("Error trying to unwrap to render target: UnwrapMaterialInstance not properly set"));
		return;
	}

	auto const Mesh = GetMeshComponent();
	if (Mesh != nullptr)
		Mesh->SetVisibility(false);

	// Store old material to restore it afterwards
	TArray<TArray<UMaterialInterface*>> OldMaterialsPerMesh;
	FRotator OldRotation = SceneCapture->GetComponentRotation();
	FVector OldPosition = SceneCapture->GetComponentLocation();

	SceneCapture->SetWorldRotation(FRotator(-90, -90, 0));
	SceneCapture->SetWorldLocation(GetOwner()->GetActorLocation() + FVector{0,0,512});

	for (auto const MeshComponent : BlastableMeshes)
	{
		// Sanity check
		if (MeshComponent == nullptr)
			continue;

		// Original materials
		auto const& Materials = MeshComponent->GetMaterials();
		TArray<UMaterialInterface*> OldMaterials(Materials);
		OldMaterialsPerMesh.Add(OldMaterials);
		for (size_t i = 0; i < Materials.Num(); i++)
		{
			MeshComponent->SetMaterial(i, UnwrapMaterialInstance);

			// Set the right material for every submaterial
			UnwrapMaterialInstance->SetScalarParameterValue(TEXT("DamageRadius"), Radius);
			UnwrapMaterialInstance->SetVectorParameterValue(TEXT("HitLocation"), HitLocation);
		}
	}

		// Capture Scene with just the unwrapped material and hit locations

		// Capture scene in the right render target
		SceneCapture->TextureTarget = DamageRenderTarget;
		SceneCapture->CaptureScene();
	
		// Now repeat for the secondary render target
		SceneCapture->TextureTarget = TimeDamageRenderTarget;
		SceneCapture->CaptureScene();

		// Restore old materials
		for (int i = 0; i < BlastableMeshes.Num(); i++)
			for (int j = 0; j < OldMaterialsPerMesh[i].Num(); j++)
				BlastableMeshes[i]->SetMaterial(j, OldMaterialsPerMesh[i][j]);

	if (Mesh != nullptr)
		Mesh->SetVisibility(true);

	SceneCapture->SetWorldRotation(OldRotation);
	SceneCapture->SetWorldLocation(OldPosition);
}

void UBlastableComponent::Blast(FVector Location, float ImpactRadius)
{
	UnwrapToRenderTarget(Location, ImpactRadius);
	bBlastJustReceived = true;
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
	SceneCapture->CompositeMode = SCCM_Additive;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->ShowOnlyActors.Add(SceneCapture->GetOwner());
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCapture->SetRelativeLocation({ 0,0,512 });
	SceneCapture->SetRelativeRotation(FRotator{ -90,-90,0 });
	SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCapture->OrthoWidth = 1024;
	SceneCapture->ShowFlags.Atmosphere = 0;
	SceneCapture->ShowFlags.AmbientCubemap = 0;
	SceneCapture->ShowFlags.Lighting = 0;
	SceneCapture->ShowFlags.PostProcessing = 0;
}

void UBlastableComponent::SetUnwrapMaterial(UMaterial* Material)
{
	if (IsValid(Material) && IsValid(this))
	{
		UnwrapMaterialInstance = UMaterialInstanceDynamic::Create(Material, this, TEXT("UnwrapMaterialInstace"));
		if (UnwrapMaterialInstance == nullptr || !IsValid(UnwrapMaterialInstance))
		{
			UE_LOG(LogTemp, Error, TEXT("Could not set up material instance for unwrap material"));
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Could not set up material instance for unwrap material"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Could not set up UnwrapMaterial since the specified material is not valid"));
}

void UBlastableComponent::SetFadingMaterial(UMaterial* Material)
{
	if (IsValid(Material) && IsValid(this))
	{
		UnwrapFadingMaterialInstance = UMaterialInstanceDynamic::Create(Material, this, TEXT("FadingMaterialInstace"));
		UnwrapFadingMaterialInstance->SetTextureParameterValue(FName("RT_FadingTexture"), TimeDamageRenderTargetBackup);
		if (UnwrapFadingMaterialInstance == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not set up material instance for fading material"));
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Could not set up material instance for fading material"));
	}
}

TArray<UStaticMeshComponent *> UBlastableComponent::GetBlastableMeshSet() const
{
	AActor* Owner = GetOwner();
	if (Owner != nullptr)
	{
		
		auto ActorComponents = GetOwner()->GetComponentsByTag(UStaticMeshComponent::StaticClass(), FName("BlastableMesh"));
		TArray<UStaticMeshComponent*> Meshes;
		Meshes.Init(nullptr, ActorComponents.Num());
		for (int i = 0; i < ActorComponents.Num(); i++)
			Meshes[i] = (Cast<UStaticMeshComponent>(ActorComponents[i]));

		return Meshes;
	}

	UE_LOG(LogTemp, Warning, TEXT("Could not retrieve Mesh Components. Owner is NULL."));
	return TArray<UStaticMeshComponent*>();
}

void UBlastableComponent::UpdateFadingDamageRenderTarget()
{
	FVector2D Size;
	if (bBlastJustReceived)
		bBlastJustReceived = false;

	// Store what we draw 
	UCanvas* Canvas;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, TimeDamageRenderTargetBackup, Canvas, Size, Context);
	{
		// No est� guardando la textura vieja en la nueva, arreglar TODO
		Canvas->K2_DrawTexture(TimeDamageRenderTarget, FVector2D::ZeroVector, Size, FVector2D::ZeroVector);
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
	Canvas = nullptr;

	// Begin a Draw Canvas To Render Target to render the material that fades the render target
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, TimeDamageRenderTarget, Canvas, Size, Context);
	{
		// Canvas->K2_DrawMaterial(UnwrapFadingMaterialInstance, FVector2D::ZeroVector, Size, FVector2D::ZeroVector);
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);

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

