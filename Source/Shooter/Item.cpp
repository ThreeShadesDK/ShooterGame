// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "ShooterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Curves/CurveVector.h"


// Sets default values
AItem::AItem() :
	ItemName(FString("Default")),
	ItemCount(0),
	ItemRarity(EItemRarity::EIR_Common),
	ItemState(EItemState::EIS_PickUp),
	//Item interp variables
	ItemInterpStartLocation(FVector(0.f)),
	ZCurveTime(0.7f),
	CameraTargetLocation(FVector(0.f)),
	bInterping(false),
	ItemInterpX(0.f),
	ItemInterpY(0.f),
	ItemType(EItemType::EIT_MAX),
	InterpLocIndex(0),
	MaterialIndex(0),
	bCanChangeCustomDepth(true), 
	GlowAmount(300.f),
	FresnelExponent(3.f),
	FresnelReflectFraction(4.f),
	PulseCurveTime(5.f),
	SlotIndex(0),
	bCharacterInventoryFull(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh "));
	SetRootComponent(ItemMesh);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ItemMesh);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	PickUpWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickUpWidget->SetupAttachment(GetRootComponent());

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());


}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	
	//Hide PickUp Widget
	if (PickUpWidget) {
		PickUpWidget->SetVisibility(false);
	}

	SetActiveStars();
	
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AItem::OnSphereEndOverlap);

	SetItemProperties(ItemState);

	//
	InitializeCustomDepth(); 

	if (ItemState == EItemState::EIS_PickUp)
	{
		StartPulseTimer();
		UpdatePulse();
	}
	
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ItemInterp(DeltaTime);

	UpdatePulse();

}


void AItem::OnConstruction(const FTransform& Transform)
{
	//Path to the Item Rarity Data Table
	FString RarityTablePath(TEXT("DataTable'/Game/_Game/DataTable/NItemRarityDataTable.NItemRarityDataTable'"));
	UDataTable* RarityTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *RarityTablePath));
	if (RarityTableObject)
	{
		FItemRarityTable* RarityRow = nullptr;
		switch (ItemRarity) {
		case EItemRarity::EIR_Damaged:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Damaged"), TEXT(""));
			break;
		case EItemRarity::EIR_Common:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Common"), TEXT(""));
			break;
		case EItemRarity::EIR_Uncommon:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Uncommon"), TEXT(""));
			break;
		case EItemRarity::EIR_Rare:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Rare"), TEXT(""));
			break;
		case EItemRarity::EIR_Legendary:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Legendary"), TEXT(""));
			break;
		}
		if (RarityRow) {
			GlowColor = RarityRow->GlowColor;
			LightColor = RarityRow->LightColor;
			DarkColor = RarityRow->DarkColor;
			NumberOfStars = RarityRow->NumberOfStars;
			IconBackground = RarityRow->IconBackGround;
			if (GetItemMesh()) {
				GetItemMesh()->SetCustomDepthStencilValue(RarityRow->CustomDepthStencil);
			}
		}
	}
	//if (MaterialInstance) {
		//DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
		//DynamicMaterialInstance->SetVectorParameterValue(TEXT("FresnelColor"), GlowColor);
		//ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);
		//EnableGlowMaterial();
	//}
}

void AItem::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult) 
{
	if (OtherActor) {
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter) {
			ShooterCharacter->IncrementOverlappedItemCount(1);
		}
	}
}

void AItem::OnSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex) 
{
	if (OtherActor) {
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter) {
			ShooterCharacter->IncrementOverlappedItemCount(-1);
			ShooterCharacter->UnhighlightInventorySlot();
		}
	}
}

void AItem::SetActiveStars() {
	//the index 0 is not used
	for (int i = 0; i <= 5; i++) {
		ActiveStars.Add(false);
	}
	
	switch (ItemRarity) {
	case EItemRarity::EIR_Damaged:
		ActiveStars[1] = true;
		break;
	case EItemRarity::EIR_Common:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		break;
	case EItemRarity::EIR_Uncommon:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		break;
	case EItemRarity::EIR_Rare:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		ActiveStars[4] = true;
		break;
	case EItemRarity::EIR_Legendary:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		ActiveStars[4] = true;
		ActiveStars[5] = true;
		break;
	}

}

void AItem::SetItemProperties(EItemState State) {
	switch (State) {
	case EItemState::EIS_PickUp:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//set area properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		//set collision box properties
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	case EItemState::EIS_Equipped:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//set area properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//set collision box properties
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_Falling:
		//Set mesh properties
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
		ItemMesh->SetEnableGravity(true);
		ItemMesh->SetVisibility(true);
		//set area properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//set collision box properties
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_EquipInterping:
		PickUpWidget->SetVisibility(false);
		//Set mesh properties
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//set area properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//set collision box properties
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_PickedUp:
		PickUpWidget->SetVisibility(false);
		//Set mesh properties
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//set area properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//set collision box properties
		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}

}

void AItem::SetItemState(EItemState State) {
	ItemState = State;
	SetItemProperties(State);
}

void AItem::StartItemCurve(AShooterCharacter* Char) {
	Character = Char;

	InterpLocIndex = Character->GetInterpLocationIndex();
	Character->IncrementInterpLocItemCount(InterpLocIndex, 1);

	PlayPickUpSound();

	ItemInterpStartLocation = GetActorLocation();
	bInterping = true;
	SetItemState(EItemState::EIS_EquipInterping);
	GetWorldTimerManager().SetTimer(ItemInterpTimer, this, &AItem::FinishInterping, ZCurveTime);
	GetWorldTimerManager().ClearTimer(PulseTimer);

	//get initial yaw 
	const float CameraRotationYaw = Character->GetFollowCamera()->GetComponentRotation().Yaw;
	const float ItemRotationYaw = GetActorRotation().Yaw;

	InterpInitialYawOffset = ItemRotationYaw - CameraRotationYaw;

	bCanChangeCustomDepth = false;


}

