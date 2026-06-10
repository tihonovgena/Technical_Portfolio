// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Mechanics/Interaction/InteractionTypes.h"
#include "InteractionComponent.generated.h"

USTRUCT(BlueprintType)
struct FInteractionTargetOptions
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> Target;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FInteractionOption> Options;
};

class UInteractableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionOptionsChangedSignature, FInteractionTargetOptions, TargetOptions);

UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class LOSTINABYSS_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable, Category="Interaction")
	AActor* GetCurrentTarget() const {return CurrentTarget.Get();}
	
	UFUNCTION(BlueprintPure, Category="Interaction")
	const TArray<FInteractionOption>& GetCurrentOptions() const { return CurrentOptions; }
	
	UFUNCTION(BlueprintPure, Category="Interaction")
	TArray<FInteractionOption> GetCurrentOptionsCopy() const { return CurrentOptions; }
	
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void SetCurrentTarget(AActor* Target);
	
	UFUNCTION(BlueprintCallable, Category="Interaction")
	bool TryExecuteOptionWithInput(AActor* Target, const FGameplayTag& InputTag);
	
	UFUNCTION(BlueprintCallable, Category="Interaction")
	bool TryExecuteOption(AActor* Target, const FInteractionOption& Option);
	
	UFUNCTION(BlueprintCallable, Category="Interaction")
	TArray<FInteractionOption> GetInteractionOptions(AActor* Target) const;
	
	// Provides first option with desired ActionType and highest priority
	UFUNCTION(BlueprintCallable, Category="Interaction")
	FInteractionOption FindOptionWithInput(AActor* Target, const FGameplayTag& ActionTag) const;
	
private:
	void RefreshCurrentOptions();
	
public:
	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FOnInteractionOptionsChangedSignature OnInteractionOptionsChanged;
	
private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTarget;
	
	UPROPERTY(Transient)
	TArray<FInteractionOption> CurrentOptions;
};
