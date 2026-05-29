// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterAttributeSet.h"
#include "Net/UnrealNetwork.h"

UShooterAttributeSet::UShooterAttributeSet()
{
	InitHealth(500.0f);
	InitMaxHealth(500.0f);
}

void UShooterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UShooterAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UShooterAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UShooterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UShooterAttributeSet, Health, OldHealth);
}

void UShooterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UShooterAttributeSet, MaxHealth, OldMaxHealth);
}
