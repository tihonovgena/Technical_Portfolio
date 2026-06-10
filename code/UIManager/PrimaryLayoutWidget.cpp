// Copyright (c) 2026 Gennadiy Tikhonov. All Rights Reserved.

#include "LostSignal/Public/Game/UIManager/PrimaryLayoutWidget.h"

#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UCommonActivatableWidget* UPrimaryLayoutWidget::PushWidget( const TSubclassOf<UCommonActivatableWidget> WidgetClass,
	const FGameplayTag& LayerTag) const
{
	if (!WidgetClass || !LayerTag.IsValid()) return nullptr;
	
	UCommonActivatableWidgetStack* Stack = GetLayerStack(LayerTag);
	if (!Stack) return nullptr;

	UCommonActivatableWidget* Widget = Stack->AddWidget(WidgetClass);
	if (!Widget) return nullptr;

	// TODO add FUIWidgetHandle implementation? 
	//{
	//		TWeakObjectPtr<UCommonActivatableWidget> Widget;
	//	FGameplayTag LayerTag;
	//}
	OnPushWidget.Broadcast(Widget, LayerTag);
	return Widget;
}

void UPrimaryLayoutWidget::PopWidget(UCommonActivatableWidget* WidgetToRemove)
{
	if (!WidgetToRemove) return;

	for (const auto&[LayerTag, Stack] : LayerMap)
	{
		if (!Stack) continue;
		if (Stack->GetWidgetList().Contains(WidgetToRemove))
		{
			Stack->RemoveWidget(*WidgetToRemove);
			OnPopWidget.Broadcast(WidgetToRemove, LayerTag);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("PopWidget failed: widget [%s] was not found in any registered UI layer."),
		*GetNameSafe(WidgetToRemove));
}

void UPrimaryLayoutWidget::PopActiveWidget(const FGameplayTag& LayerTag)
{
	UCommonActivatableWidgetStack* Stack = GetLayerStack(LayerTag);
	if (!Stack) return;

	UCommonActivatableWidget* ActiveWidget = Stack->GetActiveWidget();
	if (!ActiveWidget) return;
	
	Stack->RemoveWidget(*ActiveWidget);
	OnPopWidget.Broadcast(ActiveWidget, LayerTag);
}

UCommonActivatableWidget* UPrimaryLayoutWidget::GetActiveWidget(const FGameplayTag& LayerTag) const
{
	const UCommonActivatableWidgetStack* Stack = GetLayerStack(LayerTag);
	return Stack ? Stack->GetActiveWidget() : nullptr;
}

bool UPrimaryLayoutWidget::HasActiveWidget(const FGameplayTag& LayerTag) const
{
	return GetLayerNumWidgets(LayerTag) > 0;
}

int32 UPrimaryLayoutWidget::GetLayerNumWidgets(const FGameplayTag& LayerTag) const
{
	const UCommonActivatableWidgetStack* Stack = GetLayerStack(LayerTag);
	return Stack ? Stack->GetNumWidgets() : 0;
}

void UPrimaryLayoutWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	LayerMap.Reset();

	// TODO Change into UUILayerWidget : UCommonActivatableWidgetStack eventually
	// TODO Where layer contain layer tag and layout scans for children
	for (const auto& [LayerTag, StackWidgetName] : RegisteredLayers)
	{
		if (!LayerTag.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("PrimaryLayout: Invalid LayerTag."));
			continue;
		}

		if (StackWidgetName.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("PrimaryLayout: Layer [%s] has invalid widget name."), *LayerTag.ToString());
			continue;
		}

		UWidget* FoundWidget = WidgetTree->FindWidget(StackWidgetName);

		UCommonActivatableWidgetStack* Stack = Cast<UCommonActivatableWidgetStack>(FoundWidget);

		if (!Stack)
		{
			UE_LOG(LogTemp, Warning, TEXT("PrimaryLayout: Widget [%s] is not a CommonActivatableWidgetStack."),
				*StackWidgetName.ToString());
			continue;
		}

		LayerMap.Add(LayerTag, Stack);
	}
}
