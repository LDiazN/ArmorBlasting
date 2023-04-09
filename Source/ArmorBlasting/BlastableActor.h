// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlastableActor.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterial;
class UMaterialInstanceDynamic;

UCLASS()
class ARMORBLASTING_API ABlastableActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABlastableActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(EditAnywhere, Category = "Components")
	USceneCaptureComponent2D* SceneCaptureComponent2D;

	UPROPERTY(EditAnywhere, Category = "Resources")
	UTextureRenderTarget2D* DamageRenderTarget;

	UPROPERTY(EditAnywhere, Category = "Resources")
	UMaterial* UnwrapMaterial;

	// This is the actual instance whose parameters we modify in runtime
	UPROPERTY()
	UMaterialInstanceDynamic* UnwrapMaterialInstance;

protected:
	
	void SetUpSceneRender2D();

	void SetUnwrapMaterial(UMaterial* Material);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "ArmorBlasting")
	void UnwrapToRenderTarget(FVector HitLocation = FVector::ZeroVector, float Radius = 0);

	/// <summary>
	/// Try to blast this object's surface at the specified location
	/// </summary>
	/// <param name="Location">Location in world space where this object was hit</param>
	void Blast(FVector Location, float ImpactRadius);

};
