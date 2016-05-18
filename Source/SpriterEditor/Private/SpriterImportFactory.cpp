// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SpriterEditorPrivatePCH.h"
#include "Json.h"
#include "PaperJSONHelpers.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "PackageTools.h"
#include "SpriterImportFactory.h"
#include "SpriterImportData.h"
#include "PaperImporterSettings.h"
#include "SpriterCharacterMap.h"

#include "PaperSprite.h"

#define LOCTEXT_NAMESPACE "Spriter"

static ESpritePivotMode::Type ConvertNormalizedPivotPointToPivotMode(float PivotX, float PivotY)
{
	// Determine the ideal pivot
	const bool bIsLeft = FMath::IsNearlyEqual(PivotX, 0.0f);
	const bool bIsCenterX = FMath::IsNearlyEqual(PivotX, 0.5f);
	const bool bIsRight = FMath::IsNearlyEqual(PivotX, 1.0f);
	const bool bIsTop = FMath::IsNearlyEqual(PivotY, 0.0f);
	const bool bIsCenterY = FMath::IsNearlyEqual(PivotY, 0.5f);
	const bool bIsBottom = FMath::IsNearlyEqual(PivotY, 1.0f);

	ESpritePivotMode::Type PivotMode = ESpritePivotMode::Custom;
	if (bIsLeft)
	{
		if (bIsTop)
		{
			PivotMode = ESpritePivotMode::Top_Left;
		}
		else if (bIsCenterY)
		{
			PivotMode = ESpritePivotMode::Center_Left;
		}
		else if (bIsBottom)
		{
			PivotMode = ESpritePivotMode::Bottom_Left;
		}
	}
	else if (bIsCenterX)
	{
		if (bIsTop)
		{
			PivotMode = ESpritePivotMode::Top_Center;
		}
		else if (bIsCenterY)
		{
			PivotMode = ESpritePivotMode::Center_Center;
		}
		else if (bIsBottom)
		{
			PivotMode = ESpritePivotMode::Bottom_Center;
		}
	}
	else if (bIsRight)
	{
		if (bIsTop)
		{
			PivotMode = ESpritePivotMode::Top_Right;
		}
		else if (bIsCenterY)
		{
			PivotMode = ESpritePivotMode::Center_Right;
		}
		else if (bIsBottom)
		{
			PivotMode = ESpritePivotMode::Bottom_Right;
		}
	}

	return PivotMode;
}

//////////////////////////////////////////////////////////////////////////
// USpriterImportFactory

USpriterImportFactory::USpriterImportFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	SupportedClass = USpriterImportData::StaticClass();

	bEditorImport = true;
	bText = true;

	Formats.Add(TEXT("scon;Spriter SCON file"));
}

FText USpriterImportFactory::GetToolTip() const
{
	return LOCTEXT("SpriterImporterFactoryDescription", "Characters exported from Spriter");
}

bool USpriterImportFactory::FactoryCanImport(const FString& Filename)
{
	FString FileContent;
	if (FFileHelper::LoadFileToString(/*out*/ FileContent, *Filename))
	{
		TSharedPtr<FJsonObject> DescriptorObject = ParseJSON(FileContent, FString(), /*bSilent=*/ true);
		if (DescriptorObject.IsValid())
		{
			FSpriterSCON GlobalInfo;
			GlobalInfo.ParseFromJSON(DescriptorObject, Filename, /*bSilent=*/ true, /*bPreparseOnly=*/ true);

			return GlobalInfo.IsValid();
		}
	}

	return false;
}

