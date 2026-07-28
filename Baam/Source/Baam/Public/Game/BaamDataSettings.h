// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "BaamDataSettings.generated.h"

class UDataTable;
/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Baam Data"))
class BAAM_API UBaamDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Config, Category = "Data", meta = (RequiredAssetDataTags = "RowStructure=/Script/Baam.BaamCharacterRow"))
	TSoftObjectPtr<UDataTable> CharacterTable;

	UPROPERTY(EditAnywhere, Config, Category = "Data", meta = (RequiredAssetDataTags = "RowStructure=/Script/Baam.BaamCardRow"))
	TSoftObjectPtr<UDataTable> CardTable;
};
