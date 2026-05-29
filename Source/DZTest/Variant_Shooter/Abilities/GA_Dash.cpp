// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Dash.h"
#include "AbilityTask_PlayRepMontage.h"
#include "ShooterCharacter.h"
#include "AbilitySystemComponent.h"

UGA_Dash::UGA_Dash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool UGA_Dash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void UGA_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// UE_LOG(LogTemp, Log, TEXT("GA_Dash ActivateAbility: Avatar=%s Authority=%d"),
		*GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr),
		(ActorInfo && ActorInfo->IsNetAuthority()) ? 1 : 0);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		// UE_LOG(LogTemp, Warning, TEXT("GA_Dash: CommitAbility failed"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AShooterCharacter* Character = Cast<AShooterCharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		// UE_LOG(LogTemp, Warning, TEXT("GA_Dash: Character is null"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// UE_LOG(LogTemp, Log, TEXT("GA_Dash: FirstPersonDashMontage=%s"), *GetNameSafe(FirstPersonDashMontage));
	// UE_LOG(LogTemp, Log, TEXT("GA_Dash: ThirdPersonDashMontage=%s"), *GetNameSafe(ThirdPersonDashMontage));
	// UE_LOG(LogTemp, Log, TEXT("GA_Dash: MontagePlayRate=%.2f"), MontagePlayRate);

	if (!FirstPersonDashMontage && !ThirdPersonDashMontage)
	{
		// UE_LOG(LogTemp, Warning, TEXT("GA_Dash: Both FirstPersonDashMontage and ThirdPersonDashMontage are null"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayRepMontage* MontageTask = nullptr;
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (ASC && !ActorInfo->IsNetAuthority())
	{
		// UE_LOG(LogTemp, Log, TEXT("GA_Dash: Using predicted montage task"));
		FScopedPredictionWindow ScopedPrediction(ASC);
		MontageTask = UAbilityTask_PlayRepMontage::PlayRepMontage(
			this,
			FirstPersonDashMontage,
			ThirdPersonDashMontage,
			MontagePlayRate,
			NAME_None);
	}
	else
	{
		// UE_LOG(LogTemp, Log, TEXT("GA_Dash: Using non-predicted montage task"));
		MontageTask = UAbilityTask_PlayRepMontage::PlayRepMontage(
			this,
			FirstPersonDashMontage,
			ThirdPersonDashMontage,
			MontagePlayRate,
			NAME_None);
	}

	if (!MontageTask)
	{
		// UE_LOG(LogTemp, Warning, TEXT("GA_Dash: MontageTask creation failed"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// UE_LOG(LogTemp, Log, TEXT("GA_Dash: MontageTask created successfully"));

	MontageTask->OnCompleted.AddDynamic(this, &UGA_Dash::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Dash::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Dash::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Dash::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Dash::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Dash::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
