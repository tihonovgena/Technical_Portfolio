// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.


#include "Gameplay/Mechanics/WorldCursorComponent/WorldDraggableComponent.h"
#include "Net/UnrealNetwork.h"


UWorldDraggableComponent::UWorldDraggableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWorldDraggableComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWorldDraggableComponent, AuthoritativeDragState);
}

bool UWorldDraggableComponent::CanAcceptClick() const
{
	return Super::CanAcceptClick() && CanAcceptDrag();
}

bool UWorldDraggableComponent::CanAcceptDrag() const
{
	return bCanDrag && !AuthoritativeDragState.IsActive();
}

void UWorldDraggableComponent::ApplyDragState(const APlayerController* Instigator, EWorldDragState State, 
	const FVector& Location, const FVector& Normal, ECursorExecutionType ExecutionType)
{
	if (State == EWorldDragState::Idle)
		return;
	
	DraggingInstigator = Instigator;
	 
	switch (ExecutionType)
	{
	case ECursorExecutionType::LocalPrediction :
		ApplyLocalDragPrediction(State, Location, Normal);
		break;
	case ECursorExecutionType::ServerAuthority :
		ApplyAuthoritativeDrag(State, Location, Normal);
		break;
	case ECursorExecutionType::Reconciliation :
		ApplyReconcileDrag(State, Location, Normal);
	}
}

void UWorldDraggableComponent::ApplyAuthoritativeDrag(const EWorldDragState State, const FVector& Location, 
	const FVector& Normal)
{
	AuthoritativeDragState =
	{
		State,
		Location,
		Normal
	};
	
	BroadcastDragAction(State, Location, Normal);
	
	if (AuthoritativeDragState.DragState == EWorldDragState::Drop)
	{
		AuthoritativeDragState.Reset();
	}
}

void UWorldDraggableComponent::ApplyLocalDragPrediction(const EWorldDragState State, const FVector& Location, 
	const FVector& Normal)
{
	PredictedDragState =
	{
		State,
		Location,
		Normal
	};
	
	BroadcastDragAction(State, Location, Normal);
}

void UWorldDraggableComponent::ApplyReconcileDrag(const EWorldDragState State, const FVector& Location, 
	const FVector& Normal)
{
	ResolvePredictionCorrection({State, Location, Normal});
}

void UWorldDraggableComponent::ResolvePredictionCorrection(const FWorldDraggableState& NewDraggableState)
{
	FWorldDraggableState NewState = NewDraggableState;
	
	switch (NewState.DragState)
	{
	case EWorldDragState::Drag:
		if (!PredictedDragState.IsActive())
		{
			PredictedDragState = NewState;
			BroadcastDragAction(NewState.DragState, NewState.Location, NewState.Normal);
		}
		break;
			
	case EWorldDragState::Dragging:
		if (!PredictedDragState.IsActive())
		{
			NewState.DragState = EWorldDragState::Drag;
			PredictedDragState = NewState;
			BroadcastDragAction(NewState.DragState, NewState.Location, NewState.Normal);
		}
		break;
			
	case EWorldDragState::Drop:
		if (PredictedDragState.IsActive())
		{
			PredictedDragState = NewState;
			BroadcastDragAction(NewState.DragState, NewState.Location, NewState.Normal);
		}
		DraggingInstigator.Reset();
		PredictedDragState.Reset();
		break;
			
	case EWorldDragState::Idle:
		if (PredictedDragState.IsActive())
		{
			NewState.DragState = EWorldDragState::Drop;
			PredictedDragState = NewState;
			BroadcastDragAction(NewState.DragState, NewState.Location, NewState.Normal);
		}
		DraggingInstigator.Reset();
		PredictedDragState.Reset();
		break;
			
	default:
		break;
	}
}

void UWorldDraggableComponent::ResolveSimulationCorrection(const FWorldDraggableState& NewDraggableState, 
		const FWorldDraggableState& OldDraggableState) const
{
	FWorldDraggableState NewState = NewDraggableState;
	
	switch (NewState.DragState)
	{
	case EWorldDragState::Drag:
		if (!OldDraggableState.IsActive())
		{
			BroadcastDragAction(NewState.DragState, NewState.Location, NewState.Normal);
		}
		break;
			
	case EWorldDragState::Dragging:
		if (!OldDraggableState.IsActive())
		{
			NewState.DragState = EWorldDragState::Drag;
			BroadcastDragAction(NewState.DragState, NewState.Location, NewState.Normal);
		}
		else
		{
			BroadcastDragAction(NewState.DragState, NewState.Location, NewState.Normal);
		}
		break;
			
	case EWorldDragState::Drop:
		if (OldDraggableState.IsActive())
		{
			BroadcastDragAction(NewState.DragState, NewState.Location, NewState.Normal);
		}
		break;
			
	case EWorldDragState::Idle:
		if (OldDraggableState.IsActive())
		{
			NewState.DragState = EWorldDragState::Drop;
			BroadcastDragAction(NewState.DragState, NewState.Location, NewState.Normal);
		}
		break;
			
	default:
		break;
	}
}

void UWorldDraggableComponent::OnRep_DragState(const FWorldDraggableState& OldDragState)
{
	FWorldDraggableState NewState = AuthoritativeDragState;

	if (DraggingInstigator.IsValid())
	{
		// Resolve prediction client correction
		ResolvePredictionCorrection(NewState);
	}
	else
	{
		// Resolve client replication correction without prediction
		ResolveSimulationCorrection(NewState, OldDragState);
	}
}

void UWorldDraggableComponent::BroadcastDragAction(EWorldDragState State, const FVector& Location, 
	const FVector& Normal) const
{
	switch (State)
	{
	case EWorldDragState::Drag:
		OnWorldDrag.Broadcast(GetOwner(), Location, Normal);
		break;
	case EWorldDragState::Dragging:
		OnWorldDragging.Broadcast(GetOwner(), Location, Normal);
		break;
	case EWorldDragState::Drop:
		OnWorldDrop.Broadcast(GetOwner(), Location, Normal);
		break;
	default:
		break;
	}
}
