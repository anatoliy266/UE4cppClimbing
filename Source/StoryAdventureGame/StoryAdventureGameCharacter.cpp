// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "StoryAdventureGameCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SphereComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"
#include <EngineGlobals.h>
#include <Runtime/Engine/Classes/Engine/Engine.h>
#include "NavigationSystem.h"

#include "string.h"

//////////////////////////////////////////////////////////////////////////
// AStoryAdventureGameCharacter

AStoryAdventureGameCharacter::AStoryAdventureGameCharacter()
{
	iterator = 0;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 400.f;
	GetCharacterMovement()->AirControl = 0.1f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	MaxCameraDistance = 600.f;
	MinCameraDistance = 100.f;

	//force run
	bIsForced = false;

	//playerParameters
	MaxStamina = 1000.f;
	Stamina = MaxStamina;
	MaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	ForceCoast = 50.0f;
	SpeedUpValue = 2.f;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	///climbing
	canTrace = false;
	isClimbing = false;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereTrace"));
	Collision->SetSphereRadius(96.f);
	Collision->SetCollisionProfileName("Trace");
	
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AStoryAdventureGameCharacter::OnOverlapBegin);
	Collision->OnComponentEndOverlap.AddDynamic(this, &AStoryAdventureGameCharacter::OnOverlapEnd);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> WakeUpInstanceMontage (TEXT("AnimMontage'/Game/ParagonPhase/Characters/Heroes/Phase/Animations/WakeUpMontage.WakeUpMontage'"));
	if (WakeUpInstanceMontage.Succeeded())
	{
		WakeUpMontage = WakeUpInstanceMontage.Object;
	}
	
	
}

//////////////////////////////////////////////////////////////////////////
// Input

void AStoryAdventureGameCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AStoryAdventureGameCharacter::BeginJump);
	//PlayerInputComponent->BindAction("Jump", IE_Released, this, &AStoryAdventureGameCharacter::EndJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AStoryAdventureGameCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AStoryAdventureGameCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AStoryAdventureGameCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AStoryAdventureGameCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AStoryAdventureGameCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AStoryAdventureGameCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AStoryAdventureGameCharacter::OnResetVR);
	PlayerInputComponent->BindAction("ZoomOut", IE_Pressed, this, &AStoryAdventureGameCharacter::MouseWheelDown);
	PlayerInputComponent->BindAction("ZoomIn", IE_Pressed, this, &AStoryAdventureGameCharacter::MouseWheelUp);
	PlayerInputComponent->BindAction("Force", IE_Pressed, this, &AStoryAdventureGameCharacter::BeginForce);

	PlayerInputComponent->BindAction("ClimbOut", IE_Pressed, this, &AStoryAdventureGameCharacter::ClimbOut);
}

void AStoryAdventureGameCharacter::BeginJump()
{
	Jump();
	if (isClimbing)
	{
		isClimbing = false;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("space bar pressed"));
		/*auto targetLocation = wallsFounded.ImpactPoint - GetActorForwardVector() * 56.f;
		targetLocation.Z = ledgesFounded.ImpactPoint.Z -96.f;
		GetCapsuleComponent()->SetRelativeTransform(FTransform(GetActorRotation(), targetLocation));*/
		disableMovement = true;
		PlayAnimMontage(WakeUpMontage, 1.f, FName("start_1"));
	}
	
}

void AStoryAdventureGameCharacter::MontagePlayEnded(UAnimMontage * animMontage, bool bIsInterrupted)
{
	if (animMontage == WakeUpMontage)
	{
		/*auto targetLocation = wallsFounded.ImpactPoint + GetActorForwardVector() * 56.f;
		targetLocation.Z = ledgesFounded.ImpactPoint.Z + 96.f;
		GetCapsuleComponent()->SetRelativeTransform(FTransform(GetActorRotation(), targetLocation));*/
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		IGrabable::Execute_CanGrabEvent(GetMesh()->GetAnimInstance(), false);
		disableMovement = false;
	}
}


void AStoryAdventureGameCharacter::OnOverlapBegin(UPrimitiveComponent * OverlappedComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("start overlap event"));
	canTrace = true;
}

void AStoryAdventureGameCharacter::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("end overlap event"));
	canTrace = false;
}

void AStoryAdventureGameCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AStoryAdventureGameCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AStoryAdventureGameCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AStoryAdventureGameCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AStoryAdventureGameCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AStoryAdventureGameCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		if (isClimbing)
		{

		} else {
			if (!disableMovement)
			{
				const FRotator Rotation = Controller->GetControlRotation();
				const FRotator YawRotation(0, Rotation.Yaw, 0);

				// get forward vector
				const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
				AddMovementInput(Direction, Value);
			}
			
		}
	}
}

void AStoryAdventureGameCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		if (isClimbing)
		{

		} else {
			if (!disableMovement)
			{
				// find out which way is right
				const FRotator Rotation = Controller->GetControlRotation();
				const FRotator YawRotation(0, Rotation.Yaw, 0);

				// get right vector 
				const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
				// add movement in that direction
				AddMovementInput(Direction, Value);
			}
			
		}
	}
}

void AStoryAdventureGameCharacter::MouseWheelDown() 
{
	if (CameraBoom->TargetArmLength < MaxCameraDistance)
		CameraBoom->TargetArmLength += 50.f;
	else
		CameraBoom->TargetArmLength = MaxCameraDistance;
}

void AStoryAdventureGameCharacter::MouseWheelUp()
{
	if (CameraBoom->TargetArmLength > MinCameraDistance)
		CameraBoom->TargetArmLength -= 50.f;
	else
		CameraBoom->TargetArmLength = MinCameraDistance;
}

void AStoryAdventureGameCharacter::BeginForce()
{
	if (Stamina >= ForceCoast)
	{
		if (bIsForced)
		{
			bIsForced = false;
			GetCharacterMovement()->MaxWalkSpeed /= SpeedUpValue;
		} else
		{
			bIsForced = true;
			GetCharacterMovement()->MaxWalkSpeed *= SpeedUpValue;
		}
	}
		
}

void AStoryAdventureGameCharacter::ClimbOut()
{
	if (isClimbing)
	{
		canTrace = false;
		isClimbing = false;
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		IGrabable::Execute_CanGrabEvent(GetMesh()->GetAnimInstance(), false);
	}
}

bool AStoryAdventureGameCharacter::FoundBlock(FVector positionVec, FVector forwardVec)
{
	///walltracervecParams
	
	//positionVec += forwardVec * 50.f;
	positionVec.Z -= heightOffset;
	forwardVec.X *= 150.f;
	forwardVec.Y *= 150.f;
	forwardVec.Z -= heightOffset;
	forwardVec = forwardVec + positionVec;
	///collision params
	FCollisionQueryParams collisionParams;

	//return ActorLineTraceSingle(wallsFounded, positionVec, forwardVec, ECC_WorldStatic, &collisionParams.DefaultQueryParam);
	return GetWorld()->LineTraceSingleByChannel(wallsFounded, positionVec, forwardVec, ECC_WorldStatic, &collisionParams.DefaultQueryParam);
}

bool AStoryAdventureGameCharacter::FoundLedge(FVector positionVec, FVector forwardVec)
{
	///ledge tracer vec params
	positionVec.Z += 200.f;
	FVector calculatedStartPosLedge = positionVec + forwardVec * 70.f;
	FVector calculatedEndPosLedge = calculatedStartPosLedge;

	calculatedEndPosLedge.Z -= 250.f;

	FCollisionQueryParams collisionParams;

	return GetWorld()->LineTraceSingleByChannel(ledgesFounded, calculatedStartPosLedge, calculatedEndPosLedge, ECC_WorldStatic, &collisionParams.DefaultQueryParam);
}

void AStoryAdventureGameCharacter::Tick(float perSecondTick)
{
	AActor::Tick(perSecondTick);
	auto chMovement = GetCharacterMovement();
	
	if (iterator % 10 == 0)
	{
		auto characterPositionVec = GetActorLocation();
		auto characterForwardVec = GetActorForwardVector();
		FVector tl;
		if (canTrace)
		{
			if (FoundBlock(characterPositionVec, characterForwardVec) && FoundLedge(characterPositionVec, characterForwardVec))
			{
				auto targetLocation = wallsFounded.ImpactPoint;
				targetLocation.Z = ledgesFounded.ImpactPoint.Z - 96.f;
				tl = targetLocation;
				IGrabable::Execute_CanGrabEvent(GetMesh()->GetAnimInstance(), true);
				if (chMovement->IsFalling() && !isClimbing)
				{
					chMovement->SetMovementMode(EMovementMode::MOVE_Flying);

					GetCapsuleComponent()->SetRelativeLocation(targetLocation);

					chMovement->StopMovementImmediately();
					isClimbing = true;
				}
			} else
			{
				IGrabable::Execute_CanGrabEvent(GetMesh()->GetAnimInstance(), false);
			}
		}
		iterator = 0;
	}
	++iterator;
	
	///speedUp and stamina recovery
	if (bIsForced)
	{
		if (Stamina >= ForceCoast)
		{
			Stamina = Stamina - ForceCoast;
		} else 
		{
			bIsForced = false;
			GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed;
		}
	} else {
		if (Stamina < MaxStamina)
			Stamina += 1.f;
	}
}

void AStoryAdventureGameCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetMesh()->GetAnimInstance()->OnMontageEnded.AddDynamic(this, &AStoryAdventureGameCharacter::MontagePlayEnded);
}

void AStoryAdventureGameCharacter::CanGrabEvent(bool canGrab) 
{

}

void AStoryAdventureGameCharacter::CanWakeUpEvent(bool canWakeUp)
{

}