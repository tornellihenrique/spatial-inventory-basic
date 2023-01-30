// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/SIItem.h"

#include "Components/SIInventoryComponent.h"
#include "Net/UnrealNetwork.h"

USIItem::USIItem()
{
	DisplayName = FText::FromString("Item");
	UseActionText = FText::FromString("Use");
	Weight = 0.f;
	bStackable = true;
	Quantity = 1;
	MaxStackSize = 2;
	RepKey = 0;
}

void USIItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USIItem, Quantity);
	DOREPLIFETIME(USIItem, bRotated);
	DOREPLIFETIME(USIItem, bNewRotated);
	DOREPLIFETIME(USIItem, Owner);
}

bool USIItem::IsSupportedForNetworking() const
{
	return true;
}

UWorld* USIItem::GetWorld() const
{
	return World;
}

#if WITH_EDITOR
void USIItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	FName ChangedPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// UPROPERTY clamping doesn't support using a variable to clamp so we do in here instead
	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(USIItem, Quantity))
	{
		Quantity = FMath::Clamp(Quantity, 1, bStackable ? MaxStackSize : 1);
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(USIItem, bStackable))
	{
		if (!bStackable)
		{
			Quantity = 1;
		}
	}
}
#endif

float USIItem::GetStackWeight() const
{
	return Quantity * Weight;
}

void USIItem::MarkDirtyForReplication()
{
	// Mark this object for replication
	++RepKey;

	// Mark the array for replication
	if (OwningInventory)
	{
		++OwningInventory->ReplicatedItemsKey;
	}
}

void USIItem::AddedToInventory(USIInventoryComponent* Inventory)
{
}

void USIItem::Rotate()
{
	bNewRotated = !bNewRotated;
	OnRep_NewRotated();

	MarkDirtyForReplication();
}

void USIItem::SetRotated(const bool bValue)
{
	if (bValue != bRotated)
	{
		bRotated = bValue;
		bNewRotated = bValue;
		OnRep_Rotated();

		MarkDirtyForReplication();
	}
}

void USIItem::OnRep_Rotated()
{
	OnItemModified.Broadcast();
}

void USIItem::OnRep_NewRotated()
{
	OnItemModified.Broadcast();
}

void USIItem::SetQuantity(const int32 NewQuantity)
{
	if (NewQuantity != Quantity)
	{
		Quantity = FMath::Clamp(NewQuantity, 0, bStackable ? MaxStackSize : 1);
		OnRep_Quantity();
		
		MarkDirtyForReplication();
	}
}

void USIItem::OnRep_Quantity()
{
	OnItemModified.Broadcast();
}

void USIItem::SetOwner(AActor* NewOwner)
{
	if (NewOwner != Owner)
	{
		Owner = NewOwner;
		
		MarkDirtyForReplication();
	}
}
