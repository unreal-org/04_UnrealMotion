// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MainAnimInstance.generated.h"

class UCapsuleComponent;

/**
 * 
 */
UCLASS(Transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class UNREALMOTION_API UMainAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// Constructor
	UMainAnimInstance(const FObjectInitializer& ObjectInitializer);

	// Body Parts
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Body Parts")
	FVector RightFootLocation;   // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Body Parts")
	FVector LeftFootLocation;    // world space

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Joint Targets")
	FVector RightJointTargetLocation;   // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Joint Targets")
	FVector LeftJointTargetLocation;    // world space

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Joint Rotations")
	FRotator RightFootRotation;   // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Joint Rotations")
	FRotator LeftFootRotation;    // world space

	// IK Alpha
	UPROPERTY(BlueprintReadOnly, Category= "IK Alpha")
	float LeftIKAlpha = 0;
	UPROPERTY(BlueprintReadOnly, Category= "IK Alpha")
	float RightIKAlpha = 0;

	// Transition Events
	UFUNCTION(BlueprintCallable)
	void AnimNotify_IdleEntry();

protected:
	// Native initialization override point
	virtual void NativeInitializeAnimation() override;

	// Tick
	virtual void NativeUpdateAnimation(float DeltaTimeX) override;

private:
	UCapsuleComponent* CapsuleComponent = nullptr;

	// State Machine
	FAnimNode_StateMachine *MainState;

	// Foot Trace
	FName TraceTag = FName(TEXT("TraceTag"));
	FCollisionQueryParams TraceParameters;
	FVector IKFootTrace(int32 Foot);

};