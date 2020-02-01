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

    MainState = GetStateMachineInstanceFromName(FName(TEXT("MainState")));
    CapsuleComponent = GetSkelMeshComponent()->GetOwner()->FindComponentByClass<UCapsuleComponent>();

    TraceParameters = FCollisionQueryParams(TraceTag, false);
    TraceParameters.AddIgnoredComponent(Cast<UPrimitiveComponent>(GetSkelMeshComponent()));
    TraceParameters.AddIgnoredActor(Cast<AActor>(GetSkelMeshComponent()->GetOwner()));

    for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AStaticMeshActor* Target = *It;
		IgnoredActors.Add(Target);
	}

    Spine.Add(FJoint(FName(TEXT("neck_01")), FRotator(0, 0, 0), FRotator(45, 70, 60)));
    Spine.Add(FJoint(FName(TEXT("spine_03")), FRotator(0, 0, 0), FRotator(25, 30, 30)));
    Spine.Add(FJoint(FName(TEXT("spine_02")), FRotator(0, 0, 0), FRotator(10, 20, 20)));
    Spine.Add(FJoint(FName(TEXT("spine_01")), FRotator(0, 0, 0), FRotator(5, 10, 10)));
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
            IKHands(DeltaTimeX);
            SphereTrace(DeltaTimeX);
            break;
    }

}

void UMainAnimInstance::AnimNotify_IdleEntry()
{
    LeftFootIKAlpha = .95;
    RightFootIKAlpha = .95;
    LeftHandIKAlpha = .2;
    RightHandIKAlpha = .2;
}

void UMainAnimInstance::TargetLerp(float DeltaTimeX, float Beta)
{
    // Clamp Angles
    FRotator TargetRotation = TargetNeckRotation;
    FRotator Sway = FRotator(-ThreatVector.X * 15, 0, ThreatVector.Y * 15);  

    Spine[0].TargetJointRotation = RotatorClamp(TargetRotation, Spine[0].ClampRotation);
    TargetRotation = RotationAdjust(Spine, 0, TargetRotation);
    Spine[1].TargetJointRotation = RotatorClamp(TargetRotation, Spine[0].ClampRotation) + Sway;
    TargetRotation = RotationAdjust(Spine, 1, TargetRotation);
    Spine[2].TargetJointRotation = RotatorClamp(TargetRotation, Spine[0].ClampRotation);
    TargetRotation = RotationAdjust(Spine, 2, TargetRotation);
    Spine[3].TargetJointRotation = RotatorClamp(TargetRotation, Spine[0].ClampRotation);

    LerpTime = 0;
    if (LerpTime < LerpDuration)
    {
        LerpTime += DeltaTimeX;
        NeckRotation = FMath::Lerp(NeckRotation, Spine[0].TargetJointRotation, LerpTime / 0.25);
        Spine3Rotation = FMath::Lerp(Spine3Rotation, Spine[1].TargetJointRotation, LerpTime / 0.4);
        Spine2Rotation = FMath::Lerp(Spine2Rotation, Spine[2].TargetJointRotation, LerpTime / 0.45);
        Spine1Rotation = FMath::Lerp(Spine1Rotation, Spine[3].TargetJointRotation, LerpTime / Beta);
    }
}

FRotator UMainAnimInstance::RotationAdjust(TArray<FJoint> BoneChain, int i, FRotator InitialTargetRotation)
{   
    if (i >= BoneChain.Num()) { return InitialTargetRotation; }

    if (abs(BoneChain[i].TargetJointRotation.Pitch) == BoneChain[i].ClampRotation.Pitch
        || abs(BoneChain[i].TargetJointRotation.Yaw) == BoneChain[i].ClampRotation.Yaw
        || abs(BoneChain[i].TargetJointRotation.Roll) == BoneChain[i].ClampRotation.Roll)
    {
        if (abs(BoneChain[i].TargetJointRotation.Pitch) == BoneChain[i].ClampRotation.Pitch) {
            if (InitialTargetRotation.Pitch < 0) {
                InitialTargetRotation.Pitch += BoneChain[i].ClampRotation.Pitch;
            } else {
                InitialTargetRotation.Pitch -= BoneChain[i].ClampRotation.Pitch;
            }
        }
        if (abs(BoneChain[i].TargetJointRotation.Yaw) == BoneChain[i].ClampRotation.Yaw) {
            if (InitialTargetRotation.Yaw < 0) {
                InitialTargetRotation.Yaw += BoneChain[i].ClampRotation.Yaw;
            } else {
                InitialTargetRotation.Yaw -= BoneChain[i].ClampRotation.Yaw;
            }
        }
        if (abs(BoneChain[i].TargetJointRotation.Roll) == BoneChain[i].ClampRotation.Roll) {
            if (InitialTargetRotation.Roll < 0) {
                InitialTargetRotation.Roll += BoneChain[i].ClampRotation.Roll;
            } else {
                InitialTargetRotation.Roll -= BoneChain[i].ClampRotation.Roll;
            }
        }
    } else {
        InitialTargetRotation = FRotator(0, 0, 0);
    }

    return InitialTargetRotation;
}

