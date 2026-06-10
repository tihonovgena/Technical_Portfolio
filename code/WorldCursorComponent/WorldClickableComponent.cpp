// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.


#include "Gameplay/Mechanics/WorldCursorComponent/WorldClickableComponent.h"

#include "Net/UnrealNetwork.h"


UWorldClickableComponent::UWorldClickableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWorldClickableComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWorldClickableComponent, AuthorityClickedState);
}

void UWorldClickableComponent::ApplyCursorAction(EWorldCursorAction Action, const FVector& Location,
	const FVector& Normal, ECursorExecutionType ExecutionType)
{
	switch (ExecutionType)
	{
	case ECursorExecutionType::LocalPrediction:
		ApplyLocalState(Action, Location, Normal);
		break;
	case ECursorExecutionType::ServerAuthority:
		ApplyAuthorityState(Action, Location, Normal);
		break;
	default:
		break;
	}
}

void UWorldClickableComponent::OnRep_AuthorityClickedState()
{
	const EWorldCursorAction Action = AuthorityClickedState.bClicked ? 
	EWorldCursorAction::Click : EWorldCursorAction::Release;

	ApplyLocalState(Action, AuthorityClickedState.Location, AuthorityClickedState.Normal);
}

void UWorldClickableComponent::ApplyLocalState(EWorldCursorAction Action, const FVector& Location, const FVector& Normal)
{
	switch (Action)
	{
	case EWorldCursorAction::Click :
		if (LocalClickedState.bClicked)
			return;
		
		LocalClickedState = 
		{
			true, 
			Location, 
			Normal
		};

		BroadcastAction(Action, Location, Normal);
		break;
		
	case EWorldCursorAction::Release :
		if (!LocalClickedState.bClicked)
			return;
		
		LocalClickedState = 
		{
			false, 
			Location, 
			Normal
		};
		BroadcastAction(Action, Location, Normal);
		break;
		
	case EWorldCursorAction::Hover :
	case EWorldCursorAction::Unhover :
		BroadcastAction(Action, Location, Normal);
		break;
	}
}

void UWorldClickableComponent::ApplyAuthorityState(EWorldCursorAction Action, const FVector& Location, const FVector& Normal)
{
	switch (Action)
	{
	case EWorldCursorAction::Click :
		if (AuthorityClickedState.bClicked)
			return;
			
		AuthorityClickedState = 
		{
			true, 
			Location, 
			Normal
		};
		
		if (!IsNetMode(NM_DedicatedServer))
		{
			ApplyLocalState(Action, Location, Normal);
		}
		break;
		
	case EWorldCursorAction::Release :
		if (!AuthorityClickedState.bClicked)
			return;
		
		AuthorityClickedState = 
		{
			false, 
			Location, 
			Normal
		};
		
		if (!IsNetMode(NM_DedicatedServer))
		{
			ApplyLocalState(Action, Location, Normal);
		}
		break;
		
	default:
		break;
	}
}

void UWorldClickableComponent::BroadcastAction(EWorldCursorAction Action, const FVector& Location, const FVector& Normal) const
{
	switch (Action)
	{
	case EWorldCursorAction::Click :
		OnWorldClick.Broadcast(GetOwner(), Location, Normal);
		break;

	case EWorldCursorAction::Release :
		OnWorldRelease.Broadcast(GetOwner(), Location, Normal);
		break;

	case EWorldCursorAction::Hover :
		OnWorldHover.Broadcast(GetOwner(), Location, Normal);
		break;

	case EWorldCursorAction::Unhover :
		OnWorldUnhover.Broadcast(GetOwner(), Location, Normal);
		break;

	default:
		break;
	}
}
