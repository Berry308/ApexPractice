// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonShootGameMode.h"
#include "FirstPersonShootCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFirstPersonShootGameMode::AFirstPersonShootGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
