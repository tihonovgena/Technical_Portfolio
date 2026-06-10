// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.


#include "LostSignal/Public/Game/UIManager/UIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "LostSignal/Public/Game/UIManager/PrimaryLayoutWidget.h"

void UUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

UPrimaryLayoutWidget* UUIManagerSubsystem::InitPrimaryLayout(const TSubclassOf<UPrimaryLayoutWidget> Class)
{
	RemovePrimaryLayout();
	PrimaryLayout = CreateWidget<UPrimaryLayoutWidget>(GetLocalPlayer()->GetPlayerController(GetWorld()), Class);
	PrimaryLayout->AddToViewport();
	return PrimaryLayout;
}

void UUIManagerSubsystem::RemovePrimaryLayout()
{
	if (PrimaryLayout)
	{
		PrimaryLayout->RemoveFromParent();
		PrimaryLayout = nullptr;
	}
}

UPrimaryLayoutWidget* UUIManagerSubsystem::GetPrimaryLayout()
{
	return PrimaryLayout;
}
