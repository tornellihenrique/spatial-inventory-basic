// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/SICharacter.h"

#include "SI.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SIInteractionComponent.h"
#include "Components/SIInventoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Items/SIItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/SIPlayerController.h"
#include "Widgets/SIHUD.h"
#include "World/SIPickup.h"

//////////////////////////////////////////////////////////////////////////
// ASICharacter

ASICharacter::ASICharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Inventory
	InventoryComponent = CreateDefaultSubobject<USIInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->Rows = 15;
	InventoryComponent->Columns = 6;

	// Interaction
	InteractionCheckFrequency = 0.f;
	InteractionCheckDistance = 1000.f;
	bCanInteract = true;
}

void ASICharacter::Restart()
{
	Super::Restart();

	if (ASIPlayerController* PC = Cast<ASIPlayerController>(GetController()))
	{
		if (ASIHUD* HUD = Cast<ASIHUD>(PC->GetHUD()))
		{
			HUD->CreateGameplayWidget();
		}
	}
}

void ASICharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Interaction
	if (GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
	{
		PerformInteractionCheck();
	}
}

void ASICharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASICharacter, bCanInteract);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASICharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ASICharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ASICharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &ASICharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &ASICharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ASICharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ASICharacter::TouchStopped);

	// Inventory
	PlayerInputComponent->BindAction("ToggleInventoryAction", IE_Pressed, this, &ASICharacter::ToggleInventory);

	// Interaction
	PlayerInputComponent->BindAction("InteractAction", IE_Pressed, this, &ASICharacter::BeginInteract);
	PlayerInputComponent->BindAction("InteractAction", IE_Released, this, &ASICharacter::EndInteract);
}

void ASICharacter::LootItem(USIItem* ItemToGive, USIInventoryComponent* TargetInventory, const FInventoryTile TargetTile)
{
	if (HasAuthority())
	{
		USIInventoryComponent* ItemInventorySource = ItemToGive->OwningInventory;
		
		if (ItemToGive && ItemInventorySource)
		{
			if (TargetInventory)
			{
				if (ItemInventorySource == TargetInventory)
				{
					return;
				}
			
				if (ItemInventorySource->HasItem(ItemToGive->GetClass(), ItemToGive->GetQuantity()))
				{
					const FSIItemAddResult AddResult = TargetInventory->TryAddItem(ItemToGive, TargetTile);

					if (AddResult.AmountGiven > 0)
					{
						ItemInventorySource->ConsumeItem(ItemToGive, AddResult.AmountGiven);
					}
					else
					{
						if (ASIPlayerController* PC = Cast<ASIPlayerController>(GetController()))
						{
							// PC->ClientShowNotification(AddResult.ErrorText);
						}
					}
				}
			}
			else
			{
				FSIItemAddResult AddResult;
		
				// for (auto& Inventory : GetInventoryList()) // TODO: Update for taking all inventories
				if (USIInventoryComponent* Inventory = InventoryComponent)
				{
					if (Inventory)
					{
						AddResult = Inventory->TryAddItem(ItemToGive, FInventoryTile());

						if (AddResult.AmountGiven >= ItemToGive->GetQuantity())
						{
							ItemInventorySource->ConsumeItem(ItemToGive, AddResult.AmountGiven);

							return;
						}
				
						if (AddResult.AmountGiven < ItemToGive->GetQuantity())
						{
							ItemToGive->SetQuantity(ItemToGive->GetQuantity() - AddResult.AmountGiven);
						}
					}
				}

				if (!AddResult.ErrorText.IsEmpty())
				{
					if (ASIPlayerController* PC = Cast<ASIPlayerController>(GetController()))
					{
						// PC->ClientShowNotification(AddResult.ErrorText);
					}
				}
			}
		}
	}
	else
	{
		ServerLootItem(ItemToGive, TargetInventory, TargetTile);
	}
}

void ASICharacter::ServerLootItem_Implementation(USIItem* ItemToLoot, USIInventoryComponent* TargetInventory, const FInventoryTile TargetTile)
{
	LootItem(ItemToLoot, TargetInventory, TargetTile);
}

void ASICharacter::MoveItem(USIItem* ItemToMove, USIInventoryComponent* TargetInventory, const FInventoryTile TargetTile)
{
	if (HasAuthority())
	{
		if (ItemToMove)
		{
			if (ItemToMove->OwningInventory == TargetInventory)
			{
				TargetInventory->TryMoveItem(ItemToMove, TargetTile);
			}
			else
			{
				LootItem(ItemToMove, TargetInventory, TargetTile);
			}
		}
	}
	else
	{
		ServerMoveItem(ItemToMove, TargetInventory, TargetTile);
	}
}

void ASICharacter::ServerMoveItem_Implementation(USIItem* ItemToMove, USIInventoryComponent* TargetInventory, const FInventoryTile TargetTile)
{
	MoveItem(ItemToMove, TargetInventory, TargetTile);
}

