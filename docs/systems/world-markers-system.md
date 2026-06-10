# World Markers Spatial Query

A world marker subsystem for querying nearby points of interest through a spatial grid.

![World markers](../../screenshots/Image_WorldMarkers_1.png)

## Status

- **Current status:** Prototype-ready spatial query system.
- **Main purpose:** Find nearby interesting objects without scanning every marker actor every time.
- **Best files to review first:**
  - [WorldMarkerSubsystem.cpp](../../code/WorldMarkersSystem/WorldMarkerSubsystem.cpp)
  - [WorldMarkerComponent.cpp](../../code/WorldMarkersSystem/WorldMarkerComponent.cpp)

## Problem

The game needed a way to show markers for interesting objects within a radius.

A naive implementation could scan all marker actors every query. The implemented version uses a cell-based spatial grid so radius queries only inspect nearby cells.

## Constraints

- Markers can be registered and unregistered dynamically.
- Some markers can move.
- Invalid destroyed marker references must not remain in the grid forever.
- Query results need to be sorted by distance.
- The prototype should stay simple and easy to reason about.

## Architecture

```text
Actor
└── UWorldMarkerComponent
    ├── registers with UWorldMarkerSubsystem
    ├── optionally tracks movement
    └── updates grid cell when movement threshold is exceeded

UWorldMarkerSubsystem
├── Grid: Cell -> Markers
├── MarkerToCell: Marker -> Cell
└── GetMarkersInRadius(Origin, Radius, MaxResults)
```

## Implementation Notes

The subsystem converts world locations into integer grid cells.

During a radius query:

1. Convert the query origin to a center cell.
2. Determine the cell range from the query radius and cell size.
3. Iterate only over cells inside that range.
4. Remove invalid or disabled markers during query.
5. Check exact squared distance against the query radius.
6. Sort results by distance.
7. Apply `MaxResults` if needed.

The marker component uses a movement threshold before updating the grid cell. This avoids unnecessary grid updates for tiny movements.

## Trade-offs

The system is intentionally synchronous and simple. The May devlog mentions considering multithreading for nearest-object calculation later, but the current code keeps the implementation straightforward.

## Known Limitations

- No async query path is included.
- Query cost can still grow if many markers occupy the same cells.
- Cell size tuning is not documented in the selected source.
- There is no built-in gameplay filtering by marker type in the selected source.

## Next Steps

- Add marker categories or gameplay tags if multiple marker types are needed.
- Add profiling before deciding whether multithreading is actually necessary.
- Add debug visualization for grid cells and query radius.
- Tune cell size based on real gameplay density.

