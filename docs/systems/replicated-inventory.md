# Replicated Inventory

A server-authoritative inventory component using replicated slots and replicated UObject item instances.

![Inventory prototype](../../screenshots/Image_Inventory_1.png)

## Status

- **Current status:** Base inventory implementation for prototype use.
- **Main purpose:** Support data-driven item instances in replicated inventory slots.
- **Best files to review first:**
  - [InventoryComponent.cpp](../../code/InventorySystem/InventoryComponent.cpp)
  - [InventoryItemInstance.cpp](../../code/InventorySystem/InventoryItemInstance.cpp)

## Problem

The project needed a basic inventory system where item definitions are data-driven, while runtime item instances can exist inside inventory slots.

This first version was implemented for storage-style inventory. The devlog later separates this from the physical inventory direction.

## Constraints

- Inventory changes must be server-authoritative.
- Slot updates must replicate to clients.
- Runtime item instances need to replicate as subobjects.
- The system should be simple enough to support fast gameplay prototyping.

## Architecture

```text
Actor
└── UInventoryComponent
    ├── replicated InventorySlots
    ├── authority-only add/remove/swap operations
    └── replicated UInventoryItemInstance subobjects

FInventorySlot
└── UInventoryItemInstance
    └── UInventoryItemDefinition
```

## Networking / Authority Model

All mutating operations are guarded by authority checks:

- `TryAddItem`
- `TryAddItemToSlot`
- `TryRemoveItemFromSlot`
- `TrySwapItems`

The component uses:

- `SetIsReplicatedByDefault(true)`
- `bReplicateUsingRegisteredSubObjectList = true`
- `DOREPLIFETIME(UInventoryComponent, InventorySlots)`
- `AddReplicatedSubObject(Item)`
- `RemoveReplicatedSubObject(Item)`

`UInventoryItemInstance` replicates its `ItemDefinition`.

Clients react to replicated slot changes through `OnRep_InventorySlots`.

## Implementation Notes

- The component initializes slot count only on authority.
- Item operations return `bool`, so gameplay code can reject invalid inventory actions cleanly.
- `OnInventoryUpdated` is broadcast both after authority-side changes and after replicated slot updates.
- `GetAllItems` returns runtime item instances from non-empty slots.

## Trade-offs

This is intentionally a simple storage inventory. The devlog notes that this inventory did not match the desired physical gameplay feeling, so a separate physical inventory direction was prototyped afterwards.

## Known Limitations

- No stacking logic is shown in the selected source.
- No item move validation beyond slot validity and occupancy is shown.
- No UI binding code is included here.
- The physical inventory direction is not represented as a complete system case yet.

## Next Steps

- Add explicit item operation requests from client to server if player-driven inventory manipulation becomes part of the final gameplay.
- Add item stack / quantity support if required by design.
- Split storage inventory and physical inventory into clearly separate systems if both remain in the project.