UObject* USpriterImportFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	Flags |= RF_Transactional;

	FEditorDelegates::OnAssetPreImport.Broadcast(this, InClass, InParent, InName, Type);

 	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
 
 	bool bLoadedSuccessfully = true;
 
 	const FString CurrentFilename = UFactory::GetCurrentFilename();
 	FString CurrentSourcePath;
 	FString FilenameNoExtension;
 	FString UnusedExtension;
 	FPaths::Split(CurrentFilename, CurrentSourcePath, FilenameNoExtension, UnusedExtension);
 
 	const FString LongPackagePath = FPackageName::GetLongPackagePath(InParent->GetOutermost()->GetPathName());
 
 	const FString NameForErrors(InName.ToString());
 	const FString FileContent(BufferEnd - Buffer, Buffer);
 	TSharedPtr<FJsonObject> DescriptorObject = ParseJSON(FileContent, NameForErrors);

	USpriterImportData* Result = nullptr;
 
	// Parse the file 
	FSpriterSCON DataModel = FSpriterSCON();
	if (DescriptorObject.IsValid())
	{
		DataModel.ParseFromJSON(DescriptorObject, NameForErrors, /*bSilent=*/ false, /*bPreParseOnly=*/ false);
	}

	// Create the new 'hub' asset and convert the data model over
	if (DataModel.IsValid())
	{
		const bool bSilent = false;

		Result = NewObject<USpriterImportData>(InParent, InName, Flags);
		Result->ImportedData = DataModel;
		Result->PixelsPerUnrealUnit = GetDefault<UPaperImporterSettings>()->GetDefaultPixelsPerUnrealUnit();
		Result->Modify();

		// Create Default Character Map
		const FString TargetCharacterMapPath = LongPackagePath / TEXT("Character Maps");
		const FString CharacterMapName = InName.ToString() + TEXT(" Default");
		USpriterCharacterMap* DefaultCharacterMap = CastChecked<USpriterCharacterMap>(CreateNewAsset(USpriterCharacterMap::StaticClass(), TargetCharacterMapPath, CharacterMapName, Flags));
		// Import the assets in the folders
		for (FSpriterFolder& Folder : DataModel.Folders)
		{
			for (FSpriterFile& File : Folder.Files)
			{
				const FString RelativeFilename = File.Name.Replace(TEXT("\\"), TEXT("/"), ESearchCase::CaseSensitive);
				const FString SourceSpriterFilePath = FPaths::Combine(*CurrentSourcePath, *RelativeFilename);

				FString RelativeDestPath;
				FString JustFilename;
				FString JustExtension;
				FPaths::Split(RelativeFilename, /*out*/ RelativeDestPath, /*out*/ JustFilename, /*out*/ JustExtension);

				if (File.FileType == ESpriterFileType::Sprite)
				{
					FString TargetTexturePath;
					if (RelativeDestPath.IsEmpty())
					{
						TargetTexturePath = LongPackagePath / TEXT("Textures");
					}
					else
					{
						TargetTexturePath = LongPackagePath / TEXT("Textures") / RelativeDestPath;
					}

					FString TargetSpritePath; 
					if (RelativeDestPath.IsEmpty())
					{
						TargetSpritePath = LongPackagePath / TEXT("Sprites");
					}
					else
					{
						TargetSpritePath = LongPackagePath / TEXT("Sprites") / RelativeDestPath;
					}

					// Import the texture
					UTexture2D* ImportedTexture = ImportTexture(SourceSpriterFilePath, TargetTexturePath);

					if (ImportTexture == nullptr)
					{
						SPRITER_IMPORT_ERROR(TEXT("Failed to import texture '%s' while importing '%s'"), *SourceSpriterFilePath, *CurrentFilename);
					}

					// Create a sprite from it
					UPaperSprite* ImportedSprite = CastChecked<UPaperSprite>(CreateNewAsset(UPaperSprite::StaticClass(), TargetSpritePath, JustFilename, Flags));
					
					FSpriterCharacterMapEntry Entry = FSpriterCharacterMapEntry();
					Entry.AssociatedSprite = ImportedSprite->GetName();
					Entry.ResultSprite = ImportedSprite;
					DefaultCharacterMap->Entrys.Add(Entry);

					const ESpritePivotMode::Type PivotMode = ConvertNormalizedPivotPointToPivotMode(File.PivotX, File.PivotY);
					const float PivotInPixelsX = File.Width * File.PivotX;
					const float PivotInPixelsY = File.Height * (1.0f-File.PivotY);

					ImportedSprite->SetPivotMode(PivotMode, FVector2D(PivotInPixelsX, PivotInPixelsY));

					FSpriteAssetInitParameters SpriteInitParams;
					SpriteInitParams.SetTextureAndFill(ImportedTexture);
					GetDefault<UPaperImporterSettings>()->ApplySettingsForSpriteInit(SpriteInitParams);
					ImportedSprite->InitializeSprite(SpriteInitParams);
				}
				else if (File.FileType == ESpriterFileType::Sound)
				{
					// Import the sound
					const FString TargetAssetPath = LongPackagePath / RelativeDestPath;
					UObject* ImportedSound = ImportAsset(SourceSpriterFilePath, TargetAssetPath);
				}
				else if (File.FileType != ESpriterFileType::INVALID)
				{
					ensureMsgf(false, TEXT("Importer was not updated when a new entry was added to ESpriterFileType"));
				}
					// 		TMap<FString, class UTexture2D*> ImportedTextures;
					// 		TMap<FString, class UPaperSprite> ImportedSprites;

			}
		}



		Result->PostEditChange();
	}
 	else
 	{
 		// Failed to parse the JSON
 		bLoadedSuccessfully = false;
 	}

	if (Result != nullptr)
	{
		//@TODO: Need to do this
		// Store the current file path and timestamp for re-import purposes
// 		UAssetImportData* ImportData = UTileMapAssetImportData::GetImportDataForTileMap(Result);
// 		ImportData->SourceFilePath = FReimportManager::SanitizeImportFilename(CurrentFilename, Result);
// 		ImportData->SourceFileTimestamp = IFileManager::Get().GetTimeStamp(*CurrentFilename).ToString();
	}

	FEditorDelegates::OnAssetPostImport.Broadcast(this, Result);

	return Result;
}

