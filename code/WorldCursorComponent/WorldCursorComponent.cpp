// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.


#include "Gameplay/Mechanics/WorldCursorComponent/WorldCursorComponent.h"

#include "Gameplay/Mechanics/WorldCursorComponent/WorldClickableComponent.h"
#include "Gameplay/Mechanics/WorldCursorComponent/WorldCursorInterface.h"

UWorldCursorComponent::UWorldCursorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bWantsInitializeComponent = true;
}

void UWorldCursorComponent::InitializeComponent()
{
	Super::InitializeComponent();

	PlayerController = GetOwner<APlayerController>();
	check(PlayerController);
}

void UWorldCursorComponent::ActivateCursor()
{
	if (!PlayerController || !PlayerController->IsLocalController())
		return;
	
	SetComponentTickEnabled(true);
	bActivatedCursor = true;
}

void UWorldCursorComponent::DeactivateCursor()
{
	if (!PlayerController || !PlayerController->IsLocalController())
		return;
	
	CursorUnhover();
	TryRelease();
	
	bActivatedCursor = false;
	bGrounded = false;
	SetComponentTickEnabled(false);
	
	LocalCursorState.Reset();
}

void UWorldCursorComponent::TryClick()
{
	if (!bActivatedCursor)
		return;
	
	if (!PlayerController || !PlayerController->IsLocalController())
		return;

	FVector TraceStart, TraceDirection;
	GetTraceStartAndDirection(TraceStart, TraceDirection);
	
	const uint32 ActionId = ++LocalActionId;
	
	if (IsNetMode(NM_Client))
	{
		if (DoClickPrediction(ActionId, TraceStart, TraceDirection))
		{
			Server_CursorClick(ActionId, TraceStart, TraceDirection);
		}
	}
	else
	{
		Server_CursorClick(ActionId, TraceStart, TraceDirection);
	}
}

bool UWorldCursorComponent::DoClickPrediction(uint32 ActionId, const FVector& Start, const FVector& Direction)
{
	if (PredictedClickState.HasValidActor())
		return false;
	
	const FHitResult ClientHit = DoTrace(Start, Direction);
	
	if (!ClientHit.bBlockingHit)
		return false;
	
	PredictedClickState = 
	{
		ClientHit.GetActor(), 
		ClientHit.Location, 
		ClientHit.Normal
	};
	
	UWorldClickableComponent* Clickable = GetClickableComponent(ClientHit.GetActor());
	if (!Clickable || !Clickable->CanAcceptClick())
		return false;
	
	Clickable->ApplyCursorAction(EWorldCursorAction::Click, PredictedClickState.Location,
		PredictedClickState.Normal, ECursorExecutionType::LocalPrediction);
	OnCursorActionExecuted(ActionId, EWorldCursorAction::Click, PredictedClickState, ECursorExecutionType::LocalPrediction);
	return true;
}

void UWorldCursorComponent::Server_CursorClick_Implementation(uint32 ActionId, FVector Start, 
	FVector Direction)
{
	if (!PlayerController) 
		return;

	FHitResult ServerHit = DoTrace(Start, Direction);
	
	if (!ServerHit.bBlockingHit)
	{
		Client_ReconcileCursorClick(ActionId, {});
		return;
	}
	
	if (IsAlreadyProcessed(ActionId) || AuthorityClickState.HasValidActor())
	{
		Client_ReconcileCursorClick(ActionId, {});
		return;
	}
	
	if (AuthorityClickState.HasValidActor())
	{
		Client_ReconcileCursorClick(ActionId, AuthorityClickState);
		return;
	}

	UWorldClickableComponent* Clickable = GetClickableComponent(ServerHit.GetActor());
	if (!Clickable || !Clickable->CanAcceptClick())
	{
		Client_ReconcileCursorClick(ActionId, {});
		return;
	}

	AuthorityClickState = 
	{
		ServerHit.GetActor(), 
		ServerHit.Location, 
		ServerHit.Normal
	};
	
	ProcessedServerActionId = ActionId;

	Clickable->ApplyCursorAction(EWorldCursorAction::Click, AuthorityClickState.Location, 
		AuthorityClickState.Normal, ECursorExecutionType::ServerAuthority);
	OnCursorActionExecuted(ActionId,EWorldCursorAction::Click, AuthorityClickState, ECursorExecutionType::ServerAuthority);
	
	Client_ReconcileCursorClick(ActionId, AuthorityClickState);
}

