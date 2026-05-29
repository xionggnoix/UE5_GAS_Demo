// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_FireWeapon.generated.h"

class UAnimMontage;
class AShooterWeapon;

/**
 * 开火 GameplayAbility，第一版先复用现有 ShooterWeapon 的 Projectile 生成逻辑。
 * 通过 AbilityTask_PlayRepMontage 播放同步 Montage。
 */
UCLASS()
class DZTEST_API UGA_FireWeapon : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_FireWeapon();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	/** 服务端执行开火逻辑 */
	void ServerFire();

protected:
	/** 第一人称开火 Montage，如果为空则从当前武器获取 */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	TObjectPtr<UAnimMontage> FirstPersonFireMontage;

	/** 第三人称开火 Montage，如果为空则从当前武器获取 */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	TObjectPtr<UAnimMontage> ThirdPersonFireMontage;

	/** Montage 播放速率 */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	float MontagePlayRate = 1.0f;
	
private:
	/** 用于预测回滚的 PredictionKey */
	FPredictionKey PredictionKey;
	
	/** 是否是预测播放 */
	bool bIsPredicted = false;
};
