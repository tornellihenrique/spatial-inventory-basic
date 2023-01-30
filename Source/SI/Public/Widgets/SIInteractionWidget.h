// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SIInteractionWidget.generated.h"

/**
 * 
 */
UCLASS()
class SI_API USIInteractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateInteractionWidget(class USIInteractionComponent* InteractionComponent);

	UFUNCTION(BlueprintImplementableEvent)
	void OnUpdateInteractionWidget();

	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (ExposeOnSpawn))
	class USIInteractionComponent* OwningInteractionComponent;
	
};
