// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/ShooterAttributeSet.h"
#include "Abilities/GASMontageReplicationComponent.h"
#include "Abilities/GA_Dash.h"
#include "Animation/AnimInstance.h"
#include "DZTest.h"

AShooterCharacter::AShooterCharacter()
{
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Ability System Component"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UShooterAttributeSet>(TEXT("Attribute Set"));

	MontageReplicationComponent = CreateDefaultSubobject<UGASMontageReplicationComponent>(TEXT("Montage Replication Component"));

	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

UAbilitySystemComponent* AShooterCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	OnDamaged.Broadcast(1.0f);
}

void AShooterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	SetOwner(NewController);
	InitAbilityActorInfo();
	GiveDefaultAbilities();
}

void AShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
}

void AShooterCharacter::InitAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AShooterCharacter::GiveDefaultAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
			AbilitySystemComponent->GiveAbility(Spec);
		}
	}

	if (FireAbilityClass)
	{
		FGameplayAbilitySpec FireSpec(FireAbilityClass, 1, INDEX_NONE, this);
		AbilitySystemComponent->GiveAbility(FireSpec);
	}
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoDash);
	}
}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHP -= Damage;

	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoStartFiring()
{
	if (FireAbilityClass && AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilityByClass(FireAbilityClass);
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	if (OwnedWeapons.Num() > 1)
	{
		CurrentWeapon->DeactivateWeapon();

		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			WeaponIndex = 0;
		}
		else
		{
			++WeaponIndex;
		}

		CurrentWeapon = OwnedWeapons[WeaponIndex];
		CurrentWeapon->ActivateWeapon();
	}
}

void AShooterCharacter::DoDash()
{
	UE_LOG(LogTemp, Log, TEXT("DoDash triggered on %s (Role=%d, Local=%d)"),
		*GetName(), (int32)GetLocalRole(), IsLocallyControlled() ? 1 : 0);

	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("DoDash: AbilitySystemComponent is null"));
		return;
	}

	TArray<FGameplayAbilitySpec> Specs = AbilitySystemComponent->GetActivatableAbilities();
	UE_LOG(LogTemp, Log, TEXT("DoDash: ActivatableAbilities count=%d"), Specs.Num());

	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		if (Spec.Ability && Spec.Ability->IsA(UGA_Dash::StaticClass()))
		{
			UE_LOG(LogTemp, Log, TEXT("DoDash: Found Dash ability, TryActivateAbility"));
			const bool bActivated = AbilitySystemComponent->TryActivateAbility(Spec.Handle);
			UE_LOG(LogTemp, Log, TEXT("DoDash: TryActivateAbility result=%d"), bActivated ? 1 : 0);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("DoDash: Dash ability not found. Did you add BP_GA_Dash to DefaultAbilities?"));
}
void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	Weapon->AttachToActor(this, AttachmentRule);
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	USkeletalMeshComponent* FirstPersonMeshComponent = GetFirstPersonMesh();
	UAnimInstance* FirstPersonAnimInstance = FirstPersonMeshComponent ? FirstPersonMeshComponent->GetAnimInstance() : nullptr;

	if (!FirstPersonAnimInstance)
	{
		return;
	}

	FirstPersonAnimInstance->Montage_Play(Montage);
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			OwnedWeapons.Add(AddedWeapon);

			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon();
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
}

void AShooterCharacter::OnSemiWeaponRefire()
{
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	return nullptr;
}

void AShooterCharacter::Die()
{
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}

	GetCharacterMovement()->StopMovementImmediately();
	DisableInput(nullptr);
	OnBulletCountUpdated.Broadcast(0, 0);

	BP_OnDeath();

	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	Destroy();
}
