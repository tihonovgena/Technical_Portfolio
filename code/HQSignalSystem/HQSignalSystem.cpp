// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/HQSignalSystem/HQSignalSystem.h"

#include "Gameplay/HQSignalSystem/HQSignalReceiverComponent.h"
#include "Gameplay/HQSignalSystem/HQSignalSourceComponent.h"
#include "Kismet/KismetMathLibrary.h"

constexpr uint8 MAXSIGNALTYPES = EHQSignalStrength::Max - 1;
static TAutoConsoleVariable<bool> CVarShowSignalInfo(
	TEXT("ShowSignalInfo"),
	false,
	TEXT("Show signal debug info on screen"),
	ECVF_Cheat
);

void UHQSignalSystem::RegisterSource(UHQSignalSourceComponent* Source)
{
	if (Source == nullptr) return;
	
	SignalSources.AddUnique(Source);
}

void UHQSignalSystem::UnRegisterSource(UHQSignalSourceComponent* Source)
{
	if (Source == nullptr) return;
	
	SignalSources.Remove(Source);
}

FHQSignalData UHQSignalSystem::GetSignalData(const UHQSignalReceiverComponent* Receiver) const
{
	float StrongestValue = 0.f;
	
	for (TWeakObjectPtr<UHQSignalSourceComponent> Source : SignalSources)
	{
		if (Source == nullptr) continue;
		
		FVector ReceiverLocation = Receiver->GetOwner()->GetActorLocation();
		FVector SourceLocation = Source->GetComponentLocation();
		const float SourceRadius = Source->GetSignalRadius();
		const float SquaredDistance = FVector::DistSquared(ReceiverLocation, SourceLocation);
		const float SquaredSignalRadius = FMath::Square(SourceRadius);

		// Check if source too far
		if (SquaredDistance > SquaredSignalRadius)  continue;
		
		float SourceStrength = 1.f - FMath::Clamp(SquaredDistance / SquaredSignalRadius, 0.f, 1.f);

		// Check obstacles visibilityModifier
		UWorld* World = Receiver->GetWorld();
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActors(TArray{Receiver->GetOwner(), Source->GetOwner()});
		// TODO Move trace channel to settings?
		// TODO Change into sphere trace?
		// TODO Encapsulate into method
		World->LineTraceSingleByChannel(Hit, SourceLocation, ReceiverLocation, ECC_Visibility, Params);
		if (Hit.bBlockingHit)
		{
			// Do revers trace to check other side block location
			FHitResult ReversedHit;
			World->LineTraceSingleByChannel(ReversedHit, ReceiverLocation, SourceLocation, ECC_Visibility, Params);
			
			const float Penetration = Source->GetSignalPenetration();
			if (Penetration > 0.f)
			{
				const float ObstacleDepth = (Hit.Location - ReversedHit.Location).Size();
				SourceStrength *=  1.f - FMath::Clamp(ObstacleDepth / Penetration, 0.f, 1.f);
			}
			else
			{
				SourceStrength = 0.f;
			}
		}

		// TODO Transfer into debug level
		if (CVarShowSignalInfo.GetValueOnGameThread() && GEngine)
		{
			const FString SourceName = Source->GetOwner()
				? Source->GetOwner()->GetName()
				: Source->GetName();

			GEngine->AddOnScreenDebugMessage(
				-1,
				0.f,
				FColor::Cyan,
				FString::Printf(
					TEXT("Signal | Source: %s | Strength: %.2f | Distance: %.0f | Radius: %.0f"),
					*SourceName,
					SourceStrength,
					FMath::Sqrt(SquaredDistance),
					SourceRadius
				)
			);
		}
		StrongestValue = FMath::Max(StrongestValue, SourceStrength);
	}
	
	int32 SignalStrengthInt = FMath::CeilToInt(StrongestValue * MAXSIGNALTYPES);
	const float NormalizedValue = FMath::IsNearlyEqual(StrongestValue, 1.f) ?
		1.f : 1 - (SignalStrengthInt - (StrongestValue * MAXSIGNALTYPES));
	const EHQSignalStrength::Type SignalStrength = static_cast<EHQSignalStrength::Type>(SignalStrengthInt);
	
	return  {StrongestValue, NormalizedValue, SignalStrength};
}
