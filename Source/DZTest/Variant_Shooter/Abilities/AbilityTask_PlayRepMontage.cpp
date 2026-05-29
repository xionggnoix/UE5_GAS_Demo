// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilityTask_PlayRepMontage.h"
#include "GASMontageReplicationComponent.h"
#include "ShooterCharacter.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"

UAbilityTask_PlayRepMontage* UAbilityTask_PlayRepMontage::PlayRepMontage(
	UGameplayAbility* OwningAbility,
	UAnimMontage* InFirstPersonMontage,
	UAnimMontage* InThirdPersonMontage,
	float InPlayRate,
	FName InStartSection)
{
	UAbilityTask_PlayRepMontage* Task = NewAbilityTask<UAbilityTask_PlayRepMontage>(OwningAbility);
	Task->FirstPersonMontage = InFirstPersonMontage;
	Task->ThirdPersonMontage = InThirdPersonMontage;
	Task->PlayRate = InPlayRate;
	Task->StartSection = InStartSection;
	return Task;
}

void UAbilityTask_PlayRepMontage::Activate()
{
	Super::Activate();
	if (!Ability)
	{
		EndTask();
		return;
	}
	AShooterCharacter* Character = Cast<AShooterCharacter>(GetAvatarActor());
	if (!Character)
	{
		EndTask();
		return;
	}
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		PredictionKey = ASC->ScopedPredictionKey;
		bIsPredicted = PredictionKey.IsValidKey();
	}
	MontageReplicationComponent = Character->GetMontageReplicationComponent();
	if (!MontageReplicationComponent.IsValid())
	{
		EndTask();
		return;
	}
	float Duration = MontageReplicationComponent->PlayRepMontage(FirstPersonMontage, ThirdPersonMontage, PlayRate, StartSection);
	if (Duration <= 0.0f)
	{
		EndTask();
		return;
	}
	bPlayedMontage = true;
	if (UAnimInstance* AnimInstance = Character->GetFirstPersonMesh()->GetAnimInstance())
	{
		AnimInstance->OnMontageEnded.AddDynamic(this, &UAbilityTask_PlayRepMontage::OnMontageEnded);
		AnimInstance->OnMontageBlendingOut.AddDynamic(this, &UAbilityTask_PlayRepMontage::OnMontageBlendingOut);
	}
}

void UAbilityTask_PlayRepMontage::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != FirstPersonMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast();
		}
	}
	else
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnBlendOut.Broadcast();
		}
	}
}

void UAbilityTask_PlayRepMontage::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != FirstPersonMontage)
	{
		return;
	}

	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast();
		}
	}

	EndTask();
}

void UAbilityTask_PlayRepMontage::ExternalCancel()
{
	// 如果是预测播放且被取消，说明服务器拒绝了，需要回滚
	// if (bIsPredicted)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("AbilityTask_PlayRepMontage: Prediction rejected, rolling back montage"));
	// }
	StopPlayingMontage();
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCancelled.Broadcast();
	}
	Super::ExternalCancel();
}

void UAbilityTask_PlayRepMontage::OnDestroy(bool bInOwnerFinished)
{
	StopPlayingMontage();
	Super::OnDestroy(bInOwnerFinished);
}

void UAbilityTask_PlayRepMontage::StopPlayingMontage()
{
	if (!bPlayedMontage)
	{
		return;
	}

	bPlayedMontage = false;

	if (MontageReplicationComponent.IsValid())
	{
		MontageReplicationComponent->StopRepMontage(0.2f);
	}
}
