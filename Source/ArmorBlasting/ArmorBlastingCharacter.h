// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArmorBlastingCharacter.generated.h"

class UInputComponent;
class UNiagaraSystem;

UCLASS(config=Game)
class AArmorBlastingCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* FP_MuzzleLocation;

	/** Gun mesh: VR view (attached to the VR controller directly, no arm, just the actual gun) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* VR_Gun;

	/** Location on VR gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* VR_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

	/** Motion controller (right hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent* R_MotionController;

	/** Motion controller (left hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent* L_MotionController;

public:
	enum class ShootModes {
		Semiauto,
		Shotgun,
		Auto,
		N_MODES
	};

	AArmorBlastingCharacter();
	
	virtual void Tick(float DeltaSeconds) override;

	/// <summary>
	/// Get currently active shooting mode
	/// </summary>
	/// <returns> Currently active shooting mode </returns>
	ShootModes GetShootMode() const { return CurrentShootingMode; }

	UFUNCTION(BlueprintPure)
	FString GetCurrentGunName() const;

protected:
	virtual void BeginPlay();

public:

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AArmorBlastingProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	class USoundBase* FireSound;

	/** Sound to play after the shotgun fire*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class USoundBase* ShotgunPumpSound;

	/** Sparks emitted at impact location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VFX)
	class UNiagaraSystem* ImpactSparks;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FireAnimation;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	uint32 bUsingMotionControllers : 1;

	/** Semi Auto fire rate: How many shots per second to shoot on semi auto mode. */
	UPROPERTY(EditAnywhere, Category = Combat)
	int SemiAutoFireRate = 4;

	/** Auto Fire Rate: How many shots per second to shoot on auto mode. */
	UPROPERTY(EditAnywhere, Category = Combat)
	int AutoFireRate = 10;

	/** Shotgun Fire Rate: How many shots per second to shoot on shotgun mode. */
	UPROPERTY(EditAnywhere, Category = Combat)
	int ShotgunFireRate = 2;


protected:
	
	/// <summary>
	/// Change the firing mode properly including all required state management
	/// </summary>
	/// <param name="NewMode"> New mode to set up </param>
	void SetFireMode(ShootModes NewMode);

	/// <summary>
	/// How many bullets per second to shoot according to the current shooting mode.
	/// </summary>
	/// <returns> Current fire rate </returns>
	int GetFireRate() const;

	/// <summary>
	/// Fires a projectile.
	/// </summary>
	void OnFire();

	/// <summary>
	/// Try to fire full if in full auto mode. This is bound to an axis intead of an input bc
	/// unreal seems unable to handle repeat events for mouse input. See: 
	/// https://forums.unrealengine.com/t/how-to-use-ie-repeat-einputevent-for-mouse-buttons/287312
	/// </summary>
	/// <param name="Val">Value of axis. </param>
	void OnFireHold(float Val);

	/// <summary>
	/// Shoot a single shot
	/// </summary>
	void ShootSemiAuto();

	/// <summary>
	/// Shoot a single sho
	/// </summary>
	void ShootAuto();
	
	/// <summary>
	/// Shoot a shotgun
	/// </summary>
	void ShootShotgun();

	/// <summary>
	/// Checks if you can shoot something. 
	/// </summary>
	/// <returns></returns>
	bool CanShoot() const;

	/// <summary>
	/// Swap weapon in the direction specified by `Val`
	/// </summary>
	/// <param name="Val"> Direction to swap weapons </param>
	void SwapGun(float Val);

	/** Resets HMD orientation and position in VR. */
	void OnResetVR();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	struct TouchData
	{
		TouchData() { bIsPressed = false;Location=FVector::ZeroVector;}
		bool bIsPressed;
		ETouchIndex::Type FingerIndex;
		FVector Location;
		bool bMoved;
	};
	void BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location);
	TouchData	TouchItem;

protected:

	/** Current Shooting Mode */
	ShootModes CurrentShootingMode = ShootModes::Semiauto;

	/** Used to know whether we can shoot or not according to our frame rate */
	float TimeSinceLastShot = 0;
	
	UPROPERTY(EditAnywhere, Category = UI)
	TSubclassOf<UUserWidget> GunWidgetClass;
	UUserWidget* GunWidget;

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/* 
	 * Configures input for touchscreen devices if there is a valid touch interface for doing so 
	 *
	 * @param	InputComponent	The input component pointer to bind controls to
	 * @returns true if touch controls were enabled.
	 */
	bool EnableTouchscreenMovement(UInputComponent* InputComponent);

public:
	/** Returns Mesh1P subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

