// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetGravityAttractorComponent.h"

#include "PlanetGravityAsyncCallback.h"
#include "PlanetGravitySubsystem.h"

UPlanetGravityAttractorComponent::UPlanetGravityAttractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// Important, we need to be ticked before the Physics Thread to send the attractor location! 
	PrimaryComponentTick.bStartWithTickEnabled = true; 
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics; 
}
 

void UPlanetGravityAttractorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPlanetGravityAttractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	BuildAsyncInput();

	if (bShowGravityRadius)
	{
		DrawDebugSphere(GetWorld(), GetComponentLocation(), Radius, 32, FColor::Purple);
	}
}
 
void UPlanetGravityAttractorComponent::OnRegister()
{
	Super::OnRegister();
 
	if (UWorld* World = GetWorld())
	{
		GravitySubsystem = World->GetSubsystem<UPlanetGravitySubsystem>();
		
		if (GravitySubsystem.IsValid())
		{
			GravitySubsystem->AddAttractor(this);
		}
	}
}
 
void UPlanetGravityAttractorComponent::OnUnregister()
{
	if (GravitySubsystem.IsValid())
	{
		GravitySubsystem->RemoveAttractor(this);
	}
	
	Super::OnUnregister();
}

void UPlanetGravityAttractorComponent::GetGravityAttractorData(FGravityAttractorData& DataOut) const
{
	DataOut.Location = GetComponentLocation();
	if (bUseGravityAtRadius)
	{
		DataOut.MassDotG = Gravity * Radius * Radius;
	}
	else
	{
		DataOut.MassDotG = Mass * 6.67430E-5 ; // G = 6.67430E-11 m³kg⁻¹s⁻², because 1m equals 100 UE Units, we have to multiply by a 100³ factor, so E-11 goes E-5 
	}
}
 
void UPlanetGravityAttractorComponent::BuildAsyncInput()
{
	if (bApplyGravity && GravitySubsystem.IsValid())
	{
		FGravityAttractorData GravityAttractorData;
		GetGravityAttractorData(GravityAttractorData);
		GravitySubsystem->AddGravityAttractorData(GravityAttractorData);
	}
}

