// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.


#include "Gameplay/Mechanics/WorldMarkersSystem/WorldMarkerComponent.h"

#include "Gameplay/Mechanics/WorldMarkersSystem/WorldMarkerSubsystem.h"


UWorldMarkerComponent::UWorldMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.5f;
}


FVector UWorldMarkerComponent::GetMarkerLocation() const
{
	return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

void UWorldMarkerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	RegisterMarker();
}

void UWorldMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterMarker();
	
	Super::EndPlay(EndPlayReason);
}

void UWorldMarkerComponent::RegisterMarker()
{
	if (!bEnableMarker || bRegistered || !GetWorld())
		return;
	
	UWorldMarkerSubsystem* Subsystem = GetWorld()->GetSubsystem<UWorldMarkerSubsystem>();
	if (!Subsystem)
		return;
	
	Subsystem->RegisterMarker(this);
	bRegistered = true;
		
	if (bTrackMovement)
	{
		SetComponentTickEnabled(true);
	}
}

void UWorldMarkerComponent::UnregisterMarker()
{
	if (!bRegistered || !GetWorld())
		return;
	
	UWorldMarkerSubsystem* Subsystem = GetWorld()->GetSubsystem<UWorldMarkerSubsystem>();
	if (!Subsystem)
		return;
	
	Subsystem->UnregisterMarker(this);
	
	if (bTrackMovement)
	{
		SetComponentTickEnabled(false);
	}
	
	bRegistered = false;
}

void UWorldMarkerComponent::UpdateMarkerLocation()
{
	if (!bRegistered || !GetWorld())
		return;

	const FVector CurrentLocation = GetMarkerLocation();

	if (FVector::DistSquared(CurrentLocation, LastUpdatedLocation) < MovementUpdateThreshold)
		return;

	if (UWorldMarkerSubsystem* Subsystem = GetWorld()->GetSubsystem<UWorldMarkerSubsystem>())
	{
		Subsystem->UpdateMarkerLocation(this);
		LastUpdatedLocation = CurrentLocation;
	}
}


void UWorldMarkerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UpdateMarkerLocation();
}

void UWorldMarkerComponent::EnableMarker(bool bNewEnabled)
{
	if (bEnableMarker == bNewEnabled)
		return;

	bEnableMarker = bNewEnabled;

	if (bEnableMarker)
	{
		RegisterMarker();
	}
	else
	{
		UnregisterMarker();
	}
}

