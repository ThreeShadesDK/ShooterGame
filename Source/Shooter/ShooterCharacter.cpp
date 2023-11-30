// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework\SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/WidgetComponent.h"
#include "Item.h"
#include "Weapon.h"
#include "Enemy.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Shooter.h"
#include "BulletHitInterface.h"
#include "Ammo.h"
#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"

// Sets default values
AShooterCharacter::AShooterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create camera boom (pulls in towards the chatacter if collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 240.f; //the camera follows at this distance behind the chatacter
	CameraBoom->bUsePawnControlRotation = true; //rotate the arm based on the controller 
	CameraBoom->SocketOffset = FVector(0.f, 60.f, 60.f);

	//Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);//attach camera to end of boom
	FollowCamera->bUsePawnControlRotation = false;//Camera does not rotate relative to arm

	//do not rotate along with controller
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;	

	//Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;//character move in direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);//at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	//turn rate
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	HipTurnRate = 90.f;
	HipLookUpRate = 90.f;
	AimingTurnRate = 20.f;
	AimingLookUpRate = 20.f;
	MouseHipTurnRate = 0.6f;
	MouseHipLookUpRate = 0.6f;
	MouseAimingTurnRate = 0.4f;
	MouseAimingLookUpRate = 0.4f;
	//camera value
	CameraDefaultFOV = 0.f;
	CameraZoomedFOV = 30.f;
	CameraCurrentFOV = 0.f;
	ZoomInterpSpeed = 20.f;
	//Crosshair spread factors
	CrosshairSpreadMultiplier = 0.f;
	CrosshairVelocityFactor = 0.f;
	CrosshairInAirFactor = 0.f;
	CrosshairAimFactor = 0.f;
	CrosshairShootingFactor = 0.f;
	//Bullet Fire timer variables
	ShootTimeDuration=0.05f;
	bFiringBullet = false;
	//automatic fire varialbles
	bShouldFire = true;
	bFireButtonPressed = false;
	bShouldTraceForItems = false;

	//Camera interp location variables
	CameraInterpDistance = 250.f;
	CameraInterpElevation = 65.f;

	//Starting ammo amount
	Starting9mmAmmo = 85;
	StartingARAmmo = 120;
	//Combat variable
	CombatState = ECombatState::ECS_Unoccupied;

	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));

	bCrouching = false;

	BaseMovementSpeed = 650.f;
	CrouchMovementSpeed = 300.f;

	StandingCapsuleHalfHeight = 88.f;
	CrouchingCapsuleHalfHeight = 48.f;


	BaseGroundFriction=2.f;
	CrouchingGroundFriction=50.f;

	ActionOffset = 0.f;

	bAimingButtonPressed = false;

	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("Weapon Interpolation Component"));
	WeaponInterpComp->SetupAttachment(GetFollowCamera());

	//create interpolation component
	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 1"));
	InterpComp1->SetupAttachment(GetFollowCamera());

	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 2"));
	InterpComp2->SetupAttachment(GetFollowCamera());

	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 3"));
	InterpComp3->SetupAttachment(GetFollowCamera());

	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 4"));
	InterpComp4->SetupAttachment(GetFollowCamera());

	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 5"));
	InterpComp5->SetupAttachment(GetFollowCamera());

	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 6"));
	InterpComp6->SetupAttachment(GetFollowCamera());

	bShouldPlayEquipSound = true;
	bShouldPlayPickupSound = true;

	PickupSoundResetTime = 0.2f;
	EquipSoundResetTime = 0.2f;

	HighlightedSlot = -1;

	Health = 100.f;
	MaxHealth = 100.f;

	MyCapsuleComponent= CreateDefaultSubobject<UCapsuleComponent>(TEXT("MyCapsuleComponent"));
	MyCapsuleComponent->SetupAttachment(GetRootComponent());

	StunChance = 0.25f;
	bDying = false;

	bSpaceButtonPressed = false;

}


// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (FollowCamera) {
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}
	EquipWeapon(SpawnDefaultWeapon());
	Inventory.Add(EquippedWeapon);
	EquippedWeapon->SetSlotIndex(0);
	EquippedWeapon->DisableCustomDepth();
	EquippedWeapon->DisableGlowMaterial();
	EquippedWeapon->SetCharacer(this);

	InithializeAmmoMap();

	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;

	InitializeInterpLocations();
	
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//SET THE CURRENT field of view
	if (bAiming) {
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	}
	else {
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);

	if (bAiming) {
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else {
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
	//calculate crosshair spread multiplier
	CalculateCrosshairSpread(DeltaTime);

	//check overlap item count,then trace for item
	TraceForItems();

	//interpolate the capsule half height based on crouching/standing
	InterpCapsuleHalfHeight(DeltaTime);
}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward",this,&AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction("Select", IE_Pressed, this, &AShooterCharacter::SelectButtonPressed);

	PlayerInputComponent->BindAction("ReloadButton", IE_Released, this, &AShooterCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AShooterCharacter::CrouchButtonPressed);

	PlayerInputComponent->BindAction("FKey", IE_Released, this, &AShooterCharacter::FKeyPressed);

	PlayerInputComponent->BindAction("1Key", IE_Released, this, &AShooterCharacter::OneKeyPressed);

	PlayerInputComponent->BindAction("2Key", IE_Released, this, &AShooterCharacter::TwoKeyPressed);

	PlayerInputComponent->BindAction("3Key", IE_Released, this, &AShooterCharacter::ThreeKeyPressed);

	PlayerInputComponent->BindAction("4Key", IE_Released, this, &AShooterCharacter::FourKeyPressed);

	PlayerInputComponent->BindAction("5Key", IE_Released, this, &AShooterCharacter::FiveKeyPressed);

}


void AShooterCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && Value != 0.0f)
	{
		//find out which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0,Rotation.Yaw,0 };
		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
		AddMovementInput(Direction, Value*3.0);
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && Value != 0.0f)
	{
		//find out which way is right
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0,Rotation.Yaw,0 };
		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	//
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * -BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::Turn(float value) {
	float TurnScaleFactor;
	if (bAiming) {
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else {
		TurnScaleFactor = MouseHipTurnRate;
	}
	AddControllerYawInput(value * TurnScaleFactor );
}

//mouse lookup
void AShooterCharacter::LookUp(float value) {
	float LookUpScaleFactor;	
	if (bAiming) {
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else {
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(value * LookUpScaleFactor );
}

void AShooterCharacter::FireWeapon() 
{
	//UE_LOG(LogTemp, Warning, TEXT("Fire"));
	if (EquippedWeapon == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (!WeaponHasAmmo()) return;

	//Play fire Sound
	if (EquippedWeapon->GetFireSound()) {
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}

	SendBullet();

	//play start fire animation
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage) {
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}

	StartCrosshairBulletFire();

	EquippedWeapon->DecrementAmmo();

	StartFireTimer();

	if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol) {
		//Start moving slide timer;
		EquippedWeapon->StartSlideTimer();
	}
}

bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FHitResult& OutHitResult, bool& bHit)
{
	FVector OutBeamLocation;
	//get current size of the viewport
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	//get screen location of crosshair
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPositon;
	FVector CrosshairWorldDirection;

	//get world position and direction of crosshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation, CrosshairWorldPositon, CrosshairWorldDirection);

	if (bScreenToWorld)
	{//was deprojection successful
		//UE_LOG(LogTemp, Warning, TEXT("was deprojection successful"));
		FHitResult SCreeenTraceHit;
		const FVector Start = CrosshairWorldPositon;//MuzzleSocketLocation;
		const FVector End = CrosshairWorldPositon + CrosshairWorldDirection * 50000.f;

		//Set beam end point to line trace end
		OutBeamLocation = End;

		//trace outward from crosshairs world location
		GetWorld()->LineTraceSingleByChannel(SCreeenTraceHit, Start, End, ECollisionChannel::ECC_Visibility);
		if (SCreeenTraceHit.bBlockingHit) {//if hit object
			bHit = true;
			OutBeamLocation = SCreeenTraceHit.Location;

			GetWorld()->LineTraceSingleByChannel(OutHitResult, MuzzleSocketLocation, OutBeamLocation, ECollisionChannel::ECC_Visibility);
			if (!OutHitResult.bBlockingHit) {
				OutHitResult = SCreeenTraceHit;
				UE_LOG(LogTemp, Warning, TEXT("FIREST hit object"));
			}
			return true;
		}
	}
	return false;
}

void AShooterCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if (CombatState != ECombatState::ECS_Reloading&& CombatState!= ECombatState::ECS_Equipping&& CombatState != ECombatState::ECS_Stunned) {
		Aim();
	}
}

void AShooterCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;
	StopAiming();
}

void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime) {
	FVector2D WalkSpeedRange{ 0.f,600.f };
	FVector2D VelocityMultiplierRange{ 0.f, 1.f };
	FVector	Velocity = GetVelocity();
	Velocity.Z = 0.f;

	if (GetCharacterMovement()->IsFalling()) { //is in air
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor,2.25f,DeltaTime,5.f);
	}
	else {
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	if (bAiming) {
		CrosshairAimFactor= FMath::FInterpTo(CrosshairAimFactor, -0.5f, DeltaTime, 30.f);
	}
	else {
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
	}

	if (bFiringBullet) {
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.6f, DeltaTime, 60.f);
	}
	else {
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 10.f);
	}
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
	CrosshairSpreadMultiplier = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairAimFactor + CrosshairShootingFactor;
}

float AShooterCharacter::GetCrosshairSpreadMultiplier() const{
	return CrosshairSpreadMultiplier;
}



void AShooterCharacter::StartCrosshairBulletFire() {
	bFiringBullet = true;
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AShooterCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

void AShooterCharacter::FinishCrosshairBulletFire() {
	bFiringBullet = false;
}

void AShooterCharacter::FireButtonPressed() {
	bFireButtonPressed = true;
	FireWeapon();
}

void AShooterCharacter::FireButtonReleased() {
	bFireButtonPressed = false;
}

void AShooterCharacter::StartFireTimer(){
	if (EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_FireTimerInProgress;
	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::AutoFireReset, EquippedWeapon->GetAutoFireRate());
}
void AShooterCharacter::AutoFireReset() {
	if (CombatState == ECombatState::ECS_Stunned) return;

	CombatState = ECombatState::ECS_Unoccupied;
	if (WeaponHasAmmo()) {
		if (bFireButtonPressed && EquippedWeapon &&EquippedWeapon->GetAutomatic()) {
			FireWeapon();
		}
	}
	else {
		ReloadWeapon();
	}
}

bool AShooterCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult) {
	//get current size of the viewport
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	//get screen location of crosshair
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPositon;
	FVector CrosshairWorldDirection;

	//get world position and direction of crosshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation, CrosshairWorldPositon, CrosshairWorldDirection);

	if (bScreenToWorld)
	{//was deprojection successful
		//UE_LOG(LogTemp, Warning, TEXT("was deprojection successful"));
		const FVector Start = CrosshairWorldPositon;//MuzzleSocketLocation;
		const FVector End = CrosshairWorldPositon + CrosshairWorldDirection * 50000.f;

		//trace outward from crosshairs world location
		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (OutHitResult.bBlockingHit) {//if hit object 
			return true;
		}
		
	}
	return false;
}