FRotator UMainAnimInstance::RotatorClamp(FRotator TargetRotator, FRotator ClampRotator)
{
    TargetRotator.Pitch = FMath::ClampAngle(TargetRotator.Pitch, -ClampRotator.Pitch, ClampRotator.Pitch);
    TargetRotator.Yaw = FMath::ClampAngle(TargetRotator.Yaw, -ClampRotator.Yaw, ClampRotator.Yaw);
    TargetRotator.Roll = FMath:: ClampAngle(TargetRotator.Roll, -ClampRotator.Roll, ClampRotator.Roll);

    return TargetRotator;
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
        FVector TraceLocation = TraceResult.GetActor()->GetActorLocation();
        TargetNeckRotation = UKismetMathLibrary::FindLookAtRotation(Neck, TraceLocation);

        FVector ThreatDistance = Neck - TraceLocation;
        if (ThreatDistance.Size() < 50 && Threat < ThreatMax) { Threat += 0.01; }
        else if (ThreatDistance.Size() > 50 && Threat > ThreatMin) { Threat -= 0.01; }
        ThreatVector = UKismetMathLibrary::GetDirectionUnitVector(TraceLocation, Neck);

    } else {
        TargetNeckRotation = FRotator(0, 0, 0);
        ThreatVector = FVector(0, 0, 0);
        Threat = 0;
    }

    TargetLerp(DeltaTimeX, 0.5);
    return;
}

// TODO : Direct hands to cover vital spots in danger
void UMainAnimInstance::IKHands(float DeltaTimeX)
{
    if (!ensure(GetSkelMeshComponent())) { return; }

    LeftElbowTargetLocation = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("elbow_target_l")));
    RightElbowTargetLocation = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("elbow_target_r")));

    FVector LeftHandTarget = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("hand_l")));
    FVector RightHandTarget = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("hand_r")));

    TargetInterp(LeftHandTarget, RightHandTarget, DeltaTimeX);

    return;
}

void UMainAnimInstance::TargetInterp(FVector LeftHandInterpTo, FVector RightHandInterpTo, float DeltaTimeX)
{
    FVector LeftHandCurrent = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("hand_l")));
    FVector RightHandCurrent = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("hand_r")));
    
    if (InterpTime < InterpDuration) {
        InterpTime += DeltaTimeX;
        LeftHandLocation = FMath::VInterpTo(LeftHandCurrent, LeftHandInterpTo, DeltaTimeX, InterpSpeed);
        RightHandLocation = FMath::VInterpTo(RightHandCurrent, RightHandInterpTo, DeltaTimeX, InterpSpeed);
    }
}

FVector UMainAnimInstance::IKFootTrace(int32 Foot)
{
    if (!ensure(GetSkelMeshComponent())) { return FVector(0, 0, 0); }
    if (!ensure(CapsuleComponent)) { return FVector(0, 0, 0); }

    FName FootName;
    FVector FootSocketLocation;
    if (Foot == 0) {
        FootName = FName(TEXT("foot_l"));
        LeftKneeTargetLocation = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("knee_target_l")));
        FootSocketLocation = GetSkelMeshComponent()->GetSocketLocation(FootName);
    } else if (Foot == 1) { 
        FootName = FName(TEXT("foot_r"));
        RightKneeTargetLocation = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("knee_target_r")));
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