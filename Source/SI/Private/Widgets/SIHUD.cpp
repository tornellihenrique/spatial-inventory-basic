// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SIHUD.h"

#include "Widgets/SIGameplayWidget.h"
#include "Widgets/SIInventoryWidget.h"

ASIHUD::ASIHUD()
{
}

void ASIHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ASIHUD::CreateGameplayWidget()
{
	if (!GameplayWidget && GameplayWidgetClass && PlayerOwner && !PlayerOwner.IsNull())
	{
		GameplayWidget = CreateWidget<USIGameplayWidget>(PlayerOwner.Get(), GameplayWidgetClass);

		PlayerOwner->bShowMouseCursor = false;
		PlayerOwner->SetInputMode(FInputModeGameOnly());
	}

	if (GameplayWidget && !GameplayWidget->IsInViewport())
	{
		GameplayWidget->AddToViewport();
	}
}

void ASIHUD::ToggleInventoryWidget()
{
	if (!InventoryWidget && InventoryWidgetClass && PlayerOwner && !PlayerOwner.IsNull())
	{
		InventoryWidget = CreateWidget<USIInventoryWidget>(PlayerOwner.Get(), InventoryWidgetClass);
	}

	if (InventoryWidget)
	{
		if (!InventoryWidget->IsInViewport())
		{
			OpenInventoryWidget();
		}
		else
		{
			CloseInventoryWidget();
		}
	}
}

void ASIHUD::OpenInventoryWidget()
{
	if (!InventoryWidget && InventoryWidgetClass && PlayerOwner && !PlayerOwner.IsNull())
	{
		InventoryWidget = CreateWidget<USIInventoryWidget>(PlayerOwner.Get(), InventoryWidgetClass);
	}

	if (InventoryWidget && !InventoryWidget->IsInViewport())
	{
		InventoryWidget->AddToViewport();

		FInputModeGameAndUI UIInput;
		UIInput.SetWidgetToFocus(InventoryWidget->TakeWidget());

		PlayerOwner->bShowMouseCursor = true;
		PlayerOwner->SetInputMode(UIInput);
	}
}

void ASIHUD::CloseInventoryWidget()
{
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromViewport();

		PlayerOwner->bShowMouseCursor = false;
		PlayerOwner->SetInputMode(FInputModeGameOnly());
	}
}
