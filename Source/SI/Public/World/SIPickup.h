// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SIPickup.generated.h"

UCLASS(ClassGroup = (Items), Blueprintable, Abstract)
class SI_API ASIPickup : public AActor
{
	GENERATED_BODY()
	
public:

	ASIPickup();

	void InitializePickup(const TSubclassOf<class USIItem> ItemClass, const int32 Quantity);
	void InitializePickup(class USIItem* TargetItem, const int32 Quantity);

	UFUNCTION(BlueprintImplementableEvent)
	void AlignWithGround();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	class USIItem* ItemTemplate;

	UFUNCTION(BlueprintCallable)
	void OnTakePickup(class ASICharacter* Taker);

protected:

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, ReplicatedUsing = OnRep_Item)
	class USIItem* Item;

	UFUNCTION()
	void OnRep_Item();

	UFUNCTION()
	void OnItemModified();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* PickupMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class USIInteractionComponent* InteractionComponent;

};