void UWorldCursorComponent::TryRelease()
{
	if (!bActivatedCursor)
		return;
	
	if (!PlayerController || !PlayerController->IsLocalController())
		return;

	const uint32 ActionId = ++LocalActionId;
	
	FVector TraceStart, TraceDirection;
	GetTraceStartAndDirection(TraceStart, TraceDirection);
	
	if (IsNetMode(NM_Client))
	{
		DoReleasePrediction(ActionId, TraceStart, TraceDirection);
	}
	
	Server_CursorRelease(ActionId, TraceStart, TraceDirection);
}

void UWorldCursorComponent::Server_CursorRelease_Implementation(uint32 ActionId, FVector Start, FVector Direction)
{
	if (!PlayerController || !PlayerController->HasAuthority())
		return;

	if (IsAlreadyProcessed(ActionId) || !AuthorityClickState.HasValidActor())
	{
		Client_ReconcileCursorRelease(ActionId, {});
		return;
	}
	
	UWorldClickableComponent* ServerClickable = GetClickableComponent(AuthorityClickState.Actor.Get());
	if (ServerClickable)
	{
		const FHitResult ServerHit = DoTrace(Start, Direction);
		if (ServerHit.bBlockingHit)
		{
			AuthorityClickState.Location = ServerHit.Location;
			AuthorityClickState.Normal = ServerHit.Normal;
		}
		else
		{
			AuthorityClickState.Location = ProjectLocationToTraceDistance(Start, Direction);
			AuthorityClickState.Normal = FVector::UpVector;
		}
		
		ServerClickable->ApplyCursorAction(EWorldCursorAction::Release, AuthorityClickState.Location,
			AuthorityClickState.Normal, ECursorExecutionType::ServerAuthority);
		OnCursorActionExecuted(ActionId, EWorldCursorAction::Release, AuthorityClickState, ECursorExecutionType::ServerAuthority);
	}
	
	Client_ReconcileCursorRelease(ActionId, AuthorityClickState);
	ProcessedServerActionId = ActionId;
	AuthorityClickState.Reset();
}

void UWorldCursorComponent::RollbackLocalClick(uint32 ActionId)
{
	UWorldClickableComponent* Clickable = GetClickableComponent(PredictedClickState.Actor.Get());
	if (Clickable)
	{
		Clickable->ApplyCursorAction(EWorldCursorAction::Release, PredictedClickState.Location,
			PredictedClickState.Normal, ECursorExecutionType::LocalPrediction);
		OnCursorActionExecuted(ActionId, EWorldCursorAction::Release, PredictedClickState, ECursorExecutionType::LocalPrediction);
	}

	PredictedClickState.Reset();
}

void UWorldCursorComponent::DoReleasePrediction(uint32 ActionId, const FVector& Start, const FVector& Direction)
{
	UWorldClickableComponent* Clickable = GetClickableComponent(PredictedClickState.Actor.Get());
	if (Clickable)
	{
		const FHitResult ClientHit = DoTrace(Start, Direction);
		if (ClientHit.bBlockingHit)
		{
			PredictedClickState.Location = ClientHit.Location;
			PredictedClickState.Normal = ClientHit.Normal;
		}
		else
		{
			PredictedClickState.Location = ProjectLocationToTraceDistance(Start, Direction);
			PredictedClickState.Normal = FVector::UpVector;
		}
		
		Clickable->ApplyCursorAction(EWorldCursorAction::Release, PredictedClickState.Location,
			PredictedClickState.Normal, ECursorExecutionType::LocalPrediction);
		OnCursorActionExecuted(ActionId, EWorldCursorAction::Release, PredictedClickState, ECursorExecutionType::LocalPrediction);
	}
}

