// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InteractionTypes.h"
#include "UObject/Object.h"
#include "InteractionFragment.generated.h"

struct FInteractionContext;

UCLASS(Abstract, EditInlineNew)
class LOSTINABYSS_API UInteractionFragment : public UObject
{
	GENERATED_BODY()
	
public:
	virtual bool CanBeExecuted(const FInteractionContext& Context) const;
	virtual bool TryExecute(const FInteractionContext& Context) const;
	
	FInteractionOption GetOption() const
	{
		FInteractionOption Option;
		Option.DisplayText = DisplayText;
		Option.ActionTag = InputTag;
		Option.Priority = Priority;
		Option.Fragment = this;
		return Option;
	}
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess=true))
	FText DisplayText;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess=true))
	FGameplayTag InputTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess=true))
	int32 Priority = 0;
	
};
