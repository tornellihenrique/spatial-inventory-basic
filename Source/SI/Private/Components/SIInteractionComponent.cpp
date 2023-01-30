// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SIInteractionComponent.h"

#include "Player/SICharacter.h"
#include "Widgets/SIInteractionWidget.h"

USIInteractionComponent::USIInteractionComponent()
{
	SetComponentTickEnabled(false);

	InteractionTime = 0.f;
	InteractionDistance = 500.f;
	InteractableNameText = FText::FromString("Interactable Object");
	InteractableActionText = FText::FromString("Interact");
	bAllowMultipleInteractors = true;

	Space = EWidgetSpace::Screen;
	DrawSize = FIntPoint(600, 100);
	bDrawAtDesiredSize = true;

	SetActive(true);
	SetHiddenInGame(true);
}

void USIInteractionComponent::SetInteractableNameText(const FText& NewNameText)
{
	InteractableNameText = NewNameText;
	RefreshWidget();
}

void USIInteractionComponent::SetInteractableActionText(const FText& NewActionText)
{
	InteractableActionText = NewActionText;
	RefreshWidget();
}

void USIInteractionComponent::Deactivate()
{
	Super::Deactivate();

	for (int32 i = Interactors.Num() - 1; i >= 0; --i)
	{
		if (ASICharacter* Interactor = Interactors[i])
		{
			EndFocus(Interactor);
			EndInteract(Interactor);
		}
	}

	Interactors.Empty();
}

bool USIInteractionComponent::CanInteract(ASICharacter* Character) const
{
	const bool bPlayerAlreadyInteracting = !bAllowMultipleInteractors && Interactors.Num() >= 1;
	return !bPlayerAlreadyInteracting && IsActive() && GetOwner() != nullptr && Character != nullptr;
}

void USIInteractionComponent::RefreshWidget()
{
	//Make sure the widget is initialized, and that we are displaying the right values (these may have changed)
	if (USIInteractionWidget* InteractionWidget = Cast<USIInteractionWidget>(GetUserWidgetObject()))
	{
		InteractionWidget->UpdateInteractionWidget(this);
	}
}

void USIInteractionComponent::BeginFocus(ASICharacter* Character)
{
	if (!IsActive() || !GetOwner() || !Character)
	{
		return;
	}

	OnBeginFocus.Broadcast(Character);

	if (GetNetMode() != NM_DedicatedServer)
	{
		SetHiddenInGame(false);

		TArray<UActorComponent*> FoundComponents;
		GetOwner()->GetComponents(UPrimitiveComponent::StaticClass(), FoundComponents);

		for (auto& FoundComponent : FoundComponents)
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(FoundComponent))
			{
				Prim->SetRenderCustomDepth(true);
			}
		}
	}

	RefreshWidget();
}

void USIInteractionComponent::EndFocus(ASICharacter* Character)
{
	OnEndFocus.Broadcast(Character);

	if (GetNetMode() != NM_DedicatedServer)
	{
		SetHiddenInGame(true);

		TArray<UActorComponent*> FoundComponents;
		GetOwner()->GetComponents(UPrimitiveComponent::StaticClass(), FoundComponents);

		for (auto& FoundComponent : FoundComponents)
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(FoundComponent))
			{
				Prim->SetRenderCustomDepth(false);
			}
		}
	}
}

void USIInteractionComponent::BeginInteract(ASICharacter* Character)
{
	if (CanInteract(Character))
	{
		Interactors.AddUnique(Character);
		OnEndInteract.Broadcast(Character);
	}
}

void USIInteractionComponent::EndInteract(ASICharacter* Character)
{
	Interactors.RemoveSingle(Character);
	OnEndInteract.Broadcast(Character);
}

void USIInteractionComponent::Interact(ASICharacter* Character) const
{
	if (CanInteract(Character))
	{
		OnInteract.Broadcast(Character);
	}
}

float USIInteractionComponent::GetInteractPercentage()
{
	if (Interactors.IsValidIndex(0))
	{
		if (ASICharacter* Interactor = Interactors[0])
		{
			if (Interactor && Interactor->IsInteracting())
			{
				return 1.f - FMath::Abs(Interactor->GetRemainingInteractTime() / InteractionTime);
			}
		}
	}
	
	return 0.f;
}
