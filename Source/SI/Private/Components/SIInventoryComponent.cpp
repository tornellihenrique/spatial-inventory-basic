// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SIInventoryComponent.h"

#include "Engine/ActorChannel.h"
#include "Items/SIItem.h"
#include "Net/UnrealNetwork.h"

USIInventoryComponent::USIInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

void USIInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Items.SetNum(Rows * Columns);
	OnRep_Items();
}

void USIInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USIInventoryComponent, Items);
	
	DOREPLIFETIME(USIInventoryComponent, WeightCapacity);
}

bool USIInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	// Check if the array of items needs to replicate
	if (Channel->KeyNeedsToReplicate(0, ReplicatedItemsKey))
	{
		for (auto& Item : Items)
		{
			if (Item)
			{
				if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
				{
					bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
				}
			}
		}
	}

	return bWroteSomething;
}

FSIItemAddResult USIInventoryComponent::TryAddItem(USIItem* Item, const FInventoryTile TargetTile)
{
	return TryAddItem_Internal(Item, TileToIndex(TargetTile));
}

FSIItemAddResult USIInventoryComponent::TryAddItemFromClass(TSubclassOf<USIItem> ItemClass, const FInventoryTile TargetTile, const int32 Quantity /*=1*/)
{
	USIItem* Item = NewObject<USIItem>(GetOwner(), ItemClass);
	Item->SetQuantity(Quantity);

	return TryAddItem_Internal(Item, TileToIndex(TargetTile));
}

void USIInventoryComponent::TryMoveItem(USIItem* Item, const FInventoryTile TargetTile)
{
	TryMoveItem_Internal(Item, TargetTile);
}

int32 USIInventoryComponent::ConsumeItem(USIItem* Item)
{
	if (Item)
	{
		ConsumeItem(Item, Item->GetQuantity());
	}
	
	return 0;
}

int32 USIInventoryComponent::ConsumeItem(USIItem* Item, const int32 Quantity)
{
	if (GetOwner() && GetOwner()->HasAuthority() && Item)
	{
		const int32 RemoveQuantity = FMath::Min(Quantity, Item->GetQuantity());

		ensure(!(Item->GetQuantity() - RemoveQuantity < 0));

		if (Item->GetQuantity() - RemoveQuantity <= 0)
		{
			RemoveItem(Item);
		}
		else
		{
			Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);
			
			ClientRefreshInventory();
		}

		return RemoveQuantity;
	}

	return 0;
}

bool USIInventoryComponent::RemoveItem(USIItem* Item)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (Item)
		{
			
			for (int32 Index = 0; Index < Items.Num(); Index++)
			{
				if (Items[Index] == Item)
				{
					Items[Index] = nullptr;
				}
			}

			OnRep_Items();

			ReplicatedItemsKey++;

			return true;
		}
	}

	return false;
}

bool USIInventoryComponent::HasItem(TSubclassOf<USIItem> ItemClass, const int32 Quantity) const
{
	TArray<USIItem*> FoundItems = FindItemsByClass(ItemClass);
	int32 TotalQuantity = 0;
	
	if (FoundItems.Num() > 0)
	{
		for (auto& FoundItem : FoundItems)
		{
			TotalQuantity += FoundItem->GetQuantity();
		}
	}
	
	return TotalQuantity >= Quantity;
}

USIItem* USIInventoryComponent::FindItem(USIItem* Item) const
{
	if (Item)
	{
		for (auto& InvItem : Items)
		{
			if (InvItem && InvItem->GetClass() == Item->GetClass())
			{
				return InvItem;
			}
		}
	}
	
	return nullptr;
}

TArray<USIItem*> USIInventoryComponent::FindItems(USIItem* Item) const
{
	TArray<USIItem*> ItemsOfClass;

	for (auto& InvItem : Items)
	{
		if (InvItem && InvItem->GetClass() == Item->GetClass() && !ItemsOfClass.Contains(InvItem))
		{
			ItemsOfClass.Add(InvItem);
		}
	}

	return ItemsOfClass;
}

USIItem* USIInventoryComponent::FindItemByClass(TSubclassOf<USIItem> ItemClass) const
{
	for (auto& InvItem : Items)
	{
		if (InvItem && InvItem->GetClass() == ItemClass)
		{
			return InvItem;
		}
	}
	
	return nullptr;
}

TArray<USIItem*> USIInventoryComponent::FindItemsByClass(TSubclassOf<USIItem> ItemClass) const
{
	TArray<USIItem*> ItemsOfClass;

	for (auto& InvItem : Items)
	{
		if (InvItem && InvItem->GetClass()->IsChildOf(ItemClass) && !ItemsOfClass.Contains(InvItem))
		{
			ItemsOfClass.Add(InvItem);
		}
	}

	return ItemsOfClass;
}

float USIInventoryComponent::GetCurrentWeight() const
{
	float Weight = 0.f;

	for (auto& ItemMap : GetItemsMap())
	{
		if (ItemMap.Key)
		{
			Weight += ItemMap.Key->GetStackWeight();
		}
	}

	return Weight;
}

