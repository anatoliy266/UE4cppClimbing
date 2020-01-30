// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Public/Grabable.h"
#include "StoryAdventureGameCharacter.generated.h"


UCLASS(config=Game)
class AStoryAdventureGameCharacter : public ACharacter, public IGrabable
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AStoryAdventureGameCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UPROPERTY(VisibleAnyWhere, Category = Camera)
		float MaxCameraDistance;

	UPROPERTY(VisibleAnyWhere, Category = Camera)
		float MinCameraDistance;

	UPROPERTY(VisibleAnyWhere, Category = Force)
		bool bIsForced;

	UPROPERTY(EditAnywhere, Category = Force)
		float SpeedUpValue;

	UPROPERTY(VisibleAnyWhere, Category = PlayerParameters)
		float Stamina;

	UPROPERTY(VisibleAnyWhere, Category = PlayerParameters)
		float MaxStamina;

	UPROPERTY(EditAnywhere, Category = PlayerParameters)
		float ForceCoast;

	UPROPERTY(EditAnywhere, Category = PlayerParameters)
		float MaxWalkSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
		class UAnimMontage * WakeUpMontage;


	///climbing
	

	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = Climbing)
		class USphereComponent * Collision;
	///debug

	UFUNCTION()
		void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UFUNCTION()
		void MontagePlayEnded(UAnimMontage * animMontage, bool bIsInterrupted);

	UPROPERTY(VisibleAnyWhere, Category = Climbing)
		float heightOffset;
	UPROPERTY(EditAnywhere, Category = Climbing)
		bool canTrace;
	UPROPERTY(EditAnywhere, Category = Climbing)
		bool isClimbing;
	///can grab interface
		void CanGrabEvent(bool canGrab);
		void CanWakeUpEvent(bool canWakeUp);

		


protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	void MouseWheelDown();
	void MouseWheelUp();

	void BeginForce();
	void BeginJump();
	
	void ClimbOut();

	

	virtual void Tick(float perSecondTick) override;
	virtual void BeginPlay() override;

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

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	float ForceDelta;
	bool FoundBlock(FVector positionVec, FVector forwardVec);

	bool FoundLedge(FVector positionVec, FVector forwardVec);

	FHitResult wallsFounded;
	FHitResult ledgesFounded;

	bool disableMovement;
	int iterator;
	FAnimMontageInstance * mi;
};

