// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/KismetSystemLibrary.h"

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MainAnimInstance.generated.h"

class UCapsuleComponent;
class AActor;

USTRUCT()
struct FJoint
{
	GENERATED_BODY()

	UPROPERTY()
	FName JointName;

	UPROPERTY()
	FRotator TargetJointRotation;

	UPROPERTY()
	FRotator ClampRotation;

	FJoint() {}

	FJoint(FName Name, FRotator Joint, FRotator Clamp)
	{
		JointName = Name;
		TargetJointRotation = Joint;
		ClampRotation = Clamp;
	}
};

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
	FVector LeftKneeTargetLocation;   // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Joint Targets")
	FVector RightKneeTargetLocation;    // world space

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Body Parts")
	FVector RightHandLocation;   // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Body Parts")
	FVector LeftHandLocation;    // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Joint Targets")
	FVector LeftElbowTargetLocation;   // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Joint Targets")
	FVector RightElbowTargetLocation;    // world space


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Body Parts")
	FRotator NeckRotation;    // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Body Parts")
	FRotator Spine3Rotation;    // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Body Parts")
	FRotator Spine2Rotation;    // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Body Parts")
	FRotator Spine1Rotation;    // world space

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Joint Rotations")
	FRotator RightFootRotation;   // world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Joint Rotations")
	FRotator LeftFootRotation;    // world space

	// IK Alpha
	UPROPERTY(BlueprintReadOnly, Category= "IK Alpha")
	float LeftFootIKAlpha = 0;
	UPROPERTY(BlueprintReadOnly, Category= "IK Alpha")
	float RightFootIKAlpha = 0;
	UPROPERTY(BlueprintReadOnly, Category= "IK Alpha")
	float LeftHandIKAlpha = 0;
	UPROPERTY(BlueprintReadOnly, Category= "IK Alpha")
	float RightHandIKAlpha = 0;


	// Transition Events
	UFUNCTION(BlueprintCallable)
	void AnimNotify_IdleEntry();

protected:
	// Native initialization override point
	virtual void NativeInitializeAnimation() override;

	// Tick
	virtual void NativeUpdateAnimation(float DeltaTimeX) override;

private:
	// Capsule Component
	UCapsuleComponent* CapsuleComponent = nullptr;

	// State Machine
	FAnimNode_StateMachine *MainState;

	// Lerp
	void TargetLerp(float DeltaTimeX, float Beta);
	float LerpTime = 0;
	float LerpDuration = 0.3;

	// Clamp
	FRotator RotatorClamp(FRotator TargetRotator, FRotator RotatorClamp);
	FRotator RotationAdjust(TArray<FJoint> BoneChain, int i, FRotator InitialTargetRotation);
	TArray<FJoint> Spine;

	// Head Trace
	void SphereTrace(float DeltaTimeX);
	TArray<AActor*> IgnoredActors;
	FRotator TargetNeckRotation;

	// Foot Trace
	FName TraceTag = FName(TEXT("TraceTag"));
	FCollisionQueryParams TraceParameters;
	FVector IKFootTrace(int32 Foot);

	// Hand Trace
	void IKHands(float DeltaTimeX);
	void TargetInterp(FVector LeftHandInterpTo, FVector RightHandInterpTo, float DeltaTimeX);
	float InterpTime = 0;
	float InterpDuration = 0.3;
	float InterpSpeed = 2;

	// Threat Variable
	float Threat = 0;
	float ThreatMin = 0;
	float ThreatMax = 50;
	FVector ThreatVector;
};
