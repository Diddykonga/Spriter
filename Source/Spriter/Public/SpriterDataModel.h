// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpriterDataModel.generated.h"

// This file contains the definition of various Spriter data types
// Reference http://www.brashmonkey.com/ScmlDocs/ScmlReference.html and a bunch of exported files

//////////////////////////////////////////////////////////////////////////

struct FSpriterSCON;
struct FSpriterObjectInfo;
struct FSpriterAnimation;
struct FSpriterTimeline;
struct FSpriterFatTimelineKey;

UENUM()
enum class ESpriterObjectType : uint8
{
	INVALID,
	/** Default when not specified */
	Sprite,
	Bone,
	Box,
	Point,
	Sound, //?
	Entity, //?
	Variable, //?
	Event
};

UENUM()
enum class ESpriterCurveType : uint8
{
	INVALID,
	Instant,
	/** Default when not specified */
	Linear,
	Quadratic,
	Cubic
};

UENUM()
enum class ESpriterVariableType : uint8
{
	INVALID,
	Float,
	Integer,
	String
};

UENUM()
enum class ESpriterFileType : uint8
{
	INVALID,
	Sprite,
	Sound
};

struct SPRITER_API  FSpriterEnumHelper
{
public:
	static ESpriterObjectType StringToObjectType(const FString& InString);
	static ESpriterCurveType StringToCurveType(const FString& InString);
	static ESpriterVariableType StringToVariableType(const FString& InString);
	static ESpriterFileType StringToFileType(const FString& InString);

private:
	FSpriterEnumHelper() {}
};

//////////////////////////////////////////////////////////////////////////
// FSpriterSpatialInfo

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterSpatialInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float Y;

	// Angle (in degrees)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float AngleInDegrees;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float ScaleX;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float ScaleY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FLinearColor Color;

public:
	FSpriterSpatialInfo();

	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);

	FTransform ConvertToTransform() const;
};

//////////////////////////////////////////////////////////////////////////
// FSpriterFile

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterFile
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float PivotX;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float PivotY;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 Width;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 Height;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	ESpriterFileType FileType;

public:
	FSpriterFile();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterFolder

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterFolder
{
public:
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterFile> Files;

public:
	FSpriterFolder();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterMapInstruct SPRITER_API ion

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterMapInstruction
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 Folder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 File;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 TargetFolder;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 TargetFile;

public:
	FSpriterMapInstruction();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterTagLineKey

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterTagLineKey
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 TimeInMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<int32> Tags;
	
public:
	FSpriterTagLineKey();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterTagLine

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterTagLine
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterTagLineKey> Keys;

public:
 	FSpriterTagLine();
 	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterValLineKey

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterValLineKey
{
public:
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 TimeInMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	bool bReadAsNumber;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float ValueAsNumber;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString ValueAsString;

public:
	FSpriterValLineKey();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterValLine

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterValLine
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterValLineKey> Keys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 DefinitionIndex;

public:
	FSpriterValLine();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterMeta

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterMeta
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterTagLine> TagLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterValLine> ValLines;

public:
	FSpriterMeta();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterRefCommon

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterRefCommon
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 ParentTimelineIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 TimelineIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 KeyIndex;

public:
	FSpriterRefCommon();
	bool ParseCommonFromJSON(FSpriterSCON* Owner, FSpriterAnimation* Animation, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterRef

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterRef : public FSpriterRefCommon
{
public:
	GENERATED_USTRUCT_BODY()

	bool ParseFromJSON(FSpriterSCON* Owner, FSpriterAnimation* Animation, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterObjectRef

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterObjectRef : public FSpriterRefCommon
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 ZIndex;

public:
	FSpriterObjectRef();
	bool ParseFromJSON(FSpriterSCON* Owner, FSpriterAnimation* Animation, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterMainlineKey

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterMainlineKey
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 TimeInMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterRef> BoneRefs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterObjectRef> ObjectRefs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	ESpriterCurveType CurveType;

public:
	FSpriterMainlineKey();
	bool ParseFromJSON(FSpriterSCON* Owner, FSpriterAnimation* Animation, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterTimelineKey

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterTimelineKey
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 TimeInMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	ESpriterCurveType CurveType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float C1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float C2;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 Spin;

public:
	FSpriterTimelineKey();
	bool ParseBasicsFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterFatTimelineKey

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterFatTimelineKey : public FSpriterTimelineKey
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FSpriterSpatialInfo Info;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 FolderIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 FileIndex;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float PivotX;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float PivotY;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	bool bUseDefaultPivot;

	// Overrides linear!
public:
	FSpriterFatTimelineKey();

	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent, const ESpriterObjectType ObjectType);
	bool ParseBoneFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
	bool ParseObjectFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent, const ESpriterObjectType ObjectType);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterTimeline

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterTimeline
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spriter")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 ObjectInfoIndex;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	ESpriterObjectType ObjectType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterFatTimelineKey> Keys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FSpriterMeta Metadata;

public:
	FSpriterTimeline();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterEventLineKey

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterEventLineKey
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 TimeInMS;

public:
	FSpriterEventLineKey();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterEventLine

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterEventLine
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterEventLineKey> Keys;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 ObjectIndex;

public:
	FSpriterEventLine();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterAnimation

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterAnimation
{
public:
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 LengthInMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 IntervalInMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	bool bIsLooping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FSpriterMeta Metadata;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterMainlineKey> MainlineKeys;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterTimeline> Timelines;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterEventLine> EventLines;

public:
	FSpriterAnimation();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterCharacterMapData

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterCharacterMapData
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterMapInstruction> Maps;

public:
	FSpriterCharacterMapData();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterVariableDefinition

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterVariableDefinition
{
public:
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	ESpriterVariableType VariableType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float DefaultValueNumber;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString DefaultValueString;

public:
	FSpriterVariableDefinition();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterObjectInfo

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterObjectInfo
{
public:
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString Name;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 Width;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	int32 Height;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float PivotX;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	float PivotY;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	ESpriterObjectType ObjectType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterVariableDefinition> VariableDefinitions;

public:
	FSpriterObjectInfo();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterEntity

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterEntity
{
public:
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterCharacterMapData> CharacterMaps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterAnimation> Animations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterObjectInfo> Objects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterVariableDefinition> VariableDefinitions;

public:
	FSpriterEntity();
	bool ParseFromJSON(FSpriterSCON* Owner, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent);
};

//////////////////////////////////////////////////////////////////////////
// FSpriterSCON

USTRUCT(BlueprintType)
struct SPRITER_API  FSpriterSCON
{
public:
	GENERATED_USTRUCT_BODY()
		
	//"generator" : "BrashMonkey Spriter",
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString Generator;

	//"generator_version" : "r2",
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString GeneratorVersion;

	//"scon_version" : "1.0"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	FString SconVersion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterFolder> Folders;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterEntity> Entities;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FString> Tags;

	bool bSuccessfullyParsed;

public:
	FSpriterSCON();

	void ParseFromJSON(TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent, bool bPreparseOnly);

	bool IsValid() const;
};

