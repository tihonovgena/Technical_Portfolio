// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.


#include "Gameplay/InventorySystem/InventoryComponent.h"

#include "Gameplay/InventorySystem/InventoryItemInstance.h"
#include "Net/UnrealNetwork.h"


UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (GetOwner()->HasAuthority())
	{
		InventorySlots.SetNum(NumSlots);
	}
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, InventorySlots);
}

bool UInventoryComponent::TrySwapItems(int32 FirstIndex, int32 SecondIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!InventorySlots.IsValidIndex(FirstIndex) || !InventorySlots.IsValidIndex(SecondIndex)) return false;
	if (FirstIndex == SecondIndex) return true;

	Swap(InventorySlots[FirstIndex].ItemInstance, InventorySlots[SecondIndex].ItemInstance);
	
	OnInventoryUpdated.Broadcast(this);
	return true;
}

bool UInventoryComponent::TryRemoveItemFromSlot(int32 SlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	
	if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex].IsEmpty()) return false;

	RemoveReplicatedSubObject(InventorySlots[SlotIndex].GetItem());
	InventorySlots[SlotIndex].RemoveItem();

	OnInventoryUpdated.Broadcast(this);
	return true;
}

bool UInventoryComponent::TryAddItemToSlot(UInventoryItemInstance* Item, int32 SlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	
	if (Item == nullptr) return false;
	if (!InventorySlots.IsValidIndex(SlotIndex) || !InventorySlots[SlotIndex].IsEmpty()) return false;

	InventorySlots[SlotIndex].AddItem(Item);
	AddReplicatedSubObject(Item);

	OnInventoryUpdated.Broadcast(this);
	return true;
}

bool UInventoryComponent::TryAddItem(UInventoryItemInstance* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	
	for (int32 i = 0; i < InventorySlots.Num(); i++)
	{
		if (InventorySlots[i].IsEmpty())
		{
			return TryAddItemToSlot(Item, i);
		}
	}

	return false;
}

TArray<UInventoryItemInstance*> UInventoryComponent::GetAllItems() const
{
	TArray<UInventoryItemInstance*> Items;
	Items.Reserve(InventorySlots.Num());
	
	for (const FInventorySlot& Slot : InventorySlots)
	{
		if (Slot.IsEmpty()) continue;
		Items.Add(Slot.GetItem());
	}

	return Items;
}

UInventoryItemInstance* UInventoryComponent::GetItemInSlot(int32 SlotIndex) const
{
	if (IsValidSlotIndex(SlotIndex))
	{
		return InventorySlots[SlotIndex].GetItem();
	}

	return nullptr;
}

void UInventoryComponent::OnRep_InventorySlots()
{
	OnInventoryUpdated.Broadcast(this);
}

