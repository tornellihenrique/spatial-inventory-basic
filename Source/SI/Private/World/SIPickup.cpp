// Fill out your copyright notice in the Description page of Project Settings.


#include "World/SIPickup.h"

#include "Components/SIInteractionComponent.h"
#include "Components/SIInventoryComponent.h"
#include "Engine/ActorChannel.h"
#include "Items/SIItem.h"
#include "Library/SIInventoryStructLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/SICharacter.h"
#include "Player/SIPlayerController.h"

ASIPickup::ASIPickup()
{
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>("PickupMesh");
	PickupMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	SetRootComponent(PickupMesh);

	InteractionComponent = CreateDefaultSubobject<USIInteractionComponent>("PickupInteractionComponent");
	InteractionComponent->InteractionTime = 0.f;
	InteractionComponent->InteractionDistance = 500.f;
	InteractionComponent->InteractableNameText = FText::FromString("Pickup");
	InteractionComponent->InteractableActionText = FText::FromString("Take");
	InteractionComponent->OnInteract.AddDynamic(this, &ASIPickup::OnTakePickup);
	InteractionComponent->SetupAttachment(PickupMesh);

	bReplicates = true;
}

void ASIPickup::InitializePickup(const TSubclassOf<USIItem> ItemClass, const int32 Quantity)
{
	if (HasAuthority() && ItemClass && Quantity > 0)
	{
		Item = NewObject<USIItem>(this, ItemClass);
		Item->SetQuantity(Quantity);

		OnRep_Item();

		Item->MarkDirtyForReplication();
	}
}

void ASIPickup::InitializePickup(USIItem* TargetItem, const int32 Quantity)
{
	if (HasAuthority() && TargetItem && Quantity > 0)
	{
		Item = TargetItem;
		Item->Rename(nullptr, this);
		Item->SetQuantity(Quantity);
		Item->OwningInventory = nullptr;

		OnRep_Item();

		Item->MarkDirtyForReplication();
	}
}

void ASIPickup::OnRep_Item()
{
	if (Item)
	{
		PickupMesh->SetStaticMesh(Item->PickupMesh);
		InteractionComponent->InteractableNameText = Item->DisplayName;

		// Clients bind to this delegate in order to refresh the interaction widget if item quantity changes (not takes all)
		Item->OnItemModified.AddDynamic(this, &ASIPickup::OnItemModified);
	}

	InteractionComponent->RefreshWidget();
}

void ASIPickup::OnItemModified()
{
	if (InteractionComponent)
	{
		InteractionComponent->RefreshWidget();
	}
}

void ASIPickup::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && ItemTemplate && bNetStartup)
	{
		InitializePickup(ItemTemplate->GetClass(), ItemTemplate->GetQuantity());
	}

	if (!bNetStartup)
	{
		AlignWithGround();
	}

	if (Item)
	{
		Item->MarkDirtyForReplication();
	}
}

void ASIPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASIPickup, Item);
}

bool ASIPickup::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (Item && Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
	{
		bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
	}

	return bWroteSomething;
}

#if WITH_EDITOR
void ASIPickup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// If a new pickup is selected in the property editor, change the mesh to reflect the new item being selected
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASIPickup, ItemTemplate))
	{
		if (ItemTemplate)
		{
			PickupMesh->SetStaticMesh(ItemTemplate->PickupMesh);
		}
	}
}
#endif

void ASIPickup::OnTakePickup(ASICharacter* Taker)
{
	if (!Taker)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pickup was taken but player was not valid. "));
		return;
	}

	// Not 100% sure Pending kill check is needed but should prevent player from taking a pickup another player has already tried taking
	if (HasAuthority() && !IsPendingKillPending() && Item)
	{
		FSIItemAddResult AddResult;
		
		// for (auto& Inventory : Taker->GetInventoryList()) // TODO: Update for taking all inventories
		if (USIInventoryComponent* Inventory = Taker->GetInventoryComponent())
		{
			AddResult = Inventory->TryAddItem(Item, FInventoryTile());

			if (AddResult.AmountGiven >= Item->GetQuantity())
			{
				Destroy();

				return;
			}
			
			if (AddResult.AmountGiven < Item->GetQuantity())
			{
				Item->SetQuantity(Item->GetQuantity() - AddResult.AmountGiven);
			}
		}

		if (!AddResult.ErrorText.IsEmpty())
		{
			if (ASIPlayerController* PC = Cast<ASIPlayerController>(Taker->GetController()))
			{
				// PC->ClientShowNotification(AddResult.ErrorText); // TODO: Implement Client Notification
			}
		}
	}
}