// Copyright Epic Games, Inc. All Rights Reserved.

#include "GASMontageReplicationComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "DZTestCharacter.h"
#include "Net/UnrealNetwork.h"

UGASMontageReplicationComponent::UGASMontageReplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGASMontageReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ADZTestCharacter* Character = Cast<ADZTestCharacter>(GetOwner()))
	{
		CachedFirstPersonMesh = Character->GetFirstPersonMesh();
		CachedThirdPersonMesh = Character->GetMesh();
	}
}

UAnimInstance* UGASMontageReplicationComponent::GetFirstPersonAnimInstance() const
{
	if (CachedFirstPersonMesh.IsValid())
	{
		return CachedFirstPersonMesh->GetAnimInstance();
	}
	return nullptr;
}

UAnimInstance* UGASMontageReplicationComponent::GetThirdPersonAnimInstance() const
{
	if (CachedThirdPersonMesh.IsValid())
	{
		return CachedThirdPersonMesh->GetAnimInstance();
	}
	return nullptr;
}

float UGASMontageReplicationComponent::PlayRepMontage(UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection)
{
	float Duration = 0.0f;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return 0.0f;
	}

	if (OwnerPawn->IsLocallyControlled())
	{
		if (UAnimInstance* AnimInstance = GetFirstPersonAnimInstance())
		{
			if (FirstPersonMontage)
			{
				Duration = AnimInstance->Montage_Play(FirstPersonMontage, PlayRate);
				if (Duration > 0.0f && StartSection != NAME_None)
				{
					AnimInstance->Montage_JumpToSection(StartSection, FirstPersonMontage);
				}
				CurrentFirstPersonMontage = FirstPersonMontage;
			}
		}
	}

	if (!OwnerPawn->HasAuthority())
	{
		ServerPlayMontage(FirstPersonMontage, ThirdPersonMontage, PlayRate, StartSection);
	}
	else
	{
		MulticastPlayThirdPersonMontage(ThirdPersonMontage, PlayRate, StartSection);
	}

	CurrentThirdPersonMontage = ThirdPersonMontage;

	return Duration;
}

void UGASMontageReplicationComponent::StopRepMontage(float BlendOutTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	if (OwnerPawn->IsLocallyControlled())
	{
		if (UAnimInstance* AnimInstance = GetFirstPersonAnimInstance())
		{
			if (CurrentFirstPersonMontage)
			{
				AnimInstance->Montage_Stop(BlendOutTime, CurrentFirstPersonMontage);
			}
		}
	}

	if (!OwnerPawn->HasAuthority())
	{
		ServerStopMontage(BlendOutTime);
	}
	else
	{
		MulticastStopThirdPersonMontage(BlendOutTime);
	}

	CurrentFirstPersonMontage = nullptr;
	CurrentThirdPersonMontage = nullptr;
}

float UGASMontageReplicationComponent::PlayRepMontageOnSlot(FName SlotName, UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection)
{
	float Duration = 0.0f;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return 0.0f;
	}

	if (OwnerPawn->IsLocallyControlled())
	{
		if (UAnimInstance* AnimInstance = GetFirstPersonAnimInstance())
		{
			if (FirstPersonMontage)
			{
				Duration = AnimInstance->Montage_Play(FirstPersonMontage, PlayRate);
				if (Duration > 0.0f && StartSection != NAME_None)
				{
					AnimInstance->Montage_JumpToSection(StartSection, FirstPersonMontage);
				}
				ActiveFirstPersonMontages.Add(SlotName, FirstPersonMontage);
			}
		}
	}

	if (!OwnerPawn->HasAuthority())
	{
		ServerPlayMontageOnSlot(SlotName, FirstPersonMontage, ThirdPersonMontage, PlayRate, StartSection);
	}
	else
	{
		MulticastPlayThirdPersonMontageOnSlot(SlotName, ThirdPersonMontage, PlayRate, StartSection);
	}

	ActiveThirdPersonMontages.Add(SlotName, ThirdPersonMontage);

	return Duration;
}

void UGASMontageReplicationComponent::StopRepMontageOnSlot(FName SlotName, float BlendOutTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	if (OwnerPawn->IsLocallyControlled())
	{
		if (UAnimInstance* AnimInstance = GetFirstPersonAnimInstance())
		{
			if (TObjectPtr<UAnimMontage>* FoundMontage = ActiveFirstPersonMontages.Find(SlotName))
			{
				if (FoundMontage->Get())
				{
					AnimInstance->Montage_Stop(BlendOutTime, FoundMontage->Get());
				}
				ActiveFirstPersonMontages.Remove(SlotName);
			}
		}
	}

	if (!OwnerPawn->HasAuthority())
	{
		ServerStopMontageOnSlot(SlotName, BlendOutTime);
	}
	else
	{
		MulticastStopThirdPersonMontageOnSlot(SlotName, BlendOutTime);
	}

	ActiveThirdPersonMontages.Remove(SlotName);
}

