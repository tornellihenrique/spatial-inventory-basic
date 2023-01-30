// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SIInteractionWidget.h"

void USIInteractionWidget::UpdateInteractionWidget(USIInteractionComponent* InteractionComponent)
{
	OwningInteractionComponent = InteractionComponent;
	OnUpdateInteractionWidget();
}