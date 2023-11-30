// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WeaponType.h"
#include "ShooterAnimInstance.generated.h"

UENUM(BlueprintType)
enum class EOffsetState : uint8 {
	EOS_Aiming UMETA(DisplayName = "Aiming"),
	EOS_Hip UMETA(DisplayName = "Hip"),
	EOS_Reloading UMETA(DisplayName = "Reloading"),
	EOS_InAir UMETA(DisplayName = "InAir"),
	EOD_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class SHOOTER_API UShooterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UShooterAnimInstance();

	UFUNCTION(BlueprintCallable)
	void UpdateAnimationProperties(float Deltatime);

	virtual void NativeInitializeAnimation() override;

protected:
	//Handle turning place variables
	void TurnInPlace();

	void Lean(float DeltaTime);

	void SetCanTurn();

	UFUNCTION(BlueprintCallable)
	void ResetCanTurn();

private:
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category=Movement,meta= (AllowPrivateAccess="true"))
	class AShooterCharacter* ShooterCharacter;

	//the speed of the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))	
	float Speed;

	//where or not the character is in the air
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsInAir;

	//where or not the character is moving
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;

	//where or not the character is moving
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Idle_time;

	//offset yaw for strafing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float MovementOffsetYaw;

	//offset yaw the frame before moving
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float LastMovementOffsetYaw;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bFire;

	//Yaw of the character this frame, only updated when standing still or in the air
	FRotator TIPCharacterRotation;

	//Yaw of the character the previous frame
	FRotator TIPCharacterRotationLastFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Turn in place", meta = (AllowPrivateAccess = "true"))
	float RootYawOffset;

	//Rotation curve value this frame
	float RotationCurve;

	//Rotation curve value last frame
	float RotationCurveLastFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn in place", meta = (AllowPrivateAccess = "true"))
	float Pitch;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn in place", meta = (AllowPrivateAccess = "true"))
	bool bReloading;

	//offset state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn in place", meta = (AllowPrivateAccess = "true"))
	EOffsetState OffsetState;

	//CharacterYaw this Frame
	FRotator CharacterRotation;

	//CharacterYaw last Frame
	FRotator CharacterRotationLastFrame;

	//YawDelta use for leaning in the running blendspace
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Lean", meta = (AllowPrivateAccess = "true"))
	float YawDelta;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Crouch", meta = (AllowPrivateAccess = "true"))
	bool bCrouching;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Crouch", meta = (AllowPrivateAccess = "true"))
	bool bEquipping;

	//change the recoil weight based on turning in place and aiming
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float RecoilWeight;
	
	//true when turning in place
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bTurningInPlace;

	//time after stop
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float StopTime;

	float TimeRemainToTurn;
	
	float SpeedLastFrame;

	//Weapon type for the currently equip weapon
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	EWeaponType EquippedWeaponType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn in place", meta = (AllowPrivateAccess = "true"))
	bool bShouldTurnHip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn in place", meta = (AllowPrivateAccess = "true"))
	float HipTurnValue;

	FTimerHandle TurnTimer;

	bool bCanTurn;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Jump", meta = (AllowPrivateAccess = "true"))
	bool bJumpButtonPressed;

	FTimerHandle TurnTimerHandle;

};
