// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/SceneComponent.h"
#include "SpriterImportData.h"
#include "SpriterCharacterMap.h"
#include "PaperSpriteComponent.h"
#include "SpriterSkeletonComponent.generated.h"

class USpriterSkeletonComponent;

UENUM(BlueprintType)
enum class ESpriterAnimationState : uint8
{
	NONE,
	PLAYING,
	BLENDING
};

USTRUCT(BlueprintType)
struct SPRITER_API FSpriterBoneInstance
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		bool IsActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		FString ParentBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		FTransform RelativeTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		FTransform WorldTransform;

	FSpriterBoneInstance();
};

USTRUCT(BlueprintType)
struct SPRITER_API FSpriterSpriteInstance
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		bool IsActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		FString ParentBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		int32 ZIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		FTransform RelativeTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		FTransform WorldTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		UPaperSpriteComponent* SpriteComponent;

	FSpriterSpriteInstance();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAnimationEnded, USpriterSkeletonComponent*, Skeleton, const FSpriterAnimation&, EndedAnimation, const bool, WasForced);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAnimationStarted, USpriterSkeletonComponent*, Skeleton, const FSpriterAnimation&, StartedAnimation, const bool, FirstTime);

UCLASS( ClassGroup=(Spriter), meta=(BlueprintSpawnableComponent))
class SPRITER_API USpriterSkeletonComponent : public USceneComponent
{
	GENERATED_BODY()

public:	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		USpriterImportData* Skeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		USpriterCharacterMap* CharacterMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		TArray<FSpriterBoneInstance> Bones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		TArray<FSpriterSpriteInstance> Sprites;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		ESpriterAnimationState AnimationState;

	UPROPERTY(BlueprintAssignable, Category = "Spriter")
		FAnimationStarted OnAnimationStarted;

	UPROPERTY(BlueprintAssignable, Category = "Spriter")
		FAnimationEnded OnAnimationEnded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		float CurrentTimeMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		float CurrentBlendTimeMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		float BlendDurationMS;

	// The Active Entity
	FSpriterEntity* ActiveEntity;

	// The Active Animation
	FSpriterAnimation* ActiveAnimation;

	// The Animation to Blend to
	FSpriterAnimation* NextAnimation;

	// Sets default values for this component's properties
	USpriterSkeletonComponent();

	// Checks to see if we need to be Initialized
	virtual void BeginPlay() override;
	
	// Update the Animation based on the State
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;


	//Gameplay Functions

	// Sets Skeleton if not the same, and populates Bone's and Sprite's arrays
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void SetSkeleton(USpriterImportData* NewSkeleton);

	// Sets Active Entity if not the same, and populates Bone's and Sprite's arrays
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void SetActiveEntity(int32 EntityIndex);

	// Sets Active Entity if not the same, and populates Bone's and Sprite's arrays
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void SetActiveEntityByName(const FString& EntityName);

	// Set Character Map for Skeleton
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void SetCharacterMap(USpriterCharacterMap* Map);

	// Apply's Character Map over Current Character Map, with wether or not to Create New Entrys if pairs not found
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void ApplyCharacterMap(USpriterCharacterMap* Map, bool bCreateNewEntrys);

	// Play the animation that Animation Name references, with an optional Blend Duration
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void PlayAnimation(const FString& AnimationName, float BlendLengthMS);

	// Resumes an animation if, we still have a Current Animation
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void ResumeAnimation();

	// Stops an animation
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void StopAnimation();

	// Populates Sprite's and Bone's arrays
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void InitSkeleton();

	// Sets the Skeleton to the First Key of the First Animation
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void SetToSetupPose();

	// Update the Bones
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void UpdateBones();

	// Update the Sprites
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void UpdateSprites();

	// Clear Bone's and Sprite's arrays and destroy all Sprite Components
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void CleanupObjects();

	// Check if we have a Skeleton Asset, and if all Sprite and Bones instances have been created for it. If bShouldInit is true, this will always return true.
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		bool IsInitialized(bool bShouldInit);


