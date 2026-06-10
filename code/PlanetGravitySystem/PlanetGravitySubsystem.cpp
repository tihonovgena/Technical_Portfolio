// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetGravitySubsystem.h"

#include "PlanetGravityAsyncCallback.h"
#include "PBDRigidsSolver.h"
#include "PlanetGravityAttractorComponent.h"
#include "Physics/Experimental/PhysScene_Chaos.h"

void UPlanetGravitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}
 
void UPlanetGravitySubsystem::Deinitialize()
{
	Super::Deinitialize();
}
 
void UPlanetGravitySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	GEngine->AddOnScreenDebugMessage( -1, 0, FColor::Green, *FString::Printf(TEXT("%d Attractors in the World"), Attractors.Num()));

	if (!IsAsyncCallbackRegistered())
	{
		RegisterAsyncCallback();
	}
}
 
TStatId UPlanetGravitySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGravitySubsystem, STATGROUP_Tickables);
}
 
void UPlanetGravitySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
 
void UPlanetGravitySubsystem::AddAttractor(UPlanetGravityAttractorComponent* GravityAttractorComponent)
{
	Attractors.Add(GravityAttractorComponent);
}
 
void UPlanetGravitySubsystem::RemoveAttractor(UPlanetGravityAttractorComponent* GravityAttractorComponent)
{
	Attractors.Remove(GravityAttractorComponent);
}

void UPlanetGravitySubsystem::RegisterAsyncCallback()
{
	if (UWorld* World = GetWorld())
	{
		if (FPhysScene* PhysScene = World->GetPhysicsScene())
		{
			AsyncCallback = PhysScene->GetSolver()->CreateAndRegisterSimCallbackObject_External<FPlanetGravityAsyncCallback>();
		}
	}
}
 
bool UPlanetGravitySubsystem::IsAsyncCallbackRegistered() const
{
	return AsyncCallback != nullptr;
}
 
void UPlanetGravitySubsystem::AddGravityAttractorData(const FGravityAttractorData& InputData) const
{
	if (IsAsyncCallbackRegistered())
	{
		FGravityAsyncInput* Input = AsyncCallback->GetProducerInputData_External();
		Input->GravityAttractorsData.Add(InputData);	
	}
}
