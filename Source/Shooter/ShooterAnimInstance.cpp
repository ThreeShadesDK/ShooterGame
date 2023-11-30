// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework\CharacterMovementComponent.h"
#include "Weapon.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"


UShooterAnimInstance::UShooterAnimInstance() :
	Speed(0.f),
	bIsInAir(false),
	bIsAccelerating(false),
	MovementOffsetYaw(0.f),
	LastMovementOffsetYaw(0.f),
	bAiming(false),
	RootYawOffset(0.f),
	Pitch(0.f),
	bReloading(false),
	OffsetState(EOffsetState::EOS_Hip),
	RecoilWeight(1.0f),
	bTurningInPlace(false),
	StopTime(3.f),
	bCanTurn(false),
	bJumpButtonPressed(false)
{	

}

void UShooterAnimInstance::UpdateAnimationProperties(float Deltatime)
{
	if (ShooterCharacter == nullptr) {
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	}
	if (ShooterCharacter) {
		bReloading = ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading;

		bCrouching = ShooterCharacter->GetCrouching();

		bEquipping = ShooterCharacter->GetCombatState() == ECombatState::ECS_Equipping;

		//Get the speed of the character from velocity
		FVector Velocity = ShooterCharacter->GetVelocity();
		Velocity.Z = 0;

		SpeedLastFrame = Speed;
		Speed = Velocity.Size();
		// is the character in the air
		bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

		//if thr character is accelerating
		bIsAccelerating = (ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f);

		bFire = ShooterCharacter->GetFire();

		bJumpButtonPressed = ShooterCharacter->GetJumpButtonPressed();

		if (bIsAccelerating) {
			FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
			FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());
			FRotator MovementOffsetRotation = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
			MovementOffsetYaw = MovementOffsetRotation.Yaw;
		}
		else {
			MovementOffsetYaw = LastMovementOffsetYaw;
		}
		
		//设置bShouldTurnHip与HipTurnValue
		if (MovementOffsetYaw >= -45.f && MovementOffsetYaw <= 45.f) {
			bShouldTurnHip = true;
			HipTurnValue = MovementOffsetYaw;
		}
		else {
			bShouldTurnHip = false;
		}
		if (Speed==0.f) {
			bShouldTurnHip = false;
		}
		//if (GEngine)GEngine->AddOnScreenDebugMessage(1, -1, FColor::Blue, FString::Printf(TEXT("HipTurnValue:%f"), HipTurnValue));
		//if (GEngine)GEngine->AddOnScreenDebugMessage(2, -1, FColor::Blue, FString::Printf(TEXT("MovementOffsetYaw:%f"), MovementOffsetYaw));

		//MovementOffsetYaw重映射范围
		if (MovementOffsetYaw > 180.f) {
			MovementOffsetYaw -= 360.f;
		}
		else if (MovementOffsetYaw < -180.f) {
			MovementOffsetYaw += 360.f;
		}

		if (LastMovementOffsetYaw>120.f&&MovementOffsetYaw < -120.f) {
			MovementOffsetYaw += 360.f;
		}
		else if (LastMovementOffsetYaw < -120.f && MovementOffsetYaw>120.f) {
			MovementOffsetYaw -= 360.f;
		}

		if (ShooterCharacter->GetVelocity().Size() > 0.f) {
			LastMovementOffsetYaw = MovementOffsetYaw;
		}
		
		
	
		//if (GEngine)GEngine->AddOnScreenDebugMessage(1, -1, FColor::Blue, FString::Printf(TEXT("HipTurnValue:%d"), HipTurnValue));
		//if (GEngine)GEngine->AddOnScreenDebugMessage(5, -1, FColor::Blue, FString::Printf(TEXT("LastMovementOffsetYaw:%f"), LastMovementOffsetYaw));
		//UE_LOG(LogTemp, Warning, TEXT("LastMovementOffsetYaw:%f"), LastMovementOffsetYaw);
		bAiming = ShooterCharacter->GetAiming();

		if (bReloading) {
			OffsetState = EOffsetState::EOS_Reloading;
		}
		else if (bIsInAir) {
			OffsetState = EOffsetState::EOS_InAir;
		}
		else if (ShooterCharacter->GetAiming()) {
			OffsetState = EOffsetState::EOS_Aiming;
		}
		else {
			OffsetState = EOffsetState::EOS_Hip;
		}
		//Check if Shooter Characer has a valid Equipped weapon
		if (ShooterCharacter->GetEquippedWeapon()) {
			EquippedWeaponType = ShooterCharacter->GetEquippedWeapon()->GetWeaponType();
		}
	}
	TurnInPlace();


	if (Speed == 0.f&& SpeedLastFrame>0.f) {
		bCanTurn = false;
		//启动计时器
		ResetCanTurn();

	}
	if (!bCanTurn) {
		RootYawOffset = 0.f;
	}
	Lean(Deltatime);

}

