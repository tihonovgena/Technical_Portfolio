# Common UI Manager

A local-player UI manager built around CommonUI activatable widget stacks.

## Status

- **Current status:** Prototype UI flow manager.
- **Main purpose:** Centralize creation and routing of primary UI layers.
- **Best files to review first:**
  - [UIManagerSubsystem.cpp](../../code/UIManager/UIManagerSubsystem.cpp)
  - [PrimaryLayoutWidget.cpp](../../code/UIManager/PrimaryLayoutWidget.cpp)

## Problem

The project needed a reusable UI layout that could push and pop widgets into named layers using CommonUI.

This allows gameplay systems to route UI screens through a primary layout instead of manually adding widgets to the viewport in many places.

## Constraints

- Use CommonUI activatable widgets.
- Support multiple UI layers through gameplay tags.
- Keep the initial implementation simple.
- Avoid overbuilding MVVM before the vertical slice requires it.

## Architecture

```text
Local Player
└── UUIManagerSubsystem
    ├── creates PrimaryLayoutWidget
    ├── removes PrimaryLayoutWidget
    └── exposes current layout

UPrimaryLayoutWidget
├── RegisteredLayers: GameplayTag -> WidgetName
├── LayerMap: GameplayTag -> UCommonActivatableWidgetStack
├── PushWidget(WidgetClass, LayerTag)
├── PopWidget(Widget)
└── PopActiveWidget(LayerTag)
```

## Implementation Notes

- `UUIManagerSubsystem` owns the primary layout instance for the local player.
- `UPrimaryLayoutWidget` scans configured layer widget names during initialization.
- Layers are addressed by gameplay tags.
- Widgets are pushed into `UCommonActivatableWidgetStack`.
- Push/pop events are broadcast so other systems can react to UI changes.

## Trade-offs

The May devlog mentions considering MVVM, but postponing it because the vertical slice did not need it yet.

That is a reasonable prototype trade-off: CommonUI structure first, MVVM later only if the UI complexity justifies it.

## Known Limitations

- No widget handle system is implemented yet.
- Layer configuration is name-based and could be improved.
- MVVM binding is not included in the selected source.

## Next Steps

- Add a widget handle if lifetime management becomes more complex.
- Replace widget-name lookup with a more explicit layer widget component/type.
- Add MVVM only for UI screens where it reduces coupling.