float UGASMontageReplicationComponent::PlayThirdPersonOnlyMontage(UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection)
{
	float Duration = 0.0f;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !ThirdPersonMontage)
	{
		return 0.0f;
	}

	if (UAnimInstance* AnimInstance = GetThirdPersonAnimInstance())
	{
		Duration = AnimInstance->Montage_Play(ThirdPersonMontage, PlayRate);
		if (Duration > 0.0f && StartSection != NAME_None)
		{
			AnimInstance->Montage_JumpToSection(StartSection, ThirdPersonMontage);
		}
	}

	CurrentThirdPersonMontage = ThirdPersonMontage;

	if (!OwnerPawn->HasAuthority())
	{
		ServerPlayThirdPersonOnlyMontage(ThirdPersonMontage, PlayRate, StartSection);
	}
	else
	{
		MulticastPlayThirdPersonMontage(ThirdPersonMontage, PlayRate, StartSection);
	}

	return Duration;
}

void UGASMontageReplicationComponent::StopThirdPersonOnlyMontage(float BlendOutTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetThirdPersonAnimInstance())
	{
		if (CurrentThirdPersonMontage)
		{
			AnimInstance->Montage_Stop(BlendOutTime, CurrentThirdPersonMontage);
		}
	}

	if (!OwnerPawn->HasAuthority())
	{
		ServerStopThirdPersonOnlyMontage(BlendOutTime);
	}
	else
	{
		MulticastStopThirdPersonMontage(BlendOutTime);
	}

	CurrentThirdPersonMontage = nullptr;
}

void UGASMontageReplicationComponent::ServerPlayMontage_Implementation(UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection)
{
	MulticastPlayThirdPersonMontage(ThirdPersonMontage, PlayRate, StartSection);
}

void UGASMontageReplicationComponent::ServerStopMontage_Implementation(float BlendOutTime)
{
	MulticastStopThirdPersonMontage(BlendOutTime);
}

void UGASMontageReplicationComponent::ServerPlayMontageOnSlot_Implementation(FName SlotName, UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection)
{
	MulticastPlayThirdPersonMontageOnSlot(SlotName, ThirdPersonMontage, PlayRate, StartSection);
}

void UGASMontageReplicationComponent::ServerStopMontageOnSlot_Implementation(FName SlotName, float BlendOutTime)
{
	MulticastStopThirdPersonMontageOnSlot(SlotName, BlendOutTime);
}

void UGASMontageReplicationComponent::ServerPlayThirdPersonOnlyMontage_Implementation(UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection)
{
	MulticastPlayThirdPersonMontage(ThirdPersonMontage, PlayRate, StartSection);
}

void UGASMontageReplicationComponent::ServerStopThirdPersonOnlyMontage_Implementation(float BlendOutTime)
{
	MulticastStopThirdPersonMontage(BlendOutTime);
}

void UGASMontageReplicationComponent::MulticastPlayThirdPersonMontage_Implementation(UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	if (OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetThirdPersonAnimInstance())
	{
		if (ThirdPersonMontage)
		{
			float Duration = AnimInstance->Montage_Play(ThirdPersonMontage, PlayRate);
			if (Duration > 0.0f && StartSection != NAME_None)
			{
				AnimInstance->Montage_JumpToSection(StartSection, ThirdPersonMontage);
			}
		}
	}
}

void UGASMontageReplicationComponent::MulticastStopThirdPersonMontage_Implementation(float BlendOutTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	if (OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetThirdPersonAnimInstance())
	{
		if (CurrentThirdPersonMontage)
		{
			AnimInstance->Montage_Stop(BlendOutTime, CurrentThirdPersonMontage);
		}
	}
}

void UGASMontageReplicationComponent::MulticastPlayThirdPersonMontageOnSlot_Implementation(FName SlotName, UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	if (OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetThirdPersonAnimInstance())
	{
		if (ThirdPersonMontage)
		{
			float Duration = AnimInstance->Montage_Play(ThirdPersonMontage, PlayRate);
			if (Duration > 0.0f && StartSection != NAME_None)
			{
				AnimInstance->Montage_JumpToSection(StartSection, ThirdPersonMontage);
			}
			ActiveThirdPersonMontages.Add(SlotName, ThirdPersonMontage);
		}
	}
}

void UGASMontageReplicationComponent::MulticastStopThirdPersonMontageOnSlot_Implementation(FName SlotName, float BlendOutTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	if (OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetThirdPersonAnimInstance())
	{
		if (TObjectPtr<UAnimMontage>* FoundMontage = ActiveThirdPersonMontages.Find(SlotName))
		{
			if (FoundMontage->Get())
			{
				AnimInstance->Montage_Stop(BlendOutTime, FoundMontage->Get());
			}
			ActiveThirdPersonMontages.Remove(SlotName);
		}
	}
}