void AItem::FinishInterping() {
	bInterping = false;
	if (Character) {
		Character->IncrementInterpLocItemCount(InterpLocIndex, -1);
		PlayEquipSound(false);
		Character->GetPickupItem(this);
		Character->UnhighlightInventorySlot();
	}
	//Set scale back to normal
	SetActorScale3D(FVector(1.f));

	DisableGlowMaterial();

	bCanChangeCustomDepth = true;
	DisableCustomDepth();
}

void AItem::ItemInterp(float DeltaTime) {
	if (bInterping) {
		//UE_LOG(LogTemp, Warning, TEXT("bInterping is true"));
		if (Character && ItemZCurve) {
			//UE_LOG(LogTemp, Warning, TEXT("star interp"));
			//Elapsed time since we started ItemInterpTimer
			const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
			//get curve value corresponding to elapsedtime
			const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);

			FVector ItemLocation = ItemInterpStartLocation;
			//get location infront of the camera
			const FVector CameraInterpLocation = GetInterpLocation();

			//Vector from Item to Camera Interp Location, X and Y are zeroed out
			const FVector ItemToCamera = FVector(0.f, 0.f, (CameraInterpLocation - ItemLocation).Z);
			//Scale factor to multiply with Curve Value
			const float DeltaZ = ItemToCamera.Size();

			const FVector CurrentLocation = GetActorLocation();
			const float InterpXValue = FMath::FInterpTo(CurrentLocation.X,CameraInterpLocation.X,DeltaTime,30.f);
			const float InterpYValue = FMath::FInterpTo(CurrentLocation.Y, CameraInterpLocation.Y, DeltaTime, 30.f);

			ItemLocation.X = InterpXValue;
			ItemLocation.Y = InterpYValue;

			ItemLocation.Z += CurveValue * DeltaZ;
			SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);

			const FRotator CameraRotation = Character->GetFollowCamera()->GetComponentRotation();
			FRotator ItemRotation{ 0.f,CameraRotation.Yaw + InterpInitialYawOffset, 0.f };

			SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);
			if (ItemScaleCurve) {
				const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
				SetActorScale3D(FVector(ScaleCurveValue, ScaleCurveValue, ScaleCurveValue));
			}
			
		}
	}
}

FVector AItem::GetInterpLocation() {
	if (Character == nullptr) return FVector(0.f);
	switch (ItemType) 
	{
	case EItemType::EIT_Ammo:
		return Character->GetInterpLocation(InterpLocIndex).SceneComponent->GetComponentLocation();
		break;
	case EItemType::EIT_Weapon:
		return Character->GetInterpLocation(0).SceneComponent->GetComponentLocation();
		break;
	}
	return FVector();
}


void AItem::PlayPickUpSound() {
	if (Character) {
		if (Character->ShouldPlayPickupSound()) {
			Character->StartPickupSoundTimer();
			if (PickupSound) {
				UGameplayStatics::PlaySound2D(this, PickupSound);
			}
		}
	}
}
void AItem::PlayEquipSound(bool bForcePlaySound) {
	if (Character) {
		if (bForcePlaySound) {
			if (PickupSound) {
				UGameplayStatics::PlaySound2D(this, EquipSound);
			}
		}
		else if (Character->ShouldPlayEquipSound()) {
			Character->StartEquipSoundTimer();
			if (PickupSound) {
				UGameplayStatics::PlaySound2D(this, EquipSound);
			}
		}
	}
}


void AItem::EnableCustomDepth() {
	if (bCanChangeCustomDepth) {
		ItemMesh->SetRenderCustomDepth(true);
	}

}

void AItem::DisableCustomDepth() {
	if (bCanChangeCustomDepth) {
		ItemMesh->SetRenderCustomDepth(false);
	}
}

void AItem::InitializeCustomDepth()
{
	DisableCustomDepth();
}

void AItem::EnableGlowMaterial() {
	if (DynamicMaterialInstance) {
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"),1);
	}
}

void AItem::DisableGlowMaterial() {
	if (DynamicMaterialInstance) {
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0);
	}
}

void AItem::ResetPulseTimer() {
	StartPulseTimer();
}

void AItem::StartPulseTimer() {
	if (ItemState == EItemState::EIS_PickUp) {
		GetWorldTimerManager().SetTimer(PulseTimer, this, &AItem::ResetPulseTimer,PulseCurveTime);
	}
}

void AItem::UpdatePulse() {
	//float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(PulseTimer);
	float ElapsedTime;
	FVector CurveVectorValue;

	switch(ItemState) 
	{
	case EItemState::EIS_PickUp:
		if (PulseCurve) {
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(PulseTimer);
			CurveVectorValue = PulseCurve->GetVectorValue(ElapsedTime);
		}
		break;
	case EItemState::EIS_EquipInterping:
		if (InterpPulseCurve) {
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
			CurveVectorValue = InterpPulseCurve->GetVectorValue(ElapsedTime);
		}
		break;
	}
	if (DynamicMaterialInstance) {
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowAmount"), CurveVectorValue.X * GlowAmount);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelExponent"), CurveVectorValue.Y*FresnelExponent);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelReflectFraction"), CurveVectorValue.Z*FresnelReflectFraction);
		//float OutValue;
		//DynamicMaterialInstance->GetScalarParameterValue(TEXT("FresnelReflectFraction"), OutValue);
		//if (GEngine&& ItemName=="Assault Rifle") GEngine->AddOnScreenDebugMessage(4, -1, FColor::Red, FString::Printf(TEXT("FresnelReflectFraction:%f"), OutValue));
	}
}