// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/HQSignalSystem/HQSignalSourceComponent.h"

#include "Gameplay/HQSignalSystem/HQSignalSourceDataAsset.h"
#include "Gameplay/HQSignalSystem/HQSignalSystem.h"

UHQSignalSourceComponent::UHQSignalSourceComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

float UHQSignalSourceComponent::GetSignalRadius()
{
   return SignalSourceData != nullptr ? SignalSourceData->Radius : 0.f;
}

float UHQSignalSourceComponent::GetSignalPenetration()
{
   return SignalSourceData != nullptr ? SignalSourceData->SignalPenetration : 0.f;
}

void UHQSignalSourceComponent::BeginPlay()
{
   Super::BeginPlay();

   UHQSignalSystem* SignalSystem = GetWorld()->GetSubsystem<UHQSignalSystem>();
   if (ensure(SignalSystem))
   {
      SignalSystem->RegisterSource(this);
   }
}


void UHQSignalSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   UHQSignalSystem* SignalSystem = GetWorld()->GetSubsystem<UHQSignalSystem>();
   if (ensure(SignalSystem))
   {
      SignalSystem->UnRegisterSource(this);
   }
  
   Super::EndPlay(EndPlayReason);
}
