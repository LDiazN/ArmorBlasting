// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "BlastableComponent.generated.h"

class USceneCaptureComponent2D;

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

	UFUNCTION(BlueprintCallable, Category = "ArmorBlasting")
	void UnwrapToRenderTarget(FVector HitLocation = FVector::ZeroVector, float Radius = 0);

	/// <summary>
	/// Try to blast this object's surface at the specified location
	/// </summary>
	/// <param name="Location">Location in world space where this object was hit</param>
	void Blast(FVector Location, float ImpactRadius);

protected:
	/// <summary>
	/// Checks if this component is properly configured
	/// </summary>
	void CheckComponentConsistency() const;
		
protected:

	/** Scene capture component used to capture a snapshot of the unwrapped character */
	UPROPERTY(EditAnywhere, Category = "Components")
	USceneCaptureComponent2D* SceneCaptureComponent2D;

	/** Render target where the unwrapped texture will be drawn */
	UPROPERTY(EditAnywhere, Category = "Resources")
	UTextureRenderTarget2D* DamageRenderTarget;

	/** Material used to unwrap the texture, an instance will be created in runtime */
	UPROPERTY(EditAnywhere, Category = "Resources")
	UMaterial* UnwrapMaterial;

	/** Material instance used to compute unwraping */
	UPROPERTY()
	UMaterialInstanceDynamic* UnwrapMaterialInstance;

	void SetUpSceneRender2D();

	void SetUnwrapMaterial(UMaterial* Material);

	/// <summary>
	/// Access the skeletal mesh component from the owner, assume the owner is a character with a skeletal mesh
	/// </summary>
	/// <returns> Pointer to the owner's skeletal mesh component, or null if the owner doesn't provide a skeletal mesh </returns>
	USkeletalMeshComponent* GetMeshComponent() const;
};
