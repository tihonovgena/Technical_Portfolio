// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.


#include "Gameplay/Mechanics/WorldCursorComponent/WorldEnhancedCursorComponent.h"

#include "Gameplay/Mechanics/WorldCursorComponent/WorldDraggableInterface.h"
#include "Gameplay/Mechanics/WorldCursorComponent/WorldDraggableComponent.h"

UWorldEnhancedCursorComponent::UWorldEnhancedCursorComponent()
{
}

void UWorldEnhancedCursorComponent::InitializeComponent()
{
	Super::InitializeComponent();

	checkf(DragUpdateRate > 0.f, TEXT("DragUpdateRate must be greater than 0"));
}


void UWorldEnhancedCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
												  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsCursorActive())
	{
		FVector Location = GetCursorLocation();
		FVector Normal = GetCursorNormal();

		if (IsNetMode(NM_Client))
		{
			UpdateDraggingPrediction(Location, Normal);
		}

		TimeSinceLastDragUpdate += DeltaTime;
		const float UpdateInterval = 1.f / DragUpdateRate;
		if (TimeSinceLastDragUpdate >= UpdateInterval)
		{
			TimeSinceLastDragUpdate = 0.f;
			Server_UpdateDraggingLocation(++LocalDraggingId, Location, Normal);
		}
	}
}

void UWorldEnhancedCursorComponent::OnCursorActionExecuted(uint32 ActionId, EWorldCursorAction Action, 
	const FWorldCursorState& State, ECursorExecutionType ExecutionType)
{
	switch (ExecutionType)
	{
	case ECursorExecutionType::LocalPrediction :
		ApplyLocalAction(ActionId, Action, State);
		break;
	case ECursorExecutionType::ServerAuthority :
		ApplyAuthorityAction(ActionId, Action, State);
		break;
	case ECursorExecutionType::Reconciliation :
		ApplyReconcileAction(ActionId, Action, State);
		break;
	}
}

void UWorldEnhancedCursorComponent::Client_ReconcileDrag_Implementation(uint32 ActionId, const FWorldDragState& State)
{
	if (!IsNetMode(NM_Client))
		return;
	
	if (IsAlreadyProcessed(ActionId))
		return;
	
	if (!State.HasValidActor())
	{
		RollbackLocalDrag();
		return;
	}
	
	UWorldDraggableComponent* PredictedDraggable = GetDraggableComponent(PredictedDragState.GetActor());
	UWorldDraggableComponent* ServerDraggable = GetDraggableComponent(State.GetActor());

	if (ServerDraggable)
	{
		if (PredictedDraggable != ServerDraggable)
		{
			RollbackLocalDrag();
		}
		
		ServerDraggable->ApplyDragState(GetPlayerController(), State.DragState,
				State.GetLocation(), State.GetNormal(), ECursorExecutionType::Reconciliation);
		PredictedDragState = State;
	}
	else if (PredictedDraggable)
	{
		RollbackLocalDrag();
	}
}

void UWorldEnhancedCursorComponent::Client_ReconcileDrop_Implementation(uint32 ActionId, const FWorldDragState& State)
{
	if (!IsNetMode(NM_Client))
		return;
	
	if (IsAlreadyProcessed(ActionId)) 
		return;
	
	PredictedDragState.Reset();
	LastDragLocation = FVector::ZeroVector;
}

void UWorldEnhancedCursorComponent::Server_UpdateDraggingLocation_Implementation(uint32 ActionId, 
	FVector Location, FVector Normal)
{
	// TODO Dragging could be validated before proceed.
	
	if (ActionId <= ProcessedServerDraggingId)
		return;
	
	if (!GetPlayerController() || !GetPlayerController()->HasAuthority())
		return;
	
	if (!AuthoritativeDragState.IsActive())
		return;
	
	if (Location.Equals(LastDragLocation, DragLocationTolerance))
		return;
	
	UWorldDraggableComponent* DraggableComponent = GetDraggableComponent(AuthoritativeDragState.GetActor());
	if (!DraggableComponent)
		return;
	
	
	AuthoritativeDragState.DragState = EWorldDragState::Dragging;
	AuthoritativeDragState.CursorState.Location = Location;
	AuthoritativeDragState.CursorState.Normal = Normal;
	
	DraggableComponent->ApplyDragState(GetPlayerController(), EWorldDragState::Dragging, 
		Location, Normal, ECursorExecutionType::ServerAuthority);
	LastDragLocation = Location;
	
	Client_ReconcileDragging(ActionId, AuthoritativeDragState);
}