TMap<USIItem*, FInventoryTile> USIInventoryComponent::GetItemsMap() const
{
	TMap<USIItem*, FInventoryTile> Res;

	for (int32 Index = 0; Index < Items.Num(); Index++)
	{
		USIItem* Item = Items[Index];

		if (Item)
		{
			if (!Res.Contains(Item))
			{
				Res.Add(Item, IndexToTile(Index));
			}
		}
	}

	return Res;
}

void USIInventoryComponent::ClientRefreshInventory_Implementation()
{
	OnInventoryUpdated.Broadcast();
}

void USIInventoryComponent::OnRep_Items()
{
	OnInventoryUpdated.Broadcast();

	for (auto& Item : Items)
	{
		// On the client the world won't be set initially, so it set if not
		if (Item)
		{
			if (!Item->World)
			{
				OnItemAdded.Broadcast(Item);
			
				Item->World = GetWorld();
			}
		}
	}
}

USIItem* USIInventoryComponent::AddItem(USIItem* Item, const int32 TopLeftIndex, const int32 Quantity)
{
	if (Item && GetOwner() && GetOwner()->HasAuthority())
	{
		FInventoryTile Tile = IndexToTile(TopLeftIndex);
		
		USIItem* NewItem = NewObject<USIItem>(GetOwner(), Item->GetClass());
		// NewItem->Rename(nullptr, GetOwner());
		// NewItem->SetOwner(GetOwner());
		NewItem->SetQuantity(Quantity);
		NewItem->SetRotated(Item->GetNewRotated());
		NewItem->OwningInventory = this;
		
		FIntPoint Dimensions = NewItem->GetDimensions();
	
		for (int32 I = Tile.X; I < Tile.X + Dimensions.X; I++)
		{
			for (int32 J = Tile.Y; J < Tile.Y + Dimensions.Y; J++)
			{
				int32 TargetIndex = TileToIndex(FInventoryTile(I, J));

				Items[TargetIndex] = NewItem;
			}
		}
		
		NewItem->AddedToInventory(this);
		NewItem->MarkDirtyForReplication();
		
		OnRep_Items();

		return NewItem;
	}

	return nullptr;
}

void USIInventoryComponent::TryMoveItem_Internal(USIItem* Item, const FInventoryTile TargetTile)
{
	if (Item && GetOwner() && GetOwner()->HasAuthority())
	{
		if (Item && IsTileValid(TargetTile))
		{
			const int32 TopLeftIndex = TileToIndex(TargetTile);

			if (Items.IsValidIndex(TopLeftIndex))
			{
				USIItem* InvItem = Items[TopLeftIndex];
				
				// Check if is stackable and has space
				if (InvItem && InvItem != Item && InvItem->GetClass() == Item->GetClass() && InvItem->bStackable && !InvItem->IsStackFull())
				{
					// Somehow the items quantity went over the max stack size. This shouldn't ever happen
					ensure(Item->GetQuantity() <= Item->MaxStackSize);
					ensure(InvItem->GetQuantity() <= InvItem->MaxStackSize);

					// Find the maximum amount of the item we could take due to weight
					const int32 WeightMaxAddAmount = FMath::IsNearlyZero(Item->Weight)
							? Item->GetQuantity()
							: FMath::FloorToInt((GetWeightCapacity() - GetCurrentWeight()) / Item->Weight);
					const int32 QuantityMaxAddAmount = FMath::Min(InvItem->MaxStackSize - InvItem->GetQuantity(), Item->GetQuantity());
					const int32 AddAmount = FMath::Min(WeightMaxAddAmount, QuantityMaxAddAmount);

					if (AddAmount > 0)
					{
						InvItem->SetQuantity(InvItem->GetQuantity() + AddAmount);
							
						if (AddAmount < Item->GetQuantity())
						{
							Item->SetQuantity(Item->GetQuantity() - AddAmount);
						}
						else
						{
							ConsumeItem(Item, Item->GetQuantity());
						}
					}
				}
				else if (IsRoomAvailable(Item, TopLeftIndex, false))
				{
					ConsumeItem(Item);
					AddItem(Item, TopLeftIndex, Item->GetQuantity());
				}
				else
				{
					FSIItemAddResult AddResult = TryAddItem(Item, TargetTile);
					ConsumeItem(Item, AddResult.AmountGiven);
				}
			}
		}
	}
}

