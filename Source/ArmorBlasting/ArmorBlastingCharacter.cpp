// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArmorBlastingCharacter.h"
#include "ArmorBlastingProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "BlastableActor.h"
#include "DrawDebugHelpers.h"
#include "BlastableComponent.h"
#include "ArmorBlasting.h"
#include "NiagaraFunctionLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId


DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AArmorBlastingCharacter

AArmorBlastingCharacter::AArmorBlastingCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);
	 
	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;
}

void AArmorBlastingCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeSinceLastShot += DeltaSeconds;
}

void AArmorBlastingCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AArmorBlastingCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AArmorBlastingCharacter::OnFire);
	PlayerInputComponent->BindAxis("FireHold", this, &AArmorBlastingCharacter::OnFireHold);
	PlayerInputComponent->BindAxis("SwapWeapon", this, &AArmorBlastingCharacter::SwapGun);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AArmorBlastingCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AArmorBlastingCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AArmorBlastingCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AArmorBlastingCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AArmorBlastingCharacter::LookUpAtRate);
}

void AArmorBlastingCharacter::SetFireMode(ShootModes NewMode)
{
	CurrentShootingMode = NewMode;
}

int AArmorBlastingCharacter::GetFireRate() const
{
	switch (CurrentShootingMode)
	{
	case AArmorBlastingCharacter::ShootModes::Semiauto:
		return SemiAutoFireRate;
	case AArmorBlastingCharacter::ShootModes::Shotgun:
		return ShotgunFireRate;
	case AArmorBlastingCharacter::ShootModes::Auto:
		return AutoFireRate;
	case AArmorBlastingCharacter::ShootModes::N_MODES:
	default:
		break;
	}

	return 0;
}

void AArmorBlastingCharacter::OnFire()
{
		
	// This might not be the best way to implement multiple 
	// multiple weapon types but it will do as a demo for precodural armor damage

	// Do nothing if can't shoot
	if (!CanShoot())
		return;

	// Choose wich type of shoot to employ
	switch (CurrentShootingMode)
	{
		case ShootModes::Semiauto:
			ShootSemiAuto();
			break;
		case ShootModes::Shotgun:
			ShootShotgun();
			break;
		case ShootModes::Auto:
			ShootAuto();
			break;
		default:
			UE_LOG(LogTemp, Warning, TEXT("Unsupported type of shot"));
			return;
			break;
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

	TimeSinceLastShot = 0;
}

void AArmorBlastingCharacter::OnFireHold(float Val)
{
	// Do nothing if button is not held down
	if (FMath::IsNearlyZero(Val))
		return;

	// Only auto fire can use button-holding input
	if (CurrentShootingMode != ShootModes::Auto)
		return;
	OnFire();
}

void AArmorBlastingCharacter::ShootSemiAuto()
{
	// Sanity Check
	UWorld* const World = GetWorld();
	if (World == NULL) return;

	const FRotator SpawnRotation = GetControlRotation();
	// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
	// const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);
	const FVector SpawnLocation = GetFirstPersonCameraComponent()->GetComponentLocation();

	auto CameraComponent = GetFirstPersonCameraComponent();
	auto CameraForward = CameraComponent->GetForwardVector();
	CameraForward.Normalize();
	FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;

	// -- DEBUG ONLY -----------------------
	// Use this if you want to see the trace for shots
	// if (GetWorld() != nullptr)
	// {
	// 	const FName TraceTag = TEXT("ShotTrace");
	// 	QueryParams.TraceTag = TraceTag;
	// 	GetWorld()->DebugDrawTraceTag = TraceTag;
	// }
	// -------------------------------------

	// Try to create a linetrace shot
	FHitResult HitResult;
	bool bHitSomething = GetWorld()->LineTraceSingleByChannel(HitResult, SpawnLocation, SpawnLocation + 100000 * CameraForward, ECC_Enemy, QueryParams);

	// We have to check if what we hit provides a BlastableComponent
	if (bHitSomething)
	{
		auto Actor = HitResult.Actor;
		auto BlastableComponent = Actor->FindComponentByClass<UBlastableComponent>();

		// if doesn't provide skeletal mesh, nothing to do
		if (BlastableComponent != nullptr)
		{
			BlastableComponent->Blast(HitResult.Location, 5);
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactSparks, HitResult.Location, HitResult.ImpactNormal.Rotation(), 0.001 * FVector::OneVector);
		}
	}
}

void AArmorBlastingCharacter::ShootAuto()
{
	// Fow now shoot auto is just shoot semi auto but more often
	ShootSemiAuto();
}

