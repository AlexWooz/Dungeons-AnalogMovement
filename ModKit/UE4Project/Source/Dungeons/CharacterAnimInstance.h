#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterAnimInstance.generated.h"

// Stub of the game's native /Script/Dungeons.CharacterAnimInstance, the parent
// of the hero's AnimBP (A_Player_Animation). It only exists so Blueprints can
// reference these properties by name; at runtime they resolve against the real
// class in the game executable. The module DLL is never packaged.
// Names come from A_Player_Animation's package name table. The types are inferred:
// RelativeMoveSpeed feeds an FClamp into the Walk/Run blendspace, and
// LocomotionWalkVariedPlayRate is a play rate. Treat both as unconfirmed.
UCLASS()
class DUNGEONS_API UCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// Drives the WalkRun axis of Player_Walk_Run (walk sample at 0.3, run at 1.2).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RelativeMoveSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LocomotionWalkVariedPlayRate = 1.f;
};
