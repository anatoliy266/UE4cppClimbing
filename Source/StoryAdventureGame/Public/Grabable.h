// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Grabable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGrabable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class STORYADVENTUREGAME_API IGrabable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Gameplay)
	void CanGrabEvent(bool canGrab);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Gameplay)
	void CanWakeUpEvent(bool canWakeUp);
};
