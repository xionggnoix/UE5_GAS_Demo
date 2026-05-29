// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_FireWeapon.h"
#include "AbilityTask_PlayRepMontage.h"
#include "ShooterCharacter.h"
#include "Weapons/ShooterWeapon.h"
#include "AbilitySystemComponent.h"

UGA_FireWeapon::UGA_FireWeapon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool UGA_FireWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	AShooterCharacter* Character = Cast<AShooterCharacter>(ActorInfo->AvatarActor.Get());
	if (!Character || !Character->GetCurrentWeapon())
	{
		return false;
	}

	return true;
}

void UGA_FireWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AShooterCharacter* Character = Cast<AShooterCharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AShooterWeapon* Weapon = Character->GetCurrentWeapon();
	if (!Weapon)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* FPMontage = FirstPersonFireMontage.Get();
	if (!FPMontage)
	{
		FPMontage = Weapon->GetFiringMontage();
	}
	// if (!FPMontage)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("GA_FireWeapon: No FiringMontage found for weapon %s"), *Weapon->GetName());
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Log, TEXT("GA_FireWeapon: Using FiringMontage %s"), *FPMontage->GetName());
	// }
	
	UAnimMontage* TPMontage = ThirdPersonFireMontage.Get();
	if (!TPMontage)
	{
		TPMontage = Weapon->GetFiringMontage();
	}

	if (FPMontage)
	{
		UAbilityTask_PlayRepMontage* MontageTask = nullptr;
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		
		if (ASC && !ActorInfo->IsNetAuthority())
		{
			FScopedPredictionWindow ScopedPrediction(ASC);
			MontageTask = UAbilityTask_PlayRepMontage::PlayRepMontage(
				this, FPMontage, TPMontage, MontagePlayRate, NAME_None);
		}
		else
		{
			MontageTask = UAbilityTask_PlayRepMontage::PlayRepMontage(
				this, FPMontage, TPMontage, MontagePlayRate, NAME_None);
		}

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &UGA_FireWeapon::OnMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &UGA_FireWeapon::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &UGA_FireWeapon::OnMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}

	if (ActorInfo->IsNetAuthority())
	{
		ServerFire();
	}

	if (!FPMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_FireWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_FireWeapon::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_FireWeapon::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_FireWeapon::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_FireWeapon::ServerFire()
{
	AShooterCharacter* Character = Cast<AShooterCharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	AShooterWeapon* Weapon = Character->GetCurrentWeapon();
	if (!Weapon)
	{
		return;
	}

	FVector TargetLocation = Character->GetWeaponTargetLocation();
	Weapon->SpawnProjectile(TargetLocation);

	Character->AddWeaponRecoil(Weapon->GetFiringRecoil());
}