FSIItemAddResult USIInventoryComponent::TryAddItem_Internal(USIItem* Item, const int32 TopLeftIndex)
{
	if (Item && Items.IsValidIndex(TopLeftIndex) && GetOwner() && GetOwner()->HasAuthority())
	{
		USIItem* InvItem = Items[TopLeftIndex];
		
		// Check if is stackable and has space
		if (InvItem && InvItem->GetClass() == Item->GetClass() && InvItem->bStackable && !InvItem->IsStackFull())
		{
			// Somehow the items quantity went over the max stack size. This shouldn't ever happen
			ensure(Item->GetQuantity() <= Item->MaxStackSize);
			ensure(InvItem->GetQuantity() <= InvItem->MaxStackSize);

			// Find the maximum amount of the item we could take due to weight
			const int32 WeightMaxAddAmount = FMath::IsNearlyZero(Item->Weight)
				? Item->GetQuantity()
				: FMath::FloorToInt((GetWeightCapacity() - GetCurrentWeight()) / Item->Weight);
			const int32 QuantityMaxAddAmount = FMath::Min(InvItem->MaxStackSize - InvItem->GetQuantity(), Item->GetQuantity());
			const int32 AddAmount = FMath::Min(WeightMaxAddAmount, QuantityMaxAddAmount);

			if (AddAmount > 0)
			{
				InvItem->SetQuantity(InvItem->GetQuantity() + AddAmount);
							
				if (AddAmount >= Item->GetQuantity())
				{
					ClientRefreshInventory();
					
					return FSIItemAddResult::AddedAll(Item, Item->GetQuantity());
				}
				
				Item->SetQuantity(Item->GetQuantity() - AddAmount);
			}
		}
		else if (IsRoomAvailable(Item, TopLeftIndex, false))
		{
			AddItem(Item, TopLeftIndex, Item->GetQuantity());

			return FSIItemAddResult::AddedAll(Item, Item->GetQuantity());
		}
		
		// Try Add At Another Place
		for (int32 Index = 0; Index < Items.Num(); Index++)
		{
			if (IsRoomAvailable(Item, Index, false))
			{
				const int32 WeightMaxAddAmount = FMath::IsNearlyZero(Item->Weight)
					? Item->GetQuantity()
					: FMath::FloorToInt((GetWeightCapacity() - GetCurrentWeight()) / Item->Weight);
				const int32 QuantityMaxAddAmount = FMath::Min(Item->MaxStackSize, Item->GetQuantity());
				const int32 AddAmount = FMath::Min(WeightMaxAddAmount, QuantityMaxAddAmount);

				if (AddAmount <= 0)
				{
					return FSIItemAddResult::AddedNone(Item->GetQuantity(), FText::Format(FText::FromString("Couldn't add {ItemName} to Inventory. Inventory is full."), Item->DisplayName));
				}

				USIItem* NewItem = AddItem(Item, Index, AddAmount);

				if (AddAmount < Item->GetQuantity())
				{
					Item->SetQuantity(Item->GetQuantity() - AddAmount);
			
					continue;
				}

				return FSIItemAddResult::AddedAll(Item, Item->GetQuantity());
			}
		}

		Item->Rotate();

		for (int32 Index = 0; Index < Items.Num(); Index++)
		{
			if (IsRoomAvailable(Item, Index, false))
			{
				const int32 WeightMaxAddAmount = FMath::IsNearlyZero(Item->Weight)
					? Item->GetQuantity()
					: FMath::FloorToInt((GetWeightCapacity() - GetCurrentWeight()) / Item->Weight);
				const int32 QuantityMaxAddAmount = FMath::Min(Item->MaxStackSize, Item->GetQuantity());
				const int32 AddAmount = FMath::Min(WeightMaxAddAmount, QuantityMaxAddAmount);

				if (AddAmount <= 0)
				{
					return FSIItemAddResult::AddedNone(Item->GetQuantity(), FText::Format(FText::FromString("Couldn't add {ItemName} to Inventory. Inventory is full."), Item->DisplayName));
				}

				USIItem* NewItem = AddItem(Item, Index, AddAmount);

				if (AddAmount < Item->GetQuantity())
				{
					Item->SetQuantity(Item->GetQuantity() - AddAmount);
			
					continue;
				}

				return FSIItemAddResult::AddedAll(Item, Item->GetQuantity());
			}
		}

		Item->Rotate();
	}

	return FSIItemAddResult::AddedNone(-1, FText::FromString(""));
}

bool USIInventoryComponent::IsRoomAvailable(USIItem* Item, int32 TopLeftIndex, const bool bCurrentDimensions/* = true*/) const
{
	if (!Item)
	{
		return false;
	}
	
	FInventoryTile Tile = IndexToTile(TopLeftIndex);
	FIntPoint Dimensions = Item->GetDimensions(bCurrentDimensions);

	for (int32 I = Tile.X; I < Tile.X + Dimensions.X; I++)
	{
		for (int32 J = Tile.Y; J < Tile.Y + Dimensions.Y; J++)
		{
			FInventoryTile TargetTile(I, J);

			if (!IsTileValid(TargetTile))
			{
				return false;
			}
			
			int32 TargetIndex = TileToIndex(TargetTile);
			
			if (!Items.IsValidIndex(TargetIndex))
			{
				return false;
			}
			
			USIItem* TargetItem = Items[TargetIndex];

			if (TargetItem && TargetItem != Item)
			{
				return false;
			}
		}
	}

	return true;
}

FInventoryTile USIInventoryComponent::IndexToTile(int32 Index) const
{
	return FInventoryTile(Index % Columns, Index / Columns);
}

int32 USIInventoryComponent::TileToIndex(FInventoryTile Tile) const
{
	return Tile.X + (Tile.Y * Columns);
}

bool USIInventoryComponent::IsTileValid(FInventoryTile Tile) const
{
	return Tile.X >= 0 && Tile.Y >= 0 && Tile.X < Columns && Tile.Y < Rows;
}