void AShooterCharacter::IncrementOverlappedItemCount(int8 Amount) 
{
	if (OverlappedItemCount + Amount <= 0) {
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else {
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}

}

void AShooterCharacter::TraceForItems() {
	if (bShouldTraceForItems) {
		FHitResult ItemTraceResult;
		TraceUnderCrosshairs(ItemTraceResult);
		if (ItemTraceResult.bBlockingHit) {
			TraceHitItem = Cast<AItem>(ItemTraceResult.Actor);

			auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
			if (TraceHitWeapon) {
				if (HighlightedSlot == -1) {
					HighlightInventorySlot();
				}
			}
			else {
				//is a slot being highlighted
				if (HighlightedSlot != -1) {
					UnhighlightInventorySlot();
				}
			}

			if (TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterping) {
				TraceHitItem=nullptr;
			}

			if (TraceHitItem && TraceHitItem->GetPickUpWidget()) {
				//UE_LOG(LogTemp, Warning, TEXT("Widget"));
				//show item's pickup widget
				TraceHitItem->GetPickUpWidget()->SetVisibility(true);
				TraceHitItem->EnableCustomDepth();

				if (Inventory.Num() >= INVENTORY_CAPACITY) {
					//FULL
					TraceHitItem->SetbCharacterInventoryFull(true);
				}
				else {
					TraceHitItem->SetbCharacterInventoryFull(false);
				}
			}

			if (TraceHitItemLastFrame) {
				if (TraceHitItem != TraceHitItemLastFrame) {
					TraceHitItemLastFrame->GetPickUpWidget()->SetVisibility(false);
					TraceHitItemLastFrame->DisableCustomDepth();
				}
			}
			
			//store a reference to hititem for next frame
			TraceHitItemLastFrame = TraceHitItem;
		}
		else{
			TraceHitItem = nullptr;
			if (TraceHitItemLastFrame) {
				TraceHitItemLastFrame->GetPickUpWidget()->SetVisibility(false);
				TraceHitItemLastFrame->DisableCustomDepth();
			}
		}
	}
	else {
		if (TraceHitItem) {
			TraceHitItem->GetPickUpWidget()->SetVisibility(false);
			TraceHitItemLastFrame->DisableCustomDepth();
		}
	}
}

AWeapon* AShooterCharacter::SpawnDefaultWeapon() {
	if (DefaultWeaponClass) {
		//spawn the weapon
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}
	return nullptr;
}

void AShooterCharacter::EquipWeapon(AWeapon* WeaponToEquip,bool bSwapping) 
{
	if (WeaponToEquip) {

		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket) {
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}
		if (EquippedWeapon == nullptr) {
			// -1 == no EquippedWeapon yet
			EquipItemDelegate.Broadcast(-1, WeaponToEquip->GetSlotIndex());
		}
		else if(!bSwapping){
			//if (GEngine) GEngine->AddOnScreenDebugMessage(4, -1, FColor::Red, FString::Printf(TEXT("EquippedWeapon->GetSlotIndex():%d"), EquippedWeapon->GetSlotIndex()));
			EquipItemDelegate.Broadcast(EquippedWeapon->GetSlotIndex(), WeaponToEquip->GetSlotIndex());
		}
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AShooterCharacter::DropWeapon() {
	if (EquippedWeapon) {
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);

		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
		//EquippedWeapon = nullptr;
	}
}

void AShooterCharacter::SelectButtonPressed() {
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	//UE_LOG(LogTemp, Warning, TEXT("drop"));
	if (TraceHitItem) {
		if (bAiming) {
			StopAiming();
		}
		TraceHitItem->StartItemCurve(this);
		TraceHitItem = nullptr;
	}
	
}

void AShooterCharacter::SelectButtonReleased() {

}

void AShooterCharacter::SwapWeapon(AWeapon* WeaponToSwap) 
{
	if (Inventory.Num() - 1 >= EquippedWeapon->GetSlotIndex()) {
		Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
		WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
	}
	DropWeapon();
	EquipWeapon(WeaponToSwap,true);
}

/*
FVector AShooterCharacter::GetCameraInterpLocation()
{
	const FVector CameraWorldLocation = FollowCamera->GetComponentLocation();
	const FVector CameraForward = FollowCamera->GetForwardVector();
	return CameraWorldLocation + CameraForward * CameraInterpDistance + FVector(0.f, 0.f, CameraInterpElevation);

}
*/

void AShooterCharacter::GetPickupItem(AItem* Item)
{
	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon) {
		if (Inventory.Num() < INVENTORY_CAPACITY) {
			Weapon->SetSlotIndex(Inventory.Num());
			Inventory.Add(Weapon);
			Weapon->SetItemState(EItemState::EIS_PickedUp);
		}
		else {
			SwapWeapon(Weapon);
		}
	}

	auto Ammo = Cast<AAmmo>(Item);
	if (Ammo)
	{
		PickupAmmo(Ammo);
	}
}

void AShooterCharacter::InithializeAmmoMap() {
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AShooterCharacter::WeaponHasAmmo() {
	if (EquippedWeapon) {
		return EquippedWeapon->GetAmmo() > 0;
	}
	return false;
}

void AShooterCharacter::ReloadButtonPressed() 
{
	ReloadWeapon();
}

void AShooterCharacter::ReloadWeapon() {
	if (EquippedWeapon == nullptr) return;
	if (CombatState == ECombatState::ECS_Unoccupied) {
		 //do we have ammo of the correct type?
		if (bAiming) {
			StopAiming();
		}
		//TODO: Create bool CarryAmmo()
		if (CarryingAmmo() && !EquippedWeapon->ClipIsFull()) { //replace with carring ammo
			//TODO: Create an enum for weapon type
			//TODO: switch on EquippedWeapon->WeaponType
			CombatState = ECombatState::ECS_Reloading;
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance && ReloadMontage) {
				AnimInstance->Montage_Play(ReloadMontage);
				AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
			}
		}
	}
}