TSharedPtr<FJsonObject> USpriterImportFactory::ParseJSON(const FString& FileContents, const FString& NameForErrors, bool bSilent)
{
	// Load the file up (JSON format)
	if (!FileContents.IsEmpty())
	{
		const TSharedRef< TJsonReader<> >& Reader = TJsonReaderFactory<>::Create(FileContents);

		TSharedPtr<FJsonObject> DescriptorObject;
		if (FJsonSerializer::Deserialize(Reader, /*out*/ DescriptorObject) && DescriptorObject.IsValid())
		{
			// File was loaded and deserialized OK!
			return DescriptorObject;
		}
		else
		{
			if (!bSilent)
			{
				//@TODO: PAPER2D: How to correctly surface import errors to the user?
				UE_LOG(LogSpriterImporter, Warning, TEXT("Failed to parse Spriter SCON file '%s'.  Error: '%s'"), *NameForErrors, *Reader->GetErrorMessage());
			}
			return nullptr;
		}
	}
	else
	{
		if (!bSilent)
		{
			UE_LOG(LogSpriterImporter, Warning, TEXT("Spriter SCON file '%s' was empty.  This Spriter character cannot be imported."), *NameForErrors);
		}
		return nullptr;
	}
}

UObject* USpriterImportFactory::CreateNewAsset(UClass* AssetClass, const FString& TargetPath, const FString& DesiredName, EObjectFlags Flags)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	// Create a unique package name and asset name for the frame
	const FString TentativePackagePath = PackageTools::SanitizePackageName(TargetPath + TEXT("/") + DesiredName);
	FString DefaultSuffix;
	FString AssetName;
	FString PackageName;
	AssetToolsModule.Get().CreateUniqueAssetName(TentativePackagePath, DefaultSuffix, /*out*/ PackageName, /*out*/ AssetName);

	// Create a package for the asset
	UObject* OuterForAsset = CreatePackage(nullptr, *PackageName);

	// Create a frame in the package
	UObject* NewAsset = NewObject<UObject>(OuterForAsset, AssetClass, *AssetName, Flags);
	FAssetRegistryModule::AssetCreated(NewAsset);

	NewAsset->Modify();
	return NewAsset;
}

UObject* USpriterImportFactory::ImportAsset(const FString& SourceFilename, const FString& TargetSubPath)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<FString> FileNames;
	FileNames.Add(SourceFilename);

	TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssets(FileNames, TargetSubPath);
	return (ImportedAssets.Num() > 0) ? ImportedAssets[0] : nullptr;
}


UTexture2D* USpriterImportFactory::ImportTexture(const FString& SourceFilename, const FString& TargetSubPath)
{
	UTexture2D* ImportedTexture = Cast<UTexture2D>(ImportAsset(SourceFilename, TargetSubPath));

	if (ImportedTexture != nullptr)
	{
		// Change the compression settings
		GetDefault<UPaperImporterSettings>()->ApplyTextureSettings(ImportedTexture);
	}

	return ImportedTexture;
}

//////////////////////////////////////////////////////////////////////////

#undef SPRITER_IMPORT_ERROR
#undef LOCTEXT_NAMESPACE