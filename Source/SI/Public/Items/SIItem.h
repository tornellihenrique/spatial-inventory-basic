// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Library/SIInventoryEnumLibrary.h"
#include "UObject/NoExportTypes.h"
#include "SIItem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemModified);

/**
 * 
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class SI_API USIItem : public UObject
{
	GENERATED_BODY()

public:

	USIItem();

	UPROPERTY(Transient)
	class UWorld* World;

protected:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override;
	virtual class UWorld* GetWorld() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	class UStaticMesh* PickupMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	class UMaterialInterface* Thumbnail;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	class UMaterialInterface* ThumbnailRotated;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText UseActionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	ESIItemRarity Rarity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 0.0))
	float Weight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	bool bStackable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 2, EditCondition = bStackable))
	int32 MaxStackSize;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	TSubclassOf<class USIItemTooltipWidget> ItemTooltip;

	UFUNCTION(BlueprintPure, Category = "Item")
	virtual float GetStackWeight() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE bool IsStackFull() const { return Quantity >= MaxStackSize; }

	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE UMaterialInterface* GetThumbnail(const bool bCurrentRotated = true) const { return bCurrentRotated ? bRotated ? ThumbnailRotated : Thumbnail : bNewRotated ? ThumbnailRotated : Thumbnail; }

	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE FIntPoint GetDimensions(const bool bCurrent = true) const { return bCurrent ? bRotated ? FIntPoint(Dimensions.Y, Dimensions.X) : Dimensions : bNewRotated ? FIntPoint(Dimensions.Y, Dimensions.X) : Dimensions; }

	UPROPERTY()
	class USIInventoryComponent* OwningInventory;

	UPROPERTY()
	int32 RepKey;

	virtual void MarkDirtyForReplication();

	UPROPERTY(BlueprintAssignable)
	FOnItemModified OnItemModified;

	virtual void AddedToInventory(class USIInventoryComponent* Inventory);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Item")
	FIntPoint Dimensions;

	UPROPERTY(ReplicatedUsing = OnRep_Quantity, EditAnywhere, Category = "Item", meta = (UIMin = 1, EditCondition = bStackable))
	int32 Quantity;

	UPROPERTY(ReplicatedUsing = OnRep_Rotated, VisibleAnywhere, Category = "Item")
	bool bRotated = false;

	UPROPERTY(ReplicatedUsing = OnRep_NewRotated, VisibleAnywhere, Category = "Item")
	bool bNewRotated = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Item")
	AActor* Owner = nullptr;

public:

	UFUNCTION(BlueprintCallable, Category = "Item")
	void Rotate();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetRotated(const bool bValue);
	
	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE bool GetRotated() const { return bRotated; }
	
	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE bool GetNewRotated() const { return bNewRotated; }

	UFUNCTION()
	void OnRep_Rotated();

	UFUNCTION()
	void OnRep_NewRotated();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetQuantity(const int32 NewQuantity);
	
	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE int32 GetQuantity() const { return Quantity; }

	UFUNCTION()
	void OnRep_Quantity();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetOwner(AActor* NewOwner);
	
};
