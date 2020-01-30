// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "StoryAdventureGameGameMode.h"
#include "StoryAdventureGameCharacter.h"
#include "UObject/ConstructorHelpers.h"

AStoryAdventureGameGameMode::AStoryAdventureGameGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ParagonPhase/Characters/Heroes/Phase/PhasePlayerCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	
}
