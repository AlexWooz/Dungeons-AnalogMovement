#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterAnimInstance.generated.h"

// Stubs of the game's native /Script/Dungeons types used by the hero's AnimBP
// (A_Player_Animation). They only exist so Blueprints can reference these
// properties by name; at runtime they resolve against the real types in the
// game executable. The module DLL is never packaged.
// Names and types come from A_Player_Animation's import table. Only the members
// the AnimBP references are known; the real struct has an enum
// FallBackLocomotionBlend too (enum type unknown, so it's left out).

USTRUCT(BlueprintType)
struct FAnimGraphStateConditions
{
	GENERATED_BODY()

	// Drives the WalkRun axis of Player_Walk_Run (walk sample at 0.3, run at 1.2), via FClamp.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RelativeMoveSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LocomotionWalkVariedPlayRate = 1.f;
};

UCLASS()
class DUNGEONS_API UCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAnimGraphStateConditions StateMachineConditions;
};
