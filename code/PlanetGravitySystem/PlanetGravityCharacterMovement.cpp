// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetGravityCharacterMovement.h"

#include "PlanetGravityAsyncCallback.h"
#include "PlanetGravityAttractorComponent.h"
#include "PlanetGravitySubsystem.h"
#include "PlanetGravityUtilities.h"

UPlanetGravityCharacterMovement::UPlanetGravityCharacterMovement()
{
	PrimaryComponentTick.bCanEverTick = true;
	bOrientRotationToMovement = true;
}


void UPlanetGravityCharacterMovement::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		GravitySubsystem = World->GetSubsystem<UPlanetGravitySubsystem>();
		check(GravitySubsystem.IsValid());
	}
}

void UPlanetGravityCharacterMovement::TickComponent(float DeltaTime, ELevelTick TickType,
                                                    FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	FVector AdditionalAcceleration = FVector::ZeroVector;

	const TArray<TWeakObjectPtr<UPlanetGravityAttractorComponent>>& Attractors = GravitySubsystem->GetAttractors();
	for (auto& GravityAttractor: Attractors)
	{
		if (GravityAttractor->IsApplaiable())
		{
			FGravityAttractorData GravityAttractorData;
			GravityAttractor->GetGravityAttractorData(GravityAttractorData);

			FVector GravityAcceleration = FPlanetGravityUtilities::CalculateAcceleration(GetActorLocation(),
				GravityAttractorData.Location, GravityAttractorData.MassDotG);
			
			AdditionalAcceleration += GravityAcceleration;
		}
	}
 
	DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + AdditionalAcceleration, 1.0, FColor::Red, 0, false, 1.0f  );
	DrawDebugString(GetWorld(), GetActorLocation(), * FString::Printf(TEXT("%.2f"), AdditionalAcceleration.Length()), nullptr, FColor::Red, 0, false, 1.0f  );
	
	AddForce(AdditionalAcceleration * Mass);
	SetGravityDirection(AdditionalAcceleration.GetSafeNormal());
}

