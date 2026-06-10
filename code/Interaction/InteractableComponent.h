// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"


struct FInteractionContext;
struct FInteractionOption;
class UInteractableDefinition;

UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class LOSTINABYSS_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();
	
	TArray<FInteractionOption> GetAvailableOptions(const FInteractionContext& Context);
	TArray<FInteractionOption> GetAllOptions(const FInteractionContext& Context);
	UInteractableDefinition* GetDefinition() const {return Definition;}
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TObjectPtr<UInteractableDefinition> Definition;
};