void UWorldEnhancedCursorComponent::UpdateDraggingPrediction(const FVector& Location, const FVector& Normal)
{
	if (!PredictedDragState.IsActive())
		return;
	
	if (Location.Equals(LastDragLocation, DragLocationTolerance))
		return;
	
	UWorldDraggableComponent* DraggableComponent = GetDraggableComponent(PredictedDragState.GetActor());
	if (!DraggableComponent)
		return;
	
	DraggableComponent->ApplyDragState(GetPlayerController(), EWorldDragState::Dragging, 
		Location, Normal, ECursorExecutionType::LocalPrediction);
	LastDragLocation = Location;
}

void UWorldEnhancedCursorComponent::Client_ReconcileDragging_Implementation(uint32 ActionId, 
	const FWorldDragState& State)
{
	if (!IsNetMode(NM_Client))
		return;
		
	if (ActionId <= ProcessedServerDraggingId)
		return;
	
	ProcessedServerDraggingId = ActionId;
	
	if (!State.HasValidActor())
	{
		RollbackLocalDrag();
		return;
	}
	
	UWorldDraggableComponent* PredictedDraggable = GetDraggableComponent(PredictedDragState.GetActor());
	UWorldDraggableComponent* ServerDraggable = GetDraggableComponent(State.GetActor());

	if (ServerDraggable)
	{
		if (PredictedDraggable != ServerDraggable)
		{
			RollbackLocalDrag();
		}
		
		ServerDraggable->ApplyDragState(GetPlayerController(), State.DragState,
				State.GetLocation(), State.GetNormal(), ECursorExecutionType::Reconciliation);
		PredictedDragState = State;
	}
	else if (PredictedDraggable)
	{
		RollbackLocalDrag();
	}
	
}

UWorldDraggableComponent* UWorldEnhancedCursorComponent::GetDraggableComponent(const AActor* Actor)
{
	if (!Actor || !Actor->Implements<UWorldDraggableInterface>())
		return nullptr;
	
	UWorldDraggableComponent* Component = IWorldDraggableInterface::Execute_GetDraggableComponent(Actor);
	ensureMsgf(Component, TEXT("%s Has draggable interface but doesn't have draggable component"),
		*GetNameSafe(Actor));
	
	return Component;
}

bool UWorldEnhancedCursorComponent::CanUpdateCursorHover() const
{
	return Super::CanUpdateCursorHover() && (!PredictedDragState.IsActive() && !AuthoritativeDragState.IsActive());
}

void UWorldEnhancedCursorComponent::RollbackLocalDrag()
{
	if (!PredictedDragState.HasValidActor())
		return;
	
	if (PredictedDragState.IsActive())
	{
		UWorldDraggableComponent* DraggableComponent = GetDraggableComponent(PredictedDragState.GetActor());
		if (!DraggableComponent)
			return;

		DraggableComponent->ApplyDragState(GetPlayerController(), EWorldDragState::Drop, 
			PredictedDragState.GetLocation(), PredictedDragState.GetNormal(), ECursorExecutionType::Reconciliation);
	}

	PredictedDragState.Reset();
	TimeSinceLastDragUpdate = 0.f;
}

void UWorldEnhancedCursorComponent::ApplyLocalAction(uint32 ActionId, EWorldCursorAction Action, const FWorldCursorState& State)
{
	switch (Action)
	{
	case EWorldCursorAction::Click :
		HandleLocalPredictionCLick(ActionId, State);
		break;
		
	case EWorldCursorAction::Release :
		HandleLocalPredictionRelease(ActionId, State);
		break;
		
		// TODO Should handle hover and unhover?
	default:
		break;
	}
}

void UWorldEnhancedCursorComponent::ApplyAuthorityAction(uint32 ActionId, EWorldCursorAction Action, const FWorldCursorState& State)
{
	switch (Action)
	{
	case EWorldCursorAction::Click :
		HandleAuthoritativeClick(ActionId, State);
		break;
		
	case EWorldCursorAction::Release :
		HandleAuthoritativeRelease(ActionId, State);
		break;
		
		// TODO Should handle hover and unhover?
	default:
		break;
	}
}

void UWorldEnhancedCursorComponent::ApplyReconcileAction(uint32 ActionId, EWorldCursorAction Action, const FWorldCursorState& State)
{
	switch (Action)
	{
	case EWorldCursorAction::Click:
		HandleReconcileCLick(ActionId, State);
		break;
		
	case EWorldCursorAction::Release:
		HandleReconcileRelease(ActionId, State);	
		break;
	
	default:
		break;
	}
}

