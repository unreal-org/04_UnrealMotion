// Fill out your copyright notice in the Description page of Project Settings.

#include "MainAnimInstance.h"
#include "Animation/AnimNode_StateMachine.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/EngineTypes.h"
#include "EngineUtils.h"

// Constructors
UMainAnimInstance::UMainAnimInstance(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
    //SphereTraceBase = UKismetSystemLibrary(ObjectInitializer);
}

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
            break;
    }
}

void UMainAnimInstance::AnimNotify_IdleEntry()
{
    LeftIKAlpha = .95;
    RightIKAlpha = .95;
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

void UMainAnimInstance::SphereTrace()
{
    if (!ensure(GetSkelMeshComponent())) { return; }

    FVector Start = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("sphere_trace_start")));
    FVector End = GetSkelMeshComponent()->GetSocketLocation(FName(TEXT("sphere_trace_end")));
    FHitResult HitResult(ForceInit);

    bool Trace = UKismetSystemLibrary::SphereTraceSingle(
        GetWorld(),
        Start,
        End,
        50,
        ETraceTypeQuery::TraceTypeQuery1,
        false,
        IgnoredActors,
        EDrawDebugTrace::None,
        HitResult,
        true,
        FLinearColor(0, 0, 0, 0),
        FLinearColor(1, 1, 1, 1),
        1
    );
}