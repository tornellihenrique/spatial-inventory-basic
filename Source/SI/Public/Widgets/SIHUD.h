// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SIHUD.generated.h"

/**
 * 
 */
UCLASS()
class SI_API ASIHUD : public AHUD
{
	GENERATED_BODY()

public:

	ASIHUD();

	virtual void BeginPlay() override;
	
	void CreateGameplayWidget();

	// Inventory
	
	void ToggleInventoryWidget();
	void OpenInventoryWidget();
	void CloseInventoryWidget();

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<class USIGameplayWidget> GameplayWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<class USIInventoryWidget> InventoryWidgetClass;

public:

	UPROPERTY(BlueprintReadOnly, Category = "Widgets")
	class USIGameplayWidget* GameplayWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Widgets")
	class USIInventoryWidget* InventoryWidget;
	
};