void UWorldEnhancedCursorComponent::HandleAuthoritativeClick(uint32 ActionId, const FWorldCursorState& State)
{
	if (!State.HasValidActor())
	{
		Client_ReconcileDrag(false, {});
	}
	
	UWorldDraggableComponent* DraggableComponent = GetDraggableComponent(State.Actor.Get());
	if (!DraggableComponent || !DraggableComponent->CanAcceptDrag())
	{
		Client_ReconcileDrag(false, {});
		return;
	}
	
	if (AuthoritativeDragState.HasValidActor())
	{
		Client_ReconcileDrag(false, AuthoritativeDragState);
		return;
	}
	
	AuthoritativeDragState = 
	{
		State.Actor.Get(),
		State.Location,
		State.Normal,
		EWorldDragState::Drag
	};
	
	DraggableComponent->ApplyDragState(GetPlayerController(), EWorldDragState::Drag, 
		State.Location, State.Normal, ECursorExecutionType::ServerAuthority);

	Client_ReconcileDrag(true, AuthoritativeDragState);
}

void UWorldEnhancedCursorComponent::HandleAuthoritativeRelease(uint32 ActionId, const FWorldCursorState& State)
{
	if (!AuthoritativeDragState.IsActive())
	{
		Client_ReconcileDrop(ActionId, {});
		return;
	}
	
	UWorldDraggableComponent* DraggableComponent = GetDraggableComponent(State.Actor.Get());
	if (!DraggableComponent)
	{
		Client_ReconcileDrop(ActionId, {});
		return;
	}
	
	AuthoritativeDragState.DragState = EWorldDragState::Drop;
	AuthoritativeDragState.CursorState = State;
	DraggableComponent->ApplyDragState(GetPlayerController(), EWorldDragState::Drop, State.Location, 
		State.Normal, ECursorExecutionType::ServerAuthority);
	
	Client_ReconcileDrop(ActionId, AuthoritativeDragState);
	AuthoritativeDragState.Reset();
	LastDragLocation = FVector::ZeroVector;
}

void UWorldEnhancedCursorComponent::HandleLocalPredictionCLick(uint32 ActionId, const FWorldCursorState& State)
{
	if (!State.HasValidActor())
		return;

	UWorldDraggableComponent* DraggableComponent = GetDraggableComponent(State.Actor.Get());
	if (!DraggableComponent || !DraggableComponent->CanAcceptDrag())
		return;

	if (PredictedDragState.IsActive())
	{
		return;
	}
	
	PredictedDragState = 
	{
		State.Actor.Get(),
		State.Location,
		State.Normal,
		EWorldDragState::Drag,
	};
	
	DraggableComponent->ApplyDragState(GetPlayerController(), EWorldDragState::Drag, 
		State.Location, State.Normal, ECursorExecutionType::LocalPrediction);
	
	TimeSinceLastDragUpdate = 0.f;
}

void UWorldEnhancedCursorComponent::HandleLocalPredictionRelease(uint32 ActionId, const FWorldCursorState& State)
{
	if (PredictedDragState.IsActive())
	{
		UWorldDraggableComponent* DraggableComponent = GetDraggableComponent(State.Actor.Get());
		if (DraggableComponent)
		{
			DraggableComponent->ApplyDragState(GetPlayerController(), EWorldDragState::Drop, State.Location, 
			State.Normal, ECursorExecutionType::LocalPrediction);
		}
		
		PredictedDragState.DragState = EWorldDragState::Drop;
		PredictedDragState.CursorState = State;
	}
	
	TimeSinceLastDragUpdate = 0.f;
	LastDragLocation = FVector::ZeroVector;
}

void UWorldEnhancedCursorComponent::HandleReconcileCLick(uint32 ActionId, const FWorldCursorState& State)
{
	FWorldDragState DragState
	{
		State.Actor.Get(),
		State.Location,
		State.Normal,
		EWorldDragState::Drag,
	};
	Client_ReconcileDrag_Implementation(ActionId, DragState);
}

void UWorldEnhancedCursorComponent::HandleReconcileRelease(uint32 ActionId, const FWorldCursorState& State)
{
	FWorldDragState DragState
	{
		State.Actor.Get(),
		State.Location,
		State.Normal,
		EWorldDragState::Drop,
	};
	Client_ReconcileDrop_Implementation(ActionId, DragState);
}