	// Blueprint Data Grabbers
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetEntity(int32 EntityIndex, FSpriterEntity& Entity);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetEntityByName(const FString& EntityName, FSpriterEntity& Entity);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetActiveEntity(FSpriterEntity& Entity);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetAnimation(int32 AnimationIndex, FSpriterAnimation& Animation);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetAnimationByName(const FString& AnimationName, FSpriterAnimation& Animation);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetActiveAnimation(FSpriterAnimation& Animation);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetNextAnimation(FSpriterAnimation& Animation);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetTimeline(UPARAM(ref)FSpriterAnimation& Animation, int32 TimelineIndex, FSpriterTimeline& Timeline);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetTimelineByName(UPARAM(ref)FSpriterAnimation& Animation, const FString& TimelineName, FSpriterTimeline& Timeline);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetBoneRef(UPARAM(ref)FSpriterAnimation& Animation, UPARAM(ref)FSpriterMainlineKey& Key, const FString& BoneName, FSpriterRef& BoneRef);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetObjectRef(UPARAM(ref)FSpriterAnimation& Animation, UPARAM(ref)FSpriterMainlineKey& Key, const FString& ObjectName, FSpriterObjectRef& ObjectRef);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetObjectInfo(int32 ObjectIndex, FSpriterObjectInfo& ObjectInfo);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetObjectInfoByName(const FString& ObjectName, FSpriterObjectInfo& ObjectInfo);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetFile(int32 FolderIndex, int32 FileIndex, FSpriterFile& File);


	// Blueprint Instance Grabbers
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetBone(const FString& BoneName, FSpriterBoneInstance& Bone);

	UFUNCTION(BlueprintCallable, Category = "Spriter")
		void GetSprite(const FString& SpriteName, FSpriterSpriteInstance& Sprite);


	// Get Sprite from Character Map based on Name of passed in File
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		UPaperSprite* GetSpriteFromCharacterMap(UPARAM(ref)FSpriterFile& File);

	// Convert Seconds to Milli-Seconds
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		static int32 ToMS(float Seconds);

	// Convert Milli-Seconds to Seconds
	UFUNCTION(BlueprintCallable, Category = "Spriter")
		static float ToSeconds(int32 MS);


	// C++ Data Grabbers
	FSpriterEntity* GetEntity(int32 EntityIndex);

	FSpriterEntity* GetEntity(const FString& EntityName);

	FSpriterAnimation* GetAnimation(int32 AnimationIndex);

	FSpriterAnimation* GetAnimation(const FString& AnimationName);

	FSpriterTimeline* GetTimeline(FSpriterAnimation& Animation, int32 TimelineIndex);

	FSpriterTimeline* GetTimeline(FSpriterAnimation& Animation, const FString& TimelineName);

	FSpriterRef* GetBoneRef(FSpriterAnimation& Animation, FSpriterMainlineKey& Key, const FString& BoneName);

	FSpriterObjectRef* GetObjectRef(FSpriterAnimation& Animation, FSpriterMainlineKey& Key, const FString& ObjectName);

	FSpriterObjectInfo* GetObjectInfo(int32 ObjectIndex);

	FSpriterObjectInfo* GetObjectInfo(const FString& ObjectName);

	FSpriterFile* GetFile(int32 Folder, int32 File);


	// C++ Instance Grabbers
	FSpriterBoneInstance* GetBone(const FString& BoneName);

	FSpriterSpriteInstance* GetSprite(const FString& SpriteName);

	// Amount to offset Sprites according to thier ZIndex
	static const float SPRITER_ZOFFSET;

protected:

	// The Actor that owns this Component
	AActor* Owner;


	// Animation Dependant
	bool bAnimationWasStarted;

	TArray<FSpriterMainlineKey*>* GetMainlineKeys();

	TArray<FSpriterFatTimelineKey*>* GetTimelineKeys(const FString& TimelineName);
};