// Fill out your copyright notice in the Description page of Project Settings.

#include "MainAnimInstance.h"
#include "Animation/AnimNode_StateMachine.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/EngineTypes.h"
#include "EngineUtils.h"
#include "Containers/Array.h"

// Constructors
UMainAnimInstance::UMainAnimInstance(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{}

void UMainAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    if (!ensure(GetSkelMeshComponent())) { return; }
    if (!ensure(GetSkelMeshComponent()->GetOwner())) { return; }

    TraceParameters = FCollisionQueryParams(TraceTag, false);
    TraceParameters.AddIgnoredComponent(Cast<UPrimitiveComponent>(GetSkelMeshComponent()));
    TraceParameters.AddIgnoredActor(Cast<AActor>(GetSkelMeshComponent()->GetOwner()));

    MainState = GetStateMachineInstanceFromName(FName(TEXT("MainState")));
    CapsuleComponent = GetSkelMeshComponent()->GetOwner()->FindComponentByClass<UCapsuleComponent>();

    for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AStaticMeshActor* Target = *It;
		IgnoredActors.Add(Target);
	}

    Spine.Add(FJoint(FName(TEXT("neck_01")), FRotator(50, 90, 75)));
    Spine.Add(FJoint(FName(TEXT("spine_03")), FRotator(25, 45, 50)));
    Spine.Add(FJoint(FName(TEXT("spine_02")), FRotator(10, 25, 25)));
    Spine.Add(FJoint(FName(TEXT("spine_01")), FRotator(5, 15, 10)));
}

void UMainAnimInstance::NativeUpdateAnimation(float DeltaTimeX)
{
    if (!ensure(MainState)) { return; }

    // UE_LOG(LogTemp, Warning, TEXT("%i"), MainState->GetCurrentState())
    switch (MainState->GetCurrentState())
    {
        case 0: // Idle
            LeftFootLocation = IKFootTrace(0);
            RightFootLocation = IKFootTrace(1);
            SphereTrace(DeltaTimeX);
            break;
    }

}

void UMainAnimInstance::AnimNotify_IdleEntry()
{
    LeftIKAlpha = .95;
    RightIKAlpha = .95;
}

void UMainAnimInstance::TargetLerp(float DeltaTimeX, float Beta)
{
    // Clamp Angles
    RecursiveClamp(Spine, 0);

    TurnTime = 0;
    if (TurnTime < TurnDuration)
    {
        TurnTime += DeltaTimeX;
        NeckRotation = FMath::Lerp(NeckRotation, Spine[0].TargetJointRotation, TurnTime / Beta);
        Spine3Rotation = FMath::Lerp(Spine3Rotation, Spine[1].TargetJointRotation, TurnTime / Beta);
        Spine2Rotation = FMath::Lerp(Spine2Rotation, Spine[2].TargetJointRotation, TurnTime / Beta);
        Spine1Rotation = FMath::Lerp(Spine1Rotation, Spine[3].TargetJointRotation, TurnTime / Beta);
    }
}

void UMainAnimInstance::RecursiveClamp(TArray<FJoint> BoneChain, int i)
{
    FRotator InitialTargetRotation = BoneChain[i].TargetJointRotation; 
    RotatorClamp(BoneChain[i].TargetJointRotation, BoneChain[i].ClampRotation);

    if (i + 1 == BoneChain.Num()) { return; }

    if (abs(BoneChain[i].TargetJointRotation.Pitch) == BoneChain[i].ClampRotation.Pitch
        || abs(BoneChain[i].TargetJointRotation.Yaw) == BoneChain[i].ClampRotation.Yaw
        || abs(BoneChain[i].TargetJointRotation.Roll) == BoneChain[i].ClampRotation.Roll)
    {
        if (abs(TargetNeckRotation.Pitch) == BoneChain[i].ClampRotation.Pitch) {
            BoneChain[i+1].TargetJointRotation.Pitch = InitialTargetRotation.Pitch - BoneChain[i].ClampRotation.Pitch;
        }
        if (abs(TargetNeckRotation.Yaw) == BoneChain[i].ClampRotation.Yaw) {
            BoneChain[i+1].TargetJointRotation.Pitch = InitialTargetRotation.Pitch - BoneChain[i].ClampRotation.Yaw;
        }
        if (abs(TargetNeckRotation.Roll) == BoneChain[i].ClampRotation.Roll) {
            BoneChain[i+1].TargetJointRotation.Pitch = InitialTargetRotation.Pitch - BoneChain[i].ClampRotation.Roll;
        }
    } else {
        BoneChain[i+1].TargetJointRotation = FRotator(0, 0, 0);
    }

    RecursiveClamp(BoneChain, i+1);
}

void UMainAnimInstance::RotatorClamp(FRotator TargetRotator, FRotator ClampRotator)
{
    TargetRotator.Pitch = FMath::ClampAngle(TargetRotator.Pitch, -ClampRotator.Pitch, ClampRotator.Pitch);
    TargetRotator.Yaw = FMath::ClampAngle(TargetRotator.Yaw, -ClampRotator.Yaw, ClampRotator.Yaw);
    TargetRotator.Roll = FMath:: ClampAngle(TargetRotator.Roll, -ClampRotator.Roll, ClampRotator.Roll);
}

void UMainAnimInstance::SphereTrace(float DeltaTimeX)
{
    if (!ensure(GetSkelMeshComponent())) { return; }

    FVector Start = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("sphere_trace_start")));
    FVector End = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("sphere_trace_end")));
    FHitResult TraceResult(ForceInit);

    bool Trace = UKismetSystemLibrary::SphereTraceSingle(
        GetWorld(),
        Start,
        End,
        150,
        ETraceTypeQuery::TraceTypeQuery1,
        false,
        IgnoredActors,
        EDrawDebugTrace::ForOneFrame,
        TraceResult,
        true,
        FLinearColor(0, 0, 255, 1),
        FLinearColor(255, 0, 0, 1),
        1
    );

    if (Trace) {
        if (!ensure(TraceResult.GetActor())) { return; }

        FVector Neck = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("neck_01")));
        Spine[0].TargetJointRotation = UKismetMathLibrary::FindLookAtRotation(Neck, TraceResult.GetActor()->GetActorLocation());

    } else {
        Spine[0].TargetJointRotation = FRotator(0, 0, 0);
    }

    TargetLerp(DeltaTimeX, 0.5);
    return;
}

FVector UMainAnimInstance::IKFootTrace(int32 Foot)
{
    if (!ensure(GetSkelMeshComponent())) { return FVector(0, 0, 0); }
    if (!ensure(CapsuleComponent)) { return FVector(0, 0, 0); }

    FName FootName;
    FVector FootSocketLocation;
    if (Foot == 0) {
        FootName = FName(TEXT("foot_l"));
        LeftJointTargetLocation = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("joint_target_l")));
        FootSocketLocation = GetSkelMeshComponent()->GetSocketLocation(FootName);
    } else if (Foot == 1) { 
        FootName = FName(TEXT("foot_r"));
        RightJointTargetLocation = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("joint_target_r")));
        FootSocketLocation = GetSkelMeshComponent()->GetSocketLocation(FootName);
    }

    float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
    FVector StartTrace = FVector(FootSocketLocation.X, FootSocketLocation.Y, CapsuleHalfHeight);
    FVector EndTrace = FVector(FootSocketLocation.X, FootSocketLocation.Y, CapsuleHalfHeight - CapsuleHalfHeight - 15); // 15 = trace distance;

    FHitResult HitResult(ForceInit);

    if (!ensure(GetWorld())) { return FVector(0, 0, 0); }
    bool HitConfirm = GetWorld()->LineTraceSingleByChannel(   // Line trace for feet
            HitResult,
            StartTrace,
            EndTrace,
            ECollisionChannel::ECC_Visibility,
            TraceParameters
    );

    if (HitConfirm)
    {
        if (!ensure(HitResult.GetActor())) { return FVector(0, 0, 0); }

        // FootOffset Z - TODO: Trigger if less than 13.5
        FootSocketLocation.Z = (HitResult.Location - HitResult.TraceEnd).Size() - 15 + 13.5;
        
        // Foot Rotations
        if (Foot == 0) { 
        LeftFootRotation.Roll = UKismetMathLibrary::DegAtan2(HitResult.Normal.Y, HitResult.Normal.Z);
        LeftFootRotation.Pitch = UKismetMathLibrary::DegAtan2(HitResult.Normal.X, HitResult.Normal.Z);
        } else { 
        RightFootRotation.Roll = UKismetMathLibrary::DegAtan2(HitResult.Normal.Y, HitResult.Normal.Z);
        RightFootRotation.Pitch = UKismetMathLibrary::DegAtan2(HitResult.Normal.X, HitResult.Normal.Z);
        } 

        return FootSocketLocation; 
    }
    
    return FootSocketLocation;  // else - don't offset
}