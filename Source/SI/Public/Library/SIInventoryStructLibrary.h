// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SIInventoryEnumLibrary.h"

#include "SIInventoryStructLibrary.generated.h"

class USIItem;

USTRUCT(BlueprintType)
struct FSIInventoryLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D End;
};

USTRUCT(BlueprintType)
struct FInventoryTile
{
	GENERATED_BODY()

	FInventoryTile() {};
	FInventoryTile(int32 InX, int32 InY) : X(InX), Y(InY) {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Y = 0;
};

USTRUCT(BlueprintType)
struct FSIItemAddResult
{

	GENERATED_BODY()

	FSIItemAddResult() {};
	FSIItemAddResult(int32 InItemQuantity) : AmountToGive(InItemQuantity), AmountGiven(0) {};
	FSIItemAddResult(int32 InItemQuantity, int32 InQuantityAdded) : AmountToGive(InItemQuantity), AmountGiven(InQuantityAdded) {};

	//The amount of the item that we tried to add
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 AmountToGive = 0;

	//The amount of the item that was actually added in the end. Maybe we tried adding 10 items, but only 8 could be added because of capacity/weight
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 AmountGiven = 0;

	//The result
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	ESIItemAddResult Result = ESIItemAddResult::IAR_NoItemsAdded;

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	FText ErrorText = FText::GetEmpty();

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	USIItem* AddedItem = nullptr;

	static FSIItemAddResult AddedNone(const int32 InItemQuantity, const FText& ErrorText)
	{
		FSIItemAddResult AddedNoneResult(InItemQuantity);
		AddedNoneResult.Result = ESIItemAddResult::IAR_NoItemsAdded;
		AddedNoneResult.ErrorText = ErrorText;

		return AddedNoneResult;
	}

	static FSIItemAddResult AddedSome(USIItem* Item, const int32 InItemQuantity, const int32 ActualAmountGiven, const FText& ErrorText)
	{
		FSIItemAddResult AddedSomeResult(InItemQuantity, ActualAmountGiven);

		AddedSomeResult.Result = ESIItemAddResult::IAR_SomeItemsAdded;
		AddedSomeResult.ErrorText = ErrorText;
		AddedSomeResult.AddedItem = Item;

		return AddedSomeResult;
	}

	static FSIItemAddResult AddedAll(USIItem* Item, const int32 InItemQuantity)
	{
		FSIItemAddResult AddAllResult(InItemQuantity, InItemQuantity);

		AddAllResult.Result = ESIItemAddResult::IAR_AllItemsAdded;
		AddAllResult.AddedItem = Item;

		return AddAllResult;
	}
};
