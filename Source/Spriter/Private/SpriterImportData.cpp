// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SpriterPrivatePCH.h"
#include "SpriterImportData.h"

//////////////////////////////////////////////////////////////////////////
// USpriterImportData

USpriterImportData::USpriterImportData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USpriterImportData::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (AssetImportData != nullptr)
	{
		OutTags.Add( FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden) );
	}

	Super::GetAssetRegistryTags(OutTags);
}
