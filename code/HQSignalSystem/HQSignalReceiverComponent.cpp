// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/HQSignalSystem/HQSignalReceiverComponent.h"

#include "Gameplay/HQSignalSystem/HQSignalSystem.h"

UHQSignalReceiverComponent::UHQSignalReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHQSignalReceiverComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bActive)
	{
		UpdateSignal();
	}
}

void UHQSignalReceiverComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bStartActive)
	{
		ActivateSignalUpdate();
	}

	SignalSystem = GetWorld()->GetSubsystem<UHQSignalSystem>();
	check(SignalSystem.IsValid());
}

void UHQSignalReceiverComponent::UpdateSignal()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ReceiveSignalData"), STAT_ReceiveSignalData, STATGROUP_HQSignalSystem);
	
	// TODO logic of changed data, do not broadcast if not changed
	FHQSignalData SignalData = SignalSystem->GetSignalData(this);
	OnSignalUpdated.Broadcast(SignalData);
}
