// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpriterDataModel.h" //@TODO: For debug only
#include "SpriterImportData.generated.h"

// This is the 'hub' asset that tracks other imported assets for a rigged sprite character exported from Spriter
UCLASS(BlueprintType)
class SPRITER_API USpriterImportData : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FSpriterSCON ImportedData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float PixelsPerUnrealUnit;

	// Import data for this 
	UPROPERTY(EditAnywhere, Instanced, Category=ImportSettings)
	class UAssetImportData* AssetImportData;

	/** Override to ensure we write out the asset import data */
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
};
