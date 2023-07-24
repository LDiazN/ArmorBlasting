// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Components/SceneCaptureComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "BlastableComponent.generated.h"

class USceneCaptureComponent2D;
class UCanvasRenderTarget2D;
class UCanvas;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARMORBLASTING_API UBlastableComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBlastableComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**  
	* @param HitLocation Where the object was hit in world space
	* @param Radius Size of area of efect around `HitLocation`
	*/
	UFUNCTION(BlueprintCallable, Category = "ArmorBlasting")
	void UnwrapToRenderTarget(FVector HitLocation = FVector::ZeroVector, float Radius = 0);

	/// <summary>
	/// Try to blast this object's surface at the specified location
	/// </summary>
	/// <param name="Location">Location in world space where this object was hit</param>
	void Blast(FVector Location, float ImpactRadius);

	/** Get render target used to store damage for this blastable */
	UFUNCTION(BlueprintCallable)
	UTextureRenderTarget2D* GetDamageRenderTarget() const { return DamageRenderTarget; } // TODO: Devolver esto a DamageRenderTarget

	/** Get Render target used to store temporal damage for this blastable */
	UFUNCTION(BlueprintCallable)
	UTextureRenderTarget2D* GetTimeDamageRenderTarget() const { return Cast<UTextureRenderTarget2D>(TimeDamageRenderTarget); } // TODO: Devolver esto a DamageRenderTarget

protected:
	/// <summary>
	/// Checks if this component is properly configured
	/// </summary>
	void CheckComponentConsistency() const;

protected:

	/// <summary>
	/// This function will keep all Scene Capture default settings in one place.
	/// </summary>
	void SetUpSceneRender2D();

	/// <summary>
	/// Set up material parameters and create dynamic material instances for the unwrapping material
	/// </summary>
	/// <param name="Material">Base material for unwrapping</param>
	void SetUnwrapMaterial(UMaterial* Material);

	/// <summary>
	/// Set up material parameters and create dynamic material instances for the fading damage material
	/// </summary>
	/// <param name="Material"> Base material for color fading over time </param>
	void SetFadingMaterial(UMaterial* Material);

	/// <summary>
	/// Helper function to collect meshes that are intended to be blastable.
	/// </summary>
	/// <returns>An array of meshes child of the current owner that are tagged with 'BlastableMesh' </returns>
	TArray<UStaticMeshComponent*> GetBlastableMeshSet() const;

	/** Update fading damage every few ms to implement the slow fading effect */
	UFUNCTION()
	void UpdateFadingDamageRenderTarget();

	/// <summary>
	/// Access the skeletal mesh component from the owner, assume the owner is a character with a skeletal mesh
	/// </summary>
	/// <returns> Pointer to the owner's skeletal mesh component, or null if the owner doesn't provide a skeletal mesh </returns>
	USkeletalMeshComponent* GetMeshComponent() const;

	/** Scene capture component used to capture a snapshot of the unwrapped character */
	UPROPERTY(EditAnywhere, Category = "Components")
	USceneCaptureComponent2D* SceneCapture;

	/** Render target where the unwrapped texture will be drawn */
	UTextureRenderTarget2D* DamageRenderTarget;

	/** Render target where the damage over time will be drawn */
	UTextureRenderTarget2D* TimeDamageRenderTarget;

	/** Material used to unwrap the texture, an instance will be created in runtime */
	UPROPERTY(EditAnywhere, Category = "Resources")
	UMaterial* UnwrapMaterial;

	/** Material instance used to compute unwraping */
	UPROPERTY()
	UMaterialInstanceDynamic* UnwrapMaterialInstance;

	/** Material used to fade damange over time, an instance will be created in runtime */
	UPROPERTY(EditAnywhere, Category = "Resources")
	UMaterial* FadingMaterial;

	/** Material instance used for fading damage */
	UPROPERTY()
	UMaterialInstanceDynamic* UnwrapFadingMaterialInstance;

	/** How much time every damage mark takes to dissapear */
	UPROPERTY(EditAnywhere, Category = "VFX")
	float TimeToVanishDamage = 2.f;

	/** Meshes marked as Blastable. These are obtained using the GetBlastableMeshSet, 
		and cached to prevent overhead of multiple object searches
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Component")
	TArray<UStaticMeshComponent*> BlastableMeshes;

	/// <summary>
	/// Used to repeat fading damage material 
	/// </summary>
	FTimerHandle DamageFadingTimerHandle;
};
