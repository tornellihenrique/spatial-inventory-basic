// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Library/SIInventoryStructLibrary.h"
#include "SICharacter.generated.h"

USTRUCT(BlueprintType)
struct FSIInteractionData
{
	GENERATED_BODY()

	FSIInteractionData()
	{
		ViewedInteractionComponent = nullptr;
		LastInteractionCheckTime = 0.f;
		bInteractHeld = false;
	}

	UPROPERTY(BlueprintReadOnly)
	class USIInteractionComponent* ViewedInteractionComponent;

	UPROPERTY(BlueprintReadOnly)
	float LastInteractionCheckTime;

	UPROPERTY(BlueprintReadOnly)
	bool bInteractHeld;
};

UCLASS(config=Game)
class ASICharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	class USIInventoryComponent* InventoryComponent;

public:
	
	ASICharacter();

protected:

	virtual void Restart() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;

	// Input
	
protected:

	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);

	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	void ToggleInventory();
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Inventory

public:

	UFUNCTION(BlueprintCallable, Category = "Looting")
	void LootItem(class USIItem* ItemToGive, USIInventoryComponent* TargetInventory, const FInventoryTile TargetTile);

	UFUNCTION(Server, Reliable)
	void ServerLootItem(class USIItem* ItemToLoot, USIInventoryComponent* TargetInventory, const FInventoryTile TargetTile);

	UFUNCTION(BlueprintCallable, Category = "Items")
	void MoveItem(class USIItem* ItemToMove, USIInventoryComponent* TargetInventory, const FInventoryTile TargetTile);

	UFUNCTION(Server, Reliable)
	void ServerMoveItem(class USIItem* ItemToMove, USIInventoryComponent* TargetInventory, const FInventoryTile TargetTile);

	UFUNCTION(BlueprintCallable, Category = "Items")
	void DropItem(class USIItem* Item, const int32 Quantity);

	UFUNCTION(Server, Reliable)
	void ServerDropItem(class USIItem* Item, const int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Items")
	void RotateItem(class USIItem* Item);
	
	UFUNCTION(Server, Reliable)
	void ServerRotateItem(class USIItem* Item);

	UPROPERTY(EditDefaultsOnly, Category = "Item")
	TSubclassOf<class ASIPickup> PickupClass;
	
	// Interaction

public:

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckFrequency;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckDistance;

	void PerformInteractionCheck();

	void CouldntFindInteractable();
	void FoundNewInteractable(AActor* Actor, class USIInteractionComponent* Interactable);

	void BeginInteract();
	void EndInteract();

	UFUNCTION(Server, Reliable)
	void ServerBeginInteract();

	UFUNCTION(Server, Reliable)
	void ServerEndInteract();

	void Interact();

	UPROPERTY(BlueprintReadOnly)
	FSIInteractionData InteractionData;

	FORCEINLINE class USIInteractionComponent* GetInteractable() const { return InteractionData.ViewedInteractionComponent; }

	FTimerHandle TimerHandle_Interact;

	FORCEINLINE bool IsInteracting() const { return GetWorldTimerManager().IsTimerActive(TimerHandle_Interact); };
	FORCEINLINE float GetRemainingInteractTime() const { return GetWorldTimerManager().GetTimerRemaining(TimerHandle_Interact); };

	UFUNCTION(BlueprintCallable)
	void SetCanInteract(bool Value);

	UFUNCTION(Server, Reliable)
	void ServerSetCanInteract(bool Value);

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool GetCanInteract() const { return bCanInteract; }

protected:

	UPROPERTY(BlueprintReadOnly)
	AActor* CurrentInteractingActor;

	UFUNCTION(BlueprintImplementableEvent)
	void OnFindNewInteractingActor(AActor* NewInteractingActor);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CanInteract)
	bool bCanInteract;

	UFUNCTION()
	void OnRep_CanInteract();

	// Components
	
public:
	
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE class USIInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
};

