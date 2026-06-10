// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.


#include "Gameplay/Mechanics/WorldMarkersSystem/WorldMarkerSubsystem.h"

#include "Gameplay/Mechanics/WorldMarkersSystem/WorldMarkerComponent.h"

TArray<FWorldMarkerQueryResult> UWorldMarkerSubsystem::GetMarkersInRadius(const FVector& Origin, float Radius,
                                                                          int32 MaxResults)
{
	TArray<FWorldMarkerQueryResult> Results;

    if (Radius <= 0.0f)
    	return Results;

    const float RadiusSq = Radius * Radius;
    const FIntVector CenterCell = LocationToCell(Origin);
    const int32 CellRange = FMath::CeilToInt(Radius / FMath::Max(CellSize, 1.0f));

    for (int32 X = CenterCell.X - CellRange; X <= CenterCell.X + CellRange; ++X)
    {
        for (int32 Y = CenterCell.Y - CellRange; Y <= CenterCell.Y + CellRange; ++Y)
        {
            for (int32 Z = CenterCell.Z - CellRange; Z <= CenterCell.Z + CellRange; ++Z)
            {
                const FIntVector Cell(X, Y, Z);
                FMarkersArray* MarkersArray = Grid.Find(Cell);

                if (!MarkersArray || MarkersArray->Markers.IsEmpty())
                	continue;

                for (int32 Index = MarkersArray->Markers.Num() - 1; Index >= 0; --Index)
                {
                    UWorldMarkerComponent* Marker = MarkersArray->Markers[Index].Get();

                    if (!IsValid(Marker) || !Marker->IsEnableMarker())
                    {
                        MarkersArray->Markers.RemoveAtSwap(Index);
                        continue;
                    }

                    const FVector MarkerLocation = Marker->GetMarkerLocation();
                    const float DistanceSq = FVector::DistSquared(Origin, MarkerLocation);

                    if (DistanceSq <= RadiusSq)
                    {
                        FWorldMarkerQueryResult Result;
                        Result.Marker = Marker;
                        Result.Location = MarkerLocation;
                        Result.DistanceSq = DistanceSq;

                        Results.Add(Result);
                    }
                }
            }
        }
    }

    Results.Sort([](const FWorldMarkerQueryResult& A, const FWorldMarkerQueryResult& B)
    {
        return A.DistanceSq < B.DistanceSq;
    });

    if (MaxResults > 0 && Results.Num() > MaxResults)
    {
        Results.SetNum(MaxResults);
    }
	
	return Results;
}

void UWorldMarkerSubsystem::ClearInvalidMarkers()
{
	for (auto GridIt = Grid.CreateIterator(); GridIt; ++GridIt)
	{
		FMarkersArray& MarkersArray = GridIt.Value();

		MarkersArray.Markers.RemoveAll([](const TWeakObjectPtr<UWorldMarkerComponent>& Ptr)
		{
			return !Ptr.IsValid();
		});

		if (MarkersArray.Markers.IsEmpty())
		{
			GridIt.RemoveCurrent();
		}
	}

	for (auto MapIt = MarkerToCell.CreateIterator(); MapIt; ++MapIt)
	{
		if (!MapIt.Key().IsValid())
		{
			MapIt.RemoveCurrent();
		}
	}
}

void UWorldMarkerSubsystem::RegisterMarker(UWorldMarkerComponent* Marker)
{
	if (!IsValid(Marker))
		return;

	UnregisterMarker(Marker);

	const FIntVector Cell = LocationToCell(Marker->GetMarkerLocation());

	AddToCell(Marker, Cell);
}

void UWorldMarkerSubsystem::UnregisterMarker(UWorldMarkerComponent* Marker)
{
	if (Marker == nullptr)
		return;

	const FIntVector* ExistingCell = MarkerToCell.Find(Marker);
	if (!ExistingCell)
		return;
	
	RemoveFromCell(Marker, *ExistingCell);
	MarkerToCell.Remove(Marker);
}

void UWorldMarkerSubsystem::UpdateMarkerLocation(UWorldMarkerComponent* Marker)
{
	if (!IsValid(Marker))
		return;

	const FIntVector NewCell = LocationToCell(Marker->GetMarkerLocation());
	const FIntVector* OldCell = MarkerToCell.Find(Marker);
	
	if (!OldCell)
	{
		RegisterMarker(Marker);
		return;
	}
	
	if (*OldCell == NewCell)
		return;

	RemoveFromCell(Marker, *OldCell);
	AddToCell(Marker, NewCell);
	MarkerToCell[Marker] = NewCell;
}

FIntVector UWorldMarkerSubsystem::LocationToCell(const FVector& Location) const
{
	const float SafeCellSize = FMath::Max(CellSize, 1.0f);

	return FIntVector
	(
		FMath::FloorToInt(Location.X / SafeCellSize),
		FMath::FloorToInt(Location.Y / SafeCellSize),
		FMath::FloorToInt(Location.Z / SafeCellSize)
	);
}

void UWorldMarkerSubsystem::AddToCell(UWorldMarkerComponent* Marker, const FIntVector& Cell)
{
	FMarkersArray& CellMarkers = Grid.FindOrAdd(Cell);
	CellMarkers.Markers.AddUnique(Marker);
	MarkerToCell.Add(Marker, Cell);
}

void UWorldMarkerSubsystem::RemoveFromCell(UWorldMarkerComponent* Marker, const FIntVector& Cell)
{
	if (FMarkersArray* CellMarkers = Grid.Find(Cell))
	{
		CellMarkers->Markers.RemoveAll([Marker](const TWeakObjectPtr<UWorldMarkerComponent>& Ptr)
		{
			return !Ptr.IsValid() || Ptr.Get() == Marker;
		});

		if (CellMarkers->Markers.IsEmpty())
		{
			Grid.Remove(Cell);
		}
	}
}