void AShooterCharacter::FinishReloading() {
	if (CombatState == ECombatState::ECS_Stunned) {
		return;
	}
	// Update the Combat State
	CombatState = ECombatState::ECS_Unoccupied;

	if (bAimingButtonPressed) {
		Aim();
	}

	if (EquippedWeapon == nullptr) return;

	const auto AmmoType = EquippedWeapon->GetAmmoType();

	//Update the AmmoMap;
	if (AmmoMap.Contains(AmmoType)) {
		//the amount of the ammo the character is carrying of the euippedWeapon type
		int32 CarriedAmmo = AmmoMap[AmmoType];

		//space left in the magazine of equipped weapon
		const int32 MagEmptySpace = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

		if (MagEmptySpace > CarriedAmmo) {
			//reload the magazine with all the ammo we are carrying
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType,CarriedAmmo);
		}
		else {
			//fill the magazine
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}
}

bool AShooterCharacter::CarryingAmmo() 
{
	if (EquippedWeapon == nullptr) {
		return false;
	}
	auto AmmoType = EquippedWeapon->GetAmmoType();

	if (AmmoMap.Contains(AmmoType)) {
		return AmmoMap[AmmoType] > 0;
	}
	return false;
}

void AShooterCharacter::GrabClip() {
	if (EquippedWeapon == nullptr) return;
	if (HandSceneComponent == nullptr) return;
	//index for the clip bone on the equipped weapon
	int32 ClipBoneIndex = EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName());
	//store the transform of the clip
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("Hand_L")));
	HandSceneComponent->SetWorldTransform(ClipTransform);
		
	EquippedWeapon->SetMovingClip(true);

}

void AShooterCharacter::ReleaseClip() {
	EquippedWeapon->SetMovingClip(false);
}

