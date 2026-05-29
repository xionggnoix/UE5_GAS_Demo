// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_PlayRepMontage.generated.h"

class UAnimMontage;
class UAnimInstance;
class UGASMontageReplicationComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRepMontageDelegate);

UCLASS()
class DZTEST_API UAbilityTask_PlayRepMontage : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta=(DisplayName="Play Rep Montage", HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="true"))
	static UAbilityTask_PlayRepMontage* PlayRepMontage(
		UGameplayAbility* OwningAbility,
		UAnimMontage* FirstPersonMontage,
		UAnimMontage* ThirdPersonMontage,
		float PlayRate = 1.0f,
		FName StartSection = NAME_None
	);

	UPROPERTY(BlueprintAssignable)
	FRepMontageDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FRepMontageDelegate OnBlendOut;

	UPROPERTY(BlueprintAssignable)
	FRepMontageDelegate OnInterrupted;

	UPROPERTY(BlueprintAssignable)
	FRepMontageDelegate OnCancelled;

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	virtual void ExternalCancel() override;

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

private:
	UPROPERTY()
	TObjectPtr<UAnimMontage> FirstPersonMontage;

	UPROPERTY()
	TObjectPtr<UAnimMontage> ThirdPersonMontage;

	float PlayRate = 1.0f;
	FName StartSection = NAME_None;

	UPROPERTY()
	TWeakObjectPtr<UGASMontageReplicationComponent> MontageReplicationComponent;

	bool bPlayedMontage = false;

	void StopPlayingMontage();
private:
	
	/** 用于预测回滚的 PredictionKey */
	FPredictionKey PredictionKey;
	
	/** 是否是预测播放 */
	bool bIsPredicted = false;
};
