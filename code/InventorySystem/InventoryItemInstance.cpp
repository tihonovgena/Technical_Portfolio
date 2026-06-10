// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.


#include "Gameplay/InventorySystem/InventoryItemInstance.h"

#include "Net/UnrealNetwork.h"

void UInventoryItemInstance::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryItemInstance, ItemDefinition);
}

void UInventoryItemInstance::Initialize(UInventoryItemDefinition* Definition)
{
	ItemDefinition = Definition;
}