void AShooterCharacter::CrouchButtonPressed() 
{
	if (!GetCharacterMovement()->IsFalling()) {
		bCrouching = !bCrouching;
	}
	if (bCrouching) {
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else {
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
}

void AShooterCharacter::Jump() {
	if (bCrouching)
	{
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
	else {
		ACharacter::Jump();
		bSpaceButtonPressed = true;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		GetWorldTimerManager().SetTimer(JumpTimer, this, &AShooterCharacter::ResetSpaceButtonPressed, 0.2f);

	}
}

void AShooterCharacter::InterpCapsuleHalfHeight(float Deltatime) {
	float TargetCapsuleHalfHeight = 0.f;
	float ActionOffsetTarget ;
	if (bCrouching) {
		TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight;
		ActionOffsetTarget = 4.f;
	}
	else {
		TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;
		ActionOffsetTarget = 0.f;
	}
	const float InterpHalfHeight = FMath::FInterpTo(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), TargetCapsuleHalfHeight, Deltatime,5.f);
	ActionOffsetLastFrame = ActionOffset;
	ActionOffset = FMath::FInterpTo(ActionOffset, ActionOffsetTarget, Deltatime, 5.f);
	float ActionOffsetDelta = ActionOffset - ActionOffsetLastFrame;
	//nagetive if crouching, positive if standing
	float DeltaCapsuleHalfHeight = InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+ ActionOffsetDelta;
	//if (GEngine) GEngine->AddOnScreenDebugMessage(4, -1, FColor::Red, FString::Printf(TEXT("DeltaCapsuleHalfHeight:%f"), DeltaCapsuleHalfHeight));
	FVector MeshOffset{ 0.f,0.f, -DeltaCapsuleHalfHeight };
	GetMesh()->AddLocalOffset(MeshOffset);

	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}

void AShooterCharacter::Aim() {
	bAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void AShooterCharacter::StopAiming() {
	bAiming = false;
	if (!bCrouching) {
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

void AShooterCharacter::PickupAmmo(class AAmmo* Ammo)
{
	//check if ammomap contain ammo's ammotype
	if (AmmoMap.Find(Ammo->GetAmmoType())) {
		//get ammount of ammo in our ammotype
		int32 AmmoCount = AmmoMap[Ammo->GetAmmoType()];
		AmmoCount += Ammo->GetItemCount();
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}
	if (EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType()) {
		//check if the gun is empty
		if (EquippedWeapon->GetAmmo() == 0) {
			ReloadWeapon();
		}

	}
	Ammo->Destroy();
}

FInterpLocation AShooterCharacter::GetInterpLocation(int32 Index) {
	if (Index <= InterpLocations.Num()) {
		return InterpLocations[Index];
	}
	return FInterpLocation();
}

void AShooterCharacter::InitializeInterpLocations()
{
	FInterpLocation WeaponLocation{ WeaponInterpComp,0 };
	InterpLocations.Add(WeaponLocation);

	FInterpLocation InterpLoc1{ InterpComp1,0 };
	InterpLocations.Add(InterpLoc1);

	FInterpLocation InterpLoc2{ InterpComp2,0 };
	InterpLocations.Add(InterpLoc2);

	FInterpLocation InterpLoc3{ InterpComp3,0 };
	InterpLocations.Add(InterpLoc3);

	FInterpLocation InterpLoc4{ InterpComp4,0 };
	InterpLocations.Add(InterpLoc4);

	FInterpLocation InterpLoc5{ InterpComp5,0 };
	InterpLocations.Add(InterpLoc5);

	FInterpLocation InterpLoc6{ InterpComp6,0 };
	InterpLocations.Add(InterpLoc6);
}

int AShooterCharacter::GetInterpLocationIndex() 
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;
	for (int32 i = 1; i < InterpLocations.Num(); i++) {
		if (InterpLocations[i].ItemCount < LowestCount) {
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}
	return LowestIndex;
}

void AShooterCharacter::IncrementInterpLocItemCount(int32 Index, int32 Amount) 
{
	if (Amount < -1||Amount>1) {
		return;
	}
	if (InterpLocations.Num() >= Index) {
		InterpLocations[Index].ItemCount += Amount;
	}
}

void AShooterCharacter::ResetPickupSoundTimer() {
	bShouldPlayPickupSound = true;
}

void AShooterCharacter::ResetEquipSoundTimer() {
	bShouldPlayEquipSound = true;
}


void AShooterCharacter::StartPickupSoundTimer() {
	bShouldPlayPickupSound = false;
	GetWorldTimerManager().SetTimer(PickupSoundTimer, this, &AShooterCharacter::ResetPickupSoundTimer, PickupSoundResetTime);
}
void AShooterCharacter::StartEquipSoundTimer() {
	bShouldPlayEquipSound = false;
	GetWorldTimerManager().SetTimer(EquipSoundTimer, this, &AShooterCharacter::ResetEquipSoundTimer, EquipSoundResetTime);
}

void AShooterCharacter::FKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 0) return;
	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 0);
}

void AShooterCharacter::OneKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 1) return;
	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 1);
}

void AShooterCharacter::TwoKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 2) return;
	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 2);
}

void AShooterCharacter::ThreeKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 3) return;
	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 3);
}

void AShooterCharacter::FourKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 4) return;
	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 4);
}

void AShooterCharacter::FiveKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 5) return;
	ExchangeInventoryItem(EquippedWeapon->GetSlotIndex(), 5);
}

void AShooterCharacter::ExchangeInventoryItem(int32 CurrentItemIndex,int32 NewItemIndex) {
	if (CurrentItemIndex == NewItemIndex || NewItemIndex>=Inventory.Num()|| CombatState!=ECombatState::ECS_Unoccupied) {
		return;
	}
	if (bAiming) {
		StopAiming();
	}
	auto OldEquippedWeapon = EquippedWeapon;
	auto NewWeapon = Cast<AWeapon>(Inventory[NewItemIndex]);
	EquipWeapon(NewWeapon);
	OldEquippedWeapon->SetItemState(EItemState::EIS_PickedUp);
	NewWeapon->SetItemState(EItemState::EIS_Equipped);
	CombatState = ECombatState::ECS_Equipping;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && EquipMontage) {
		AnimInstance->Montage_Play(EquipMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Equip"));
	}
	NewWeapon->PlayEquipSound(true);
}

void  AShooterCharacter::FinishEquipping() {
	if (CombatState == ECombatState::ECS_Stunned) return;
	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimingButtonPressed) {
		Aim();
	}
}


int32 AShooterCharacter::GetEmptyInventorySlot() {
	for (int32 i = 0; i < Inventory.Num(); i++) {
		if (Inventory[i] == nullptr) {
			return i;
		}
	}
	if (Inventory.Num() < INVENTORY_CAPACITY) {
		return Inventory.Num();
	}
	return -1;
}

void AShooterCharacter::HighlightInventorySlot() {
	int32 EmptySlot = GetEmptyInventorySlot();
	HighlightIconDelegate.Broadcast(EmptySlot, true);
	HighlightedSlot = EmptySlot;
}


void AShooterCharacter::UnhighlightInventorySlot() {
	HighlightIconDelegate.Broadcast(HighlightedSlot, false);
	HighlightedSlot = -1;
}

void AShooterCharacter::Stun()
{
	if (Health <= 0.f) return;
	CombatState = ECombatState::ECS_Stunned;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage) {
		AnimInstance->Montage_Play(HitReactMontage);
		//AnimInstance->Montage_JumpToSection()
	}
}

EPhysicalSurface AShooterCharacter::GetSurfaceType()
{
	FHitResult HitResult;
	const FVector Start = GetActorLocation();
	const FVector End = Start + FVector(0.f, 0.f, -400.f);
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, QueryParams);
	return UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());
}

void AShooterCharacter::SendBullet()
{
	const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");
	//play muzzle flash
	if (BarrelSocket) {
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());

		if (EquippedWeapon->GetMuzzleFlash()) {
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleFlash(), SocketTransform);
		}
		FHitResult BeamHitResult;
		bool bHit = false;
		bool bBeamEnd = GetBeamEndLocation(SocketTransform.GetLocation(), BeamHitResult, bHit);
		if (bBeamEnd) {
			//Does hit actor implement bulletHitInterface
			if (BeamHitResult.Actor.IsValid()) {
				IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResult.Actor.Get());
				if (BulletHitInterface) {
					BulletHitInterface->BulletHit_Implementation(BeamHitResult,this,GetController());
					AEnemy* HitEnemy = Cast<AEnemy>(BeamHitResult.Actor.Get());
					if (HitEnemy) {
						int32 Damage;
						if (BeamHitResult.BoneName.ToString() == HitEnemy->GetHeadBone()) {
							Damage = EquippedWeapon->GetHeadShotDamage();
							UGameplayStatics::ApplyDamage(BeamHitResult.Actor.Get(),
								Damage,
								GetController(),
								this,
								UDamageType::StaticClass());
							HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location,true);
						}
						else {
							Damage = EquippedWeapon->GetDamage();
							UGameplayStatics::ApplyDamage(BeamHitResult.Actor.Get(),
								Damage,
								GetController(),
								this,
								UDamageType::StaticClass());
							HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, false);
						}
						

					}
				}
				else {
					//default
					if (ImpactParticles && bHit) {
						UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamHitResult.Location);
					}
					if (BeamParticles) {
						UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);
						if (Beam) {
							Beam->SetVectorParameter(FName("Target"), BeamHitResult.Location);
						}

					}
				}
			}
		}
	}
}

void AShooterCharacter::EndStun()
{
	CombatState = ECombatState::ECS_Unoccupied;
	if (bAiming) {
		Aim();
	}
}


float AShooterCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{

	if (Health - DamageAmount <= 0.f) {
		Health = 0.f;
		Die();
		for (TObjectIterator<AEnemyController> It; It; ++It) {
			It->GetBlackboardComponent()->SetValueAsBool(FName(TEXT("CharacterDead")), true);
		}
		//auto EnemyController = Cast<AEnemyController>(EventInstigator);
		//if (EnemyController) {
			//EnemyController->GetBlackboardComponent()->SetValueAsBool(FName(TEXT("CharacterDead")), true);
		//}
	}

	else {
		Health -= DamageAmount;
	}
	return DamageAmount;
}

void AShooterCharacter::Die()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC) {
		DisableInput(PC);
	}
	if (bDying) return;
	bDying = true;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage) {
		AnimInstance->Montage_Play(DeathMontage);
	}
}

void AShooterCharacter::FinishDeath()
{
	GetMesh()->bPauseAnims = true;
}

void AShooterCharacter::ResetSpaceButtonPressed() {
	bSpaceButtonPressed = false;
}