void ASICharacter::DropItem(USIItem* Item, const int32 Quantity)
{
	if (!HasAuthority())
	{
		ServerDropItem(Item, Quantity);
	}
	else
	{
		if (Quantity > 0 && Item && (Item->OwningInventory && Item->OwningInventory->FindItem(Item))) // TODO: Update for taking all inventories
		{
			const int32 ItemQuantity = Item->GetQuantity();
			int32 DroppedQuantity;
			
			DroppedQuantity = Item->OwningInventory->ConsumeItem(Item, Quantity);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.bNoFail = true;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			FVector SpawnLocation = GetActorLocation();
			SpawnLocation.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

			ensure(PickupClass);

			if (ASIPickup* Pickup = GetWorld()->SpawnActor<ASIPickup>(PickupClass, SpawnTransform, SpawnParams))
			{
				if (ItemQuantity != DroppedQuantity)
				{
					Pickup->InitializePickup(Item->GetClass(), DroppedQuantity);
				}
				else
				{
					Pickup->InitializePickup(Item, DroppedQuantity);
				}
			}
		}
	}
}

void ASICharacter::RotateItem(USIItem* Item)
{
	if (HasAuthority())
	{
		Item->Rotate();
	}
	else
	{
		ServerRotateItem(Item);
	}
}

void ASICharacter::ServerRotateItem_Implementation(USIItem* Item)
{
	RotateItem(Item);
}

void ASICharacter::ServerDropItem_Implementation(USIItem* Item, const int32 Quantity)
{
	DropItem(Item, Quantity);
}

void ASICharacter::PerformInteractionCheck()
{
	if (GetController() == nullptr)
	{
		return;
	}

	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	FVector EyesLoc;
	FRotator EyesRot;

	GetController()->GetPlayerViewPoint(EyesLoc, EyesRot);

	FVector TraceStart = EyesLoc;
	FVector TraceEnd = (EyesRot.Vector() * InteractionCheckDistance) + TraceStart;
	FHitResult TraceHit;

	FCollisionQueryParams QueryParams;

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);

	QueryParams.AddIgnoredActors(IgnoredActors);

	if (bCanInteract && GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, COLLISION_INTERACTION, QueryParams))
	{
		if (TraceHit.GetActor())
		{
			if (USIInteractionComponent* InteractionComponent = Cast<USIInteractionComponent>(TraceHit.GetActor()->GetComponentByClass(USIInteractionComponent::StaticClass())))
			{
				float Distance = (TraceStart - TraceHit.ImpactPoint).Size();

				if (InteractionComponent != GetInteractable() && Distance <= InteractionComponent->InteractionDistance)
				{
					FoundNewInteractable(TraceHit.GetActor(), InteractionComponent);
				}
				else if (Distance > InteractionComponent->InteractionDistance && GetInteractable())
				{
					CouldntFindInteractable();
				}

				return;
			}
		}
	}

	CouldntFindInteractable();
}

void ASICharacter::CouldntFindInteractable()
{
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Interact))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Interact);
	}

	if (USIInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->EndFocus(this);

		if (InteractionData.bInteractHeld)
		{
			EndInteract();
		}
	}

	InteractionData.ViewedInteractionComponent = nullptr;

	OnFindNewInteractingActor(nullptr);
	CurrentInteractingActor = nullptr;
}

void ASICharacter::FoundNewInteractable(AActor* Actor, USIInteractionComponent* Interactable)
{
	EndInteract();

	if (USIInteractionComponent* OldInteractable = GetInteractable())
	{
		OldInteractable->EndFocus(this);
	}

	InteractionData.ViewedInteractionComponent = Interactable;
	Interactable->BeginFocus(this);

	OnFindNewInteractingActor(Actor);
	CurrentInteractingActor = Actor;
}

void ASICharacter::BeginInteract()
{
	if (!HasAuthority())
	{
		ServerBeginInteract();
	}

	if (HasAuthority()/* && IsAlive()*/) // TODO: Handle if implement Health System
	{
		PerformInteractionCheck();
	}

	InteractionData.bInteractHeld = true;

	if (USIInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->BeginInteract(this);

		if (FMath::IsNearlyZero(Interactable->InteractionTime))
		{
			Interact();
		}
		else
		{
			GetWorldTimerManager().SetTimer(TimerHandle_Interact, this, &ASICharacter::Interact, Interactable->InteractionTime, false);
		}
	}
}

void ASICharacter::EndInteract()
{
	if (!HasAuthority())
	{
		ServerEndInteract();
	}

	InteractionData.bInteractHeld = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (USIInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->EndInteract(this);
	}
}

void ASICharacter::Interact()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (USIInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->Interact(this);
	}
}

void ASICharacter::SetCanInteract(bool Value)
{
	if (HasAuthority())
	{
		bCanInteract = Value;

		OnRep_CanInteract();
	}
	else
	{
		ServerSetCanInteract(Value);
	}
}

void ASICharacter::OnRep_CanInteract()
{
	if (!bCanInteract)
	{
		CouldntFindInteractable();
	}
}

void ASICharacter::ServerSetCanInteract_Implementation(bool Value)
{
	SetCanInteract(Value);
}

void ASICharacter::ServerEndInteract_Implementation()
{
	EndInteract();
}

void ASICharacter::ServerBeginInteract_Implementation()
{
	BeginInteract();
}

void ASICharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ASICharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ASICharacter::ToggleInventory()
{
	if (ASIPlayerController* PC = Cast<ASIPlayerController>(GetController()))
	{
		if (ASIHUD* HUD = Cast<ASIHUD>(PC->GetHUD()))
		{
			HUD->ToggleInventoryWidget();
		}
	}
}

void ASICharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ASICharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ASICharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASICharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
