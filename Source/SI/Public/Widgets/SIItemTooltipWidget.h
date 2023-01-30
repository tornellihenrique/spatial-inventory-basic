// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SIItemTooltipWidget.generated.h"

/**
 * 
 */
UCLASS()
class SI_API USIItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip Item", meta = (ExposeOnSpawn = true))
	class USIItem* Item;
	
};