void UWorldCursorComponent::Client_ReconcileCursorClick_Implementation(uint32 ActionId, const FWorldCursorState ServerState)
{
	if (!IsNetMode(NM_Client))
		return;
	
	if (IsAlreadyProcessed(ActionId)) 
		return;
	
	ProcessedServerActionId = ActionId;
	
	if (!ServerState.HasValidActor())
	{
		RollbackLocalClick(ActionId);
		return;
	}

	UWorldClickableComponent* PredictedClickable = GetClickableComponent(PredictedClickState.Actor.Get());
	UWorldClickableComponent* ServerClickable = GetClickableComponent(ServerState.Actor.Get());

	if (ServerClickable)
	{
		if (PredictedClickable != ServerClickable)
		{
			RollbackLocalClick(ActionId);
		}
		
		ServerClickable->ApplyCursorAction(EWorldCursorAction::Click, ServerState.Location,
				ServerState.Normal, ECursorExecutionType::Reconciliation);
		OnCursorActionExecuted(ActionId, EWorldCursorAction::Click, ServerState, ECursorExecutionType::Reconciliation);
		PredictedClickState = ServerState;
	}
	else if (PredictedClickable)
	{
		RollbackLocalClick(ActionId);
	}
}

void UWorldCursorComponent::Client_ReconcileCursorRelease_Implementation(uint32 ActionId,
	const FWorldCursorState ServerState)
{
	if (!IsNetMode(NM_Client))
		return;
	
	if (IsAlreadyProcessed(ActionId)) 
		return;
	
	ProcessedServerActionId = ActionId;
	PredictedClickState.Reset();
}

void UWorldCursorComponent::UpdateLocalCursorHover(const FHitResult& CursorHit)
{
	if (!CanUpdateCursorHover())
	{
		if (LocalHoverState.HasValidActor())
		{
			CursorUnhover();
			LocalHoverState.Reset();
		}
		
		return;
	}

	UWorldClickableComponent* Clickable = GetClickableComponent(CursorHit.GetActor());
	if (CursorHit.bBlockingHit && Clickable && Clickable->CanAcceptClick())
	{
		const FWorldCursorState NewHoverState
		{
			CursorHit.GetActor(),
			CursorHit.Location,
			CursorHit.Normal,
		};
		
		if (LocalHoverState.HasValidActor())
		{
			if (NewHoverState.Actor.Get() != LocalHoverState.Actor.Get())
			{
				CursorUnhover();
				CursorHover(NewHoverState);
			}
		}
		else
		{
			CursorHover(NewHoverState);
		}
	}
	else if (LocalHoverState.HasValidActor())
	{
		CursorUnhover();
	}
}

void UWorldCursorComponent::CursorHover(const FWorldCursorState& NewState)
{
	if (NewState.HasValidActor())
	{
		UWorldClickableComponent* ClickableComponent = GetClickableComponent(NewState.Actor.Get());
		if (!ClickableComponent)
			return;
		
		ClickableComponent->ApplyCursorAction(EWorldCursorAction::Hover, NewState.Location,
			NewState.Normal, ECursorExecutionType::LocalPrediction);
		
		LocalHoverState = NewState;
	}
}

void UWorldCursorComponent::CursorUnhover()
{
	if (LocalHoverState.HasValidActor())
	{
		UWorldClickableComponent* ClickableComponent = GetClickableComponent(LocalHoverState.Actor.Get());
		if (!ClickableComponent)
			return;
		
		ClickableComponent->ApplyCursorAction(EWorldCursorAction::Unhover, LocalHoverState.Location,
			LocalHoverState.Normal, ECursorExecutionType::LocalPrediction);
		LocalHoverState.Reset();
	}
}

