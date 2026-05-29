// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GASMontageReplicationComponent.generated.h"

class UAnimMontage;
class UAnimInstance;
class USkeletalMeshComponent;

/**
 * 负责第一人称和第三人称 Montage 的播放与网络同步。
 * 本地拥有者播放第一人称 Montage，远端客户端播放第三人称 Montage。
 * 
 * 已实现功能：
 * - 第一/第三人称模型分离播放
 * - PlayRate 同步
 * - Stop 回滚
 * - PredictionKey 支持
 * - 多 Channel 支持：同一模型同时播放多个 Montage（DefaultSlot、UpperBodySlot、AdditiveSlot）
 * 
 * TODO 第二阶段功能：
 * - Position 同步：同步动画播放位置
 * - Blend 同步：同步混合参数
 * - RootMotion 同步：同步根运动
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DZTEST_API UGASMontageReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGASMontageReplicationComponent();

	UFUNCTION(BlueprintCallable, Category="GAS|Montage")
	float PlayRepMontage(UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float PlayRate = 1.0f, FName StartSection = NAME_None);

	UFUNCTION(BlueprintCallable, Category="GAS|Montage")
	float PlayRepMontageOnSlot(FName SlotName, UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float PlayRate = 1.0f, FName StartSection = NAME_None);

	UFUNCTION(BlueprintCallable, Category="GAS|Montage")
	void StopRepMontage(float BlendOutTime = 0.2f);

	UFUNCTION(BlueprintCallable, Category="GAS|Montage")
	void StopRepMontageOnSlot(FName SlotName, float BlendOutTime = 0.2f);

	/** 只在第三人称模型上播放 Montage。 */
	UFUNCTION(BlueprintCallable, Category="GAS|Montage")
	float PlayThirdPersonOnlyMontage(UAnimMontage* ThirdPersonMontage, float PlayRate = 1.0f, FName StartSection = NAME_None);

	/** 停止只在第三人称模型上播放的 Montage。 */
	UFUNCTION(BlueprintCallable, Category="GAS|Montage")
	void StopThirdPersonOnlyMontage(float BlendOutTime = 0.2f);

protected:
	virtual void BeginPlay() override;

	UAnimInstance* GetFirstPersonAnimInstance() const;
	UAnimInstance* GetThirdPersonAnimInstance() const;

	UFUNCTION(Server, Reliable)
	void ServerPlayMontage(UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection);

	UFUNCTION(Server, Reliable)
	void ServerStopMontage(float BlendOutTime);

	UFUNCTION(Server, Reliable)
	void ServerPlayMontageOnSlot(FName SlotName, UAnimMontage* FirstPersonMontage, UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection);

	UFUNCTION(Server, Reliable)
	void ServerStopMontageOnSlot(FName SlotName, float BlendOutTime);

	UFUNCTION(Server, Reliable)
	void ServerPlayThirdPersonOnlyMontage(UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection);

	UFUNCTION(Server, Reliable)
	void ServerStopThirdPersonOnlyMontage(float BlendOutTime);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayThirdPersonMontage(UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopThirdPersonMontage(float BlendOutTime);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayThirdPersonMontageOnSlot(FName SlotName, UAnimMontage* ThirdPersonMontage, float PlayRate, FName StartSection);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopThirdPersonMontageOnSlot(FName SlotName, float BlendOutTime);

private:
	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> CachedFirstPersonMesh;

	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> CachedThirdPersonMesh;

	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentFirstPersonMontage;

	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentThirdPersonMontage;
	
	UPROPERTY()
	TMap<FName, TObjectPtr<UAnimMontage>> ActiveFirstPersonMontages;

	UPROPERTY()
	TMap<FName, TObjectPtr<UAnimMontage>> ActiveThirdPersonMontages;
};