void AArmorBlastingCharacter::ShootShotgun()
{
	// try and fire a projectile
	if (ProjectileClass == NULL) return;

	UWorld* const World = GetWorld();
	if (World == NULL) return;

	const FRotator SpawnRotation = GetControlRotation();
	// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
	// const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);
	const FVector SpawnLocation = GetFirstPersonCameraComponent()->GetComponentLocation();
	// TODO finish Shoot code
	auto CameraComponent = GetFirstPersonCameraComponent();
	// auto CameraForward = CameraComponent->GetComponentRotation().Vector
	
	auto CameraForward = CameraComponent->GetForwardVector();
	auto CameraUp = CameraComponent->GetUpVector();
	auto CameraRight = CameraComponent->GetRightVector();

	// To Shoot the shotgun you have to compute many rays around the center of the shotgun reticle. They all
	// have the same origin but might have different end points. The endpoints will be randombly distributed 
	// around a circle at the end of the main ray

	// Config parameters
	const float MaxSpreadRadius = 50;
	const float MaxRange = 1000;
	const FVector CentralEndpoint = SpawnLocation + CameraForward * MaxRange;
	const int NShots = 15;
	const float MaxShotImpactRadius = 6;
	const float MinShotImpactRadius = 2;

	for (int i = 0; i < NShots; i++)
	{
		// Compute radius and rotation for this endpoint
		const float Radius = FMath::FRand() * MaxSpreadRadius;
		const float Rotation = FMath::FRand() * 2.f * PI;


		// Endpoint is where to point to
		const FVector Endpoint =	CentralEndpoint + 
									CameraRight * Radius * FMath::Cos(Rotation) + 
									CameraUp    * Radius * FMath::Sin(Rotation);

		// Impact Radius is how big the holes are in the impact point
		const float ImpactRadius = FMath::Lerp(MinShotImpactRadius, MaxShotImpactRadius, Radius / MaxSpreadRadius);

		// Ray Cast!
		FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;

		// -- DEBUG ONLY -----------------------
		// if (GetWorld() != nullptr)
		// {
		// 	const FName TraceTag = TEXT("ShotTrace");
		// 	QueryParams.TraceTag = TraceTag;
		// 	GetWorld()->DebugDrawTraceTag = TraceTag;
		// }
		// -------------------------------------

		// Try to create a linetrace shot
		FHitResult HitResult;
		bool bHitSomething = GetWorld()->LineTraceSingleByChannel(
											HitResult, 
											SpawnLocation, 
											Endpoint, 
											ECC_Enemy, 
											QueryParams
							);
		// We have to check if what we hit provides a BlastableComponent
		if (bHitSomething)
		{
			auto Actor = HitResult.Actor;
			auto BlastableComponent = Actor->FindComponentByClass<UBlastableComponent>();

			// if doesn't provide skeletal mesh, nothing to do
			if (BlastableComponent != nullptr)
			{
				BlastableComponent->Blast(HitResult.Location, ImpactRadius);
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactSparks, HitResult.Location, HitResult.ImpactNormal.Rotation(), 0.001 * FVector::OneVector);
			}


		}
	}


}

bool AArmorBlastingCharacter::CanShoot() const
{
	// Check whether we can shot depending on our current shooting style
	int FireRate = GetFireRate();
	float TimeToWait = 1 / static_cast<float>(FireRate);

	// Don't shoot if cooldown not already set
	return TimeSinceLastShot > TimeToWait;
}

void AArmorBlastingCharacter::SwapGun(float Val)
{
	// If no input, do nothing
	if (FMath::IsNearlyZero(Val))
		return;

	auto AmountModes = static_cast<int>(ShootModes::N_MODES);
	auto CurrentModeInt = static_cast<int>(CurrentShootingMode);
	auto Direction = Val > 0 ? 1 : -1;

	// Note that mode applies the same for negative numbers, so if we go down from 0, we need to go back to 
	// the highest value
	auto NextMode = Direction == -1 && CurrentModeInt == 0 ? AmountModes - 1 : (CurrentModeInt + Direction) % AmountModes;

	SetFireMode(static_cast<ShootModes>(NextMode));
	
	FString Gun;
	switch (CurrentShootingMode)
	{
	case AArmorBlastingCharacter::ShootModes::Semiauto:
		Gun = "Semiauto";
		break;
	case AArmorBlastingCharacter::ShootModes::Shotgun:
		Gun = "Shotgun";
		break;
	case AArmorBlastingCharacter::ShootModes::Auto:
		Gun = "Auto";
		break;
	case AArmorBlastingCharacter::ShootModes::N_MODES:
	default:
		break;
	}

	UE_LOG(LogTemp, Warning, TEXT("Using Gun: %s\n"), *Gun);
}

void AArmorBlastingCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AArmorBlastingCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AArmorBlastingCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void AArmorBlastingCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void AArmorBlastingCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AArmorBlastingCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AArmorBlastingCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AArmorBlastingCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AArmorBlastingCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AArmorBlastingCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AArmorBlastingCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AArmorBlastingCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}