void UShooterAnimInstance::NativeInitializeAnimation()
{
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}

void UShooterAnimInstance::TurnInPlace() 
{
	
	if (ShooterCharacter == nullptr) return;

	Pitch = ShooterCharacter->GetBaseAimRotation().Pitch;

	

	if (bIsAccelerating ||Speed > 0.f || OffsetState== EOffsetState::EOS_InAir) {
		RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.f, GetWorld()->GetDeltaSeconds(), 10.f);
		//TIPCharacterRotation = ShooterCharacter->GetActorRotation();
		//TIPCharacterRotationLastFrame = TIPCharacterRotation;
		//do not want to turn in place; character is moving
		RotationCurveLastFrame = 0.f;
		RotationCurve = 0.f;
	}
	else
	{
		TIPCharacterRotationLastFrame = TIPCharacterRotation;
		TIPCharacterRotation = ShooterCharacter->GetActorRotation();
		//TIPCharacterRotation.Yaw = FMath::FInterpTo(TIPCharacterRotation.Yaw, ShooterCharacter->GetActorRotation().Yaw, GetWorld()->GetDeltaSeconds(), 1.f);
		// Change in a frame
		FRotator TIPDelta = UKismetMathLibrary::NormalizedDeltaRotator(TIPCharacterRotation, TIPCharacterRotationLastFrame);
		
		//Root Yaw Offset , updated and clamped to [-180,180]
		float TIPYawDelta = TIPDelta.Yaw;
		RootYawOffset -= TIPYawDelta;

		//1.0 if turning , 0.0 if not
		const float Turning = GetCurveValue(TEXT("Turning"));
		if (Turning == 1.f) {
			bTurningInPlace = true;
			RotationCurveLastFrame = RotationCurve;
			RotationCurve = GetCurveValue(TEXT("Rotation"));
			const float DeltaRotation = RotationCurve - RotationCurveLastFrame;
			if (RootYawOffset > 0) {
				RootYawOffset -= DeltaRotation;
			}
			else {
				RootYawOffset += DeltaRotation;
			}
			const float ABSRootYawOffset = FMath::Abs(RootYawOffset);
			if (ABSRootYawOffset > 90.f) {
				const float YawExcess = ABSRootYawOffset - 90.f;
				RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
			}
		}
		else {
			bTurningInPlace = false;
		}
	}

	if (bTurningInPlace) {
		if (bReloading || bEquipping) {
			RecoilWeight = 1.f;
		}
		else {
			RecoilWeight = 0.1f;
		}
	}
	else { //not turn
		if (bCrouching) {
			if (bReloading|| bEquipping) {
				RecoilWeight = 1.f;
			}
			else {
				RecoilWeight = 0.2f;
			}
		}
		else {
			if (bAiming) {
				RecoilWeight = 0.7f;
			}
			else {
				RecoilWeight = 0.6f;
				if (bReloading) {
					RecoilWeight = 1.f;
				}
			}
		}
	}
}

void UShooterAnimInstance::Lean(float DeltaTime) {
	if (ShooterCharacter == nullptr) return;
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ShooterCharacter->GetActorRotation();
	FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw * 30.f;
	const float Interp = FMath::FInterpTo(YawDelta, Target, DeltaTime, 1.5f);
	YawDelta = FMath::Clamp(Interp, -90.f, 90.f);

}

void UShooterAnimInstance::SetCanTurn()
{
	bCanTurn = true;
}


void UShooterAnimInstance::ResetCanTurn()
{
	ShooterCharacter->GetWorldTimerManager().SetTimer(TurnTimer, this, &UShooterAnimInstance::SetCanTurn, 0.3f);
}
