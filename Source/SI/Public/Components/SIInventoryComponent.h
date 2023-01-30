// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Library/SIInventoryStructLibrary.h"
#include "SIInventoryComponent.generated.h"

//Called when the inventory is changed and the UI needs an update. 
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

/**Called on server when an item is added to this inventory*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, class USIItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, class USIItem*, Item);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SI_API USIInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class USIItem;

public:
	
	USIInventoryComponent();

protected:
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;

public:

	// API

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FSIItemAddResult TryAddItem(class USIItem* Item, const FInventoryTile TargetTile);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FSIItemAddResult TryAddItemFromClass(TSubclassOf<class USIItem> ItemClass, const FInventoryTile TargetTile, const int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void TryMoveItem(class USIItem* Item, const FInventoryTile TargetTile);

	int32 ConsumeItem(class USIItem* Item);
	int32 ConsumeItem(class USIItem* Item, const int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(class USIItem* Item);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(TSubclassOf <class USIItem> ItemClass, const int32 Quantity = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	USIItem* FindItem(class USIItem* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<USIItem*> FindItems(class USIItem* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	USIItem* FindItemByClass(TSubclassOf<class USIItem> ItemClass) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<USIItem*> FindItemsByClass(TSubclassOf<class USIItem> ItemClass) const;
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE float GetWeightCapacity() const { return WeightCapacity; };

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE int32 GetCapacity() const { return Rows * Columns; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetCurrentWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE TArray<class USIItem*> GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TMap<class USIItem*, FInventoryTile> GetItemsMap() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsRoomAvailable(class USIItem* Item, int32 TopLeftIndex, const bool bCurrentDimensions = true) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryTile IndexToTile(int32 Index) const;
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 TileToIndex(FInventoryTile Tile) const;
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsTileValid(FInventoryTile Tile) const;

	UFUNCTION(Client, Reliable)
	void ClientRefreshInventory();

	// Events

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemRemoved OnItemRemoved;

	// Config

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 1, ClampMax = 20))
	int32 Rows = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 1, ClampMax = 20))
	int32 Columns = 0;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Inventory")
	float WeightCapacity;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Items, Category = "Inventory")
	TArray<class USIItem*> Items;

private:

	UFUNCTION()
	void OnRep_Items();

	UPROPERTY()
	int32 ReplicatedItemsKey;

	// Internal

	FSIItemAddResult TryAddItem_Internal(class USIItem* Item, const int32 TopLeftIndex);

	USIItem* AddItem(class USIItem* Item, const int32 TopLeftIndex, const int32 Quantity);
	
	void TryMoveItem_Internal(class USIItem* Item, const FInventoryTile TargetTile);
		
};
