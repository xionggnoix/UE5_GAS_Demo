// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DZTestCharacter.h"
#include "ShooterWeaponHolder.h"
#include "AbilitySystemInterface.h"
#include "ShooterCharacter.generated.h"

class AShooterWeapon;
class UAbilitySystemComponent;
class UGASMontageReplicationComponent;
class UGameplayAbility;
class UShooterAttributeSet;
class UInputAction;
class UInputComponent;
class UPawnNoiseEmitterComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBulletCountUpdatedDelegate, int32, MagazineSize, int32, Bullets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamagedDelegate, float, LifePercent);

/**
 *  A player controllable first person shooter character
 *  Manages a weapon inventory through the IShooterWeaponHolder interface
 *  Manages health and death
 */
UCLASS(abstract)
class DZTEST_API AShooterCharacter : public ADZTestCharacter, public IShooterWeaponHolder, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
	/** AI Noise emitter component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPawnNoiseEmitterComponent* PawnNoiseEmitter;

	/** 技能系统组件，负责管理 Gameplay Ability、Gameplay Effect 和 Gameplay Tag */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	/** 射击玩法属性集 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities", meta = (AllowPrivateAccess = "true"))
	UShooterAttributeSet* AttributeSet;

	/** GAS Montage 同步组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities", meta = (AllowPrivateAccess = "true"))
	UGASMontageReplicationComponent* MontageReplicationComponent;

protected:

	/** Fire weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** Switch weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SwitchWeaponAction;

	/** Dash input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* DashAction;

	/** Name of the first person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** Name of the third person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** Max distance to use for aim traces */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxAimDistance = 10000.0f;

	/** 角色生成后由服务端默认授予的 Gameplay Ability */
	UPROPERTY(EditDefaultsOnly, Category="Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	/** 直接用于开火输入的 Gameplay Ability，配置后会优先尝试激活它 */
	UPROPERTY(EditDefaultsOnly, Category="Abilities")
	TSubclassOf<UGameplayAbility> FireAbilityClass;

	/** Max HP this character can have */
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHP = 500.0f;

	/** Current HP remaining to this character */
	float CurrentHP = 0.0f;

	/** Team ID for this character*/
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 0;

	/** List of weapons picked up by the character */
	TArray<AShooterWeapon*> OwnedWeapons;

	/** Weapon currently equipped and ready to shoot with */
	TObjectPtr<AShooterWeapon> CurrentWeapon;

	UPROPERTY(EditAnywhere, Category ="Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 5.0f;

	FTimerHandle RespawnTimer;

public:

	/** Bullet count updated delegate */
	FBulletCountUpdatedDelegate OnBulletCountUpdated;

	/** Damaged delegate */
	FDamagedDelegate OnDamaged;

public:

	AShooterCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UShooterAttributeSet* GetAttributeSet() const { return AttributeSet; }

	UGASMontageReplicationComponent* GetMontageReplicationComponent() const { return MontageReplicationComponent; }

	/** 返回当前装备的武器 */
	AShooterWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

protected:

	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopFiring();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSwitchWeapon();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoDash();

public:

	//~Begin IShooterWeaponHolder interface
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) override;
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;
	virtual void AddWeaponRecoil(float Recoil) override;
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;
	virtual FVector GetWeaponTargetLocation() override;
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) override;
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) override;
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) override;
	virtual void OnSemiWeaponRefire() override;
	//~End IShooterWeaponHolder interface

protected:

	AShooterWeapon* FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const;

	/** 初始化 ASC 的 ActorInfo */
	void InitAbilityActorInfo();

	/** 服务端授予默认技能 */
	void GiveDefaultAbilities();

	void Die();

	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	void OnRespawn();
};