void UWorldCursorComponent::UpdateLocalCursorPosition(const FHitResult& CursorHit)
{
	if (CursorHit.bBlockingHit)
	{
		LocalCursorState.Location = CursorHit.Location;
		LocalCursorState.Normal = CursorHit.Normal;
		OnCursorChanged.Broadcast(bGrounded, LocalCursorState.Location, LocalCursorState.Normal);
	}
	else
	{
		if (!PlayerController) 
			return;

		FVector WorldLocation;
		FVector WorldDirection;
		PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
		
		LocalCursorState.Location = ProjectLocationToTraceDistance(WorldLocation, WorldDirection);
		LocalCursorState.Normal = FVector::UpVector;
		OnCursorChanged.Broadcast(bGrounded, LocalCursorState.Location, LocalCursorState.Normal);
	}
}

void UWorldCursorComponent::OnCursorActionExecuted(uint32 ActionId, EWorldCursorAction Action, const FWorldCursorState& State,
	ECursorExecutionType ExecutionType)
{
	
}

FHitResult UWorldCursorComponent::DoTrace(const FVector& Start, const FVector& Direction) const
{
	if (!PlayerController) 
		return FHitResult();
	
	const UWorld* World = GEngine->GetWorldFromContextObjectChecked(PlayerController);
	if (!World) 
		return FHitResult();

	const FVector SafeDirection = Direction.GetSafeNormal();
	if (SafeDirection.IsNearlyZero()) 
		return FHitResult();
	
	const FVector End = Start + (SafeDirection * TraceDistance);

	FHitResult Hit;
	World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, TraceChannel,
		FCollisionShape::MakeSphere(TraceRadius));

	/*if (IsNetMode(NM_Client))
	{
		World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, TraceChannel,
			FCollisionShape::MakeSphere(TraceRadius));
	}else
	{
		UKismetSystemLibrary::SphereTraceSingle(World, Start, End, TraceRadius, 
		UEngineTypes::ConvertToTraceType(TraceChannel), false, 
		TArray<AActor*>(), EDrawDebugTrace::ForDuration, Hit, true);
	}*/
	

	return Hit;
}

void UWorldCursorComponent::GetTraceStartAndDirection(FVector& StartOut, FVector& DirectionOut) const
{
	FVector WorldLocation;
	FVector WorldDirection;
	
	PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
	StartOut = GetPlayerViewLocation(PlayerController);
	DirectionOut = WorldDirection;
}

FVector UWorldCursorComponent::ProjectLocationToTraceDistance(const FVector& Start, const FVector& Direction) const
{
	return Start + (Direction * TraceDistance);
}

FVector UWorldCursorComponent::GetPlayerViewLocation(const APlayerController* PC) const
{
	if (!PC) 
		return FVector();

	if (PC->PlayerCameraManager)
	{
		return PC->PlayerCameraManager->GetCameraLocation();
	}

	if (PC->GetPawn())
	{
		return PC->GetPawn()->GetPawnViewLocation();
	}

	return FVector();
}

bool UWorldCursorComponent::IsAlreadyProcessed(uint32 ActionId) const
{
	return ActionId <= ProcessedServerActionId;
}

UWorldClickableComponent* UWorldCursorComponent::GetClickableComponent(AActor* Actor)
{
	if (!Actor || !Actor->Implements<UWorldCursorInterface>())
		return nullptr;
	
	UWorldClickableComponent* Component = IWorldCursorInterface::Execute_GetClickableComponent(Actor);
	ensureMsgf(Component, TEXT("%s Has clickable interface but doesn't have clickable component"),
		*GetNameSafe(Actor));
	
	return Component;
}

void UWorldCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bActivatedCursor)
	{
		FVector TraceStart, TraceDirection;
		GetTraceStartAndDirection(TraceStart, TraceDirection);
		const FHitResult TraceResult = DoTrace(TraceStart, TraceDirection);
		
		UpdateLocalCursorHover(TraceResult);
		UpdateLocalCursorPosition(TraceResult);
	}
}

