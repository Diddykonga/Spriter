// Fill out your copyright notice in the Description page of Project Settings.

#include "SpriterPrivatePCH.h"
#include "SpriterSkeletonComponent.h"

// Static's Initialization
const float USpriterSkeletonComponent::SPRITER_ZOFFSET = 3.0f;

USpriterSkeletonComponent::USpriterSkeletonComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	Owner = GetOwner();

	// ...
}

FSpriterBoneInstance::FSpriterBoneInstance()
	: Name("")
	, ParentBoneName("")
	, RelativeTransform()
	, WorldTransform()
{
}

FSpriterSpriteInstance::FSpriterSpriteInstance()
	: IsActive(true)
	, Name("")
	, ParentBoneName("")
	, RelativeTransform()
	, WorldTransform()
	, ZIndex(0)
	, SpriteComponent(nullptr)
{
}

void USpriterSkeletonComponent::BeginPlay()
{
	Super::BeginPlay();

	// Init our default Skeleton
	IsInitialized(true);
}

void USpriterSkeletonComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	if (IsInitialized(true))
	{
		//Update our Blending to the Next Animation
		if (AnimationState == ESpriterAnimationState::BLENDING)
		{
			if (CurrentAnimation && NextAnimation)
			{
				if (CurrentBlendTimeMS >= BlendDurationMS)
				{
					CurrentTimeMS = 0.f;
					CurrentBlendTimeMS = 0.f;
					BlendDurationMS = 0.f;
					CurrentAnimation = NextAnimation;
					NextAnimation = nullptr;
					AnimationState = ESpriterAnimationState::PLAYING;

					UpdateBones();
					UpdateSprites();

					bAnimationWasStarted = true;

					return;
				}
				else
				{
					UpdateBones();
					UpdateSprites();

					CurrentBlendTimeMS = FMath::Min<int32>(BlendDurationMS, (CurrentBlendTimeMS + ToMS(DeltaTime)));
				}
			}
		}
		// Update our Playing of our Current Animation
		else if (AnimationState == ESpriterAnimationState::PLAYING)
		{
			if (CurrentAnimation)
			{
				if (CurrentTimeMS >= CurrentAnimation->LengthInMS)
				{
					UpdateBones();
					UpdateSprites();

					OnAnimationEnded.Broadcast(*CurrentAnimation, false);

					if (CurrentAnimation->bIsLooping)
					{
						CurrentTimeMS = 0;
					}
					else
					{
						AnimationState = ESpriterAnimationState::NONE;

						OnAnimationEnded.Broadcast(*CurrentAnimation, false);
						return;
					}
				}
				else
				{
					UpdateBones();
					UpdateSprites();

					if (CurrentTimeMS == 0)
					{
						bool FirstTime = bAnimationWasStarted;
						OnAnimationStarted.Broadcast(*CurrentAnimation, FirstTime);

						bAnimationWasStarted = false;
					}

					CurrentTimeMS = FMath::Min<int32>(CurrentAnimation->LengthInMS, (CurrentTimeMS + ToMS(DeltaTime)));
				}
			}
		}
	}
}

// Gameplay Functions
void USpriterSkeletonComponent::SetSkeleton(USpriterImportData * NewSkeleton)
{
	if (!NewSkeleton || NewSkeleton == Skeleton)
	{
		return;
	}

	if (IsInitialized(false))
	{
		CleanupObjects();
	}

	Skeleton = NewSkeleton;
	InitSkeleton();
}

void USpriterSkeletonComponent::SetCharacterMap(USpriterCharacterMap * Map)
{
	if (Skeleton && Map)
	{
		if (Map != CharacterMap)
		{
			CharacterMap = Map;
		}
	}
}

void USpriterSkeletonComponent::ApplyCharacterMap(USpriterCharacterMap * Map, bool bCreateNewEntrys)
{
	if (Skeleton && Map)
	{
		bool bFoundEntry;

		for (FSpriterCharacterMapEntry& NewEntry : Map->Entrys)
		{
			bFoundEntry = false;

			for (FSpriterCharacterMapEntry& CurrentEntry : CharacterMap->Entrys)
			{
				if (CurrentEntry.AssociatedSprite.Equals(NewEntry.AssociatedSprite, ESearchCase::IgnoreCase))
				{
					CurrentEntry.ResultSprite = NewEntry.ResultSprite;
					bFoundEntry = true;
				}
			}

			if(!bFoundEntry && bCreateNewEntrys)
			{
				CharacterMap->Entrys.Add(NewEntry);
			}
		}

		UpdateSprites();
	}
}

void USpriterSkeletonComponent::PlayAnimation(const FString& AnimationName, float BlendLengthMS)
{
	if (IsInitialized(true) && !AnimationName.IsEmpty() && BlendLengthMS >= 0)
	{
		if(BlendLengthMS > 0)
		{
			if (!CurrentAnimation)
			{
				SetToSetupPose();

				NextAnimation = GetAnimation(AnimationName);
				CurrentBlendTimeMS = 0.f;
				BlendDurationMS = BlendLengthMS;
				AnimationState = ESpriterAnimationState::BLENDING;
			}
			else
			{
				NextAnimation = GetAnimation(AnimationName);
				CurrentBlendTimeMS = 0.f;
				BlendDurationMS = BlendLengthMS;
				AnimationState = ESpriterAnimationState::BLENDING;

				OnAnimationEnded.Broadcast(*CurrentAnimation, true);
			}
		}
		else
		{
			if (CurrentAnimation)
			{
				OnAnimationEnded.Broadcast(*CurrentAnimation, true);
			}

			CurrentAnimation = GetAnimation(AnimationName);
			NextAnimation = nullptr;
			CurrentTimeMS = 0.f;
			CurrentBlendTimeMS = 0.f;
			BlendDurationMS = 0.f;
			AnimationState = ESpriterAnimationState::PLAYING;

			bAnimationWasStarted = true;
		}
	}
}

void USpriterSkeletonComponent::ResumeAnimation()
{
	if (IsInitialized(true) && CurrentAnimation)
	{	
		if (AnimationState == ESpriterAnimationState::NONE)
		{
			if (CurrentTimeMS <= CurrentAnimation->LengthInMS || CurrentAnimation->bIsLooping)
			{
				if (CurrentTimeMS >= CurrentAnimation->LengthInMS)
				{
				CurrentTimeMS = 0.f;
				}
				AnimationState = ESpriterAnimationState::PLAYING;
			}
		}
	}
}

void USpriterSkeletonComponent::StopAnimation()
{
	AnimationState = ESpriterAnimationState::NONE;
}

int32 USpriterSkeletonComponent::ToMS(float Seconds)
{
	return FMath::FloorToInt( Seconds * 1000);
}

float USpriterSkeletonComponent::ToSeconds(int32 MS)
{
	return MS/1000;
}

void USpriterSkeletonComponent::InitSkeleton()
{

	if (Skeleton)
	{
		FSpriterEntity* Entity = GetEntity();
		FSpriterAnimation* FirstAnimation = GetAnimation(0);

		// Loop through Object Infos, and create Bones and Sprites for Skeleton
		if (Entity && FirstAnimation)
		{
			for (FSpriterObjectInfo& Obj : Entity->Objects)
			{
				if (Obj.ObjectType == ESpriterObjectType::Bone)
				{
					FSpriterBoneInstance Bone = FSpriterBoneInstance();
					Bone.Name = Obj.Name;

					for (FSpriterRef& BoneRef : FirstAnimation->MainlineKeys[0].BoneRefs)
					{
						if (BoneRef.ParentTimelineIndex != INDEX_NONE && BoneRef.TimelineIndex != INDEX_NONE)
						{
							FSpriterTimeline* Timeline = GetTimeline(*FirstAnimation, BoneRef.TimelineIndex);
							FSpriterTimeline* ParentTimeline = GetTimeline(*FirstAnimation, BoneRef.ParentTimelineIndex);
							if (Timeline && ParentTimeline)
							{
								if (Timeline->Name.Equals(Obj.Name, ESearchCase::IgnoreCase))
								{
									Bone.ParentBoneName = ParentTimeline->Name;
								}
							}
						}
					}

					Bones.Add(Bone);
				}
				else if (Obj.ObjectType == ESpriterObjectType::Sprite)
				{
					FSpriterSpriteInstance Sprite = FSpriterSpriteInstance();
					Sprite.Name = Obj.Name;

					for (FSpriterObjectRef& SpriteRef : FirstAnimation->MainlineKeys[0].ObjectRefs)
					{
						if (SpriteRef.ParentTimelineIndex != INDEX_NONE && SpriteRef.TimelineIndex != INDEX_NONE)
						{
							FSpriterTimeline* Timeline = GetTimeline(*FirstAnimation, SpriteRef.TimelineIndex);
							FSpriterTimeline* ParentTimeline = GetTimeline(*FirstAnimation, SpriteRef.ParentTimelineIndex);

							if (Timeline->Name.Equals(Obj.Name, ESearchCase::IgnoreCase))
							{
								Sprite.ParentBoneName = ParentTimeline->Name;
							}
						}
					}

					Sprite.SpriteComponent = NewObject<UPaperSpriteComponent>((UObject*)Owner, FName(*Obj.Name));
					Sprite.SpriteComponent->AttachTo(this);
					Sprite.SpriteComponent->bWantsBeginPlay = true;
					Sprite.SpriteComponent->RegisterComponent();

					Sprites.Add(Sprite);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_InitSkeleton() : Tried to initialize without a Skeleton!"));
	}
}

void USpriterSkeletonComponent::SetToSetupPose()
{
	if (IsInitialized(true))
	{
		AnimationState = ESpriterAnimationState::PLAYING;
		CurrentBlendTimeMS = 0.f;
		BlendDurationMS = 0.f;
		CurrentTimeMS = 0.f;
		CurrentAnimation = GetAnimation(0);

		UpdateBones();
		UpdateSprites();

		AnimationState = ESpriterAnimationState::NONE;
	}
}

void USpriterSkeletonComponent::UpdateBones()
{
	if (IsInitialized(true))
	{
		float Alpha = 0.f;
		float C1 = 0.f;
		float C2 = 0.f;

		for (FSpriterBoneInstance& Bone : Bones)
		{
			TArray<FSpriterFatTimelineKey*>& Keys = *GetTimelineKeys(Bone.Name);
			if (Keys.Num() >= 2)
			{
				if (AnimationState == ESpriterAnimationState::BLENDING)
				{
					C1 = CurrentBlendTimeMS;
					C2 = BlendDurationMS;
				}
				else if (AnimationState == ESpriterAnimationState::PLAYING)
				{
					if (Keys[1]->TimeInMS == 0 && CurrentAnimation)
					{
						C1 = (CurrentTimeMS - Keys[0]->TimeInMS);
						C2 = (CurrentAnimation->LengthInMS - Keys[0]->TimeInMS);
					}
					else
					{
						C1 = (CurrentTimeMS - Keys[0]->TimeInMS);
						C2 = (Keys[1]->TimeInMS - Keys[0]->TimeInMS);
					}
				}

				if (Keys[0]->TimeInMS == Keys[1]->TimeInMS && AnimationState == ESpriterAnimationState::PLAYING)
				{
					Alpha = 0;
				}
				else if (C2 == 0)
				{
					Alpha = 0;
				}
				else
				{
					Alpha = C1 / C2;
				}

				FTransform RelativeTransform = FTransform();
				FTransform FirstTransform = Keys[0]->Info.ConvertToTransform();
				FTransform SecondTransform = Keys[1]->Info.ConvertToTransform();
				float SpinCorrection = 0;
				if (Keys[0]->Spin > 0 && (FirstTransform.Rotator().Pitch > SecondTransform.Rotator().Pitch))
				{
					SpinCorrection = 360;
				}
				else if (Keys[0]->Spin < 0 && (SecondTransform.Rotator().Pitch > FirstTransform.Rotator().Pitch))
				{
					SpinCorrection = -360;
				}

				RelativeTransform.SetLocation(FMath::Lerp(FirstTransform.GetLocation(), SecondTransform.GetLocation(), Alpha));
				RelativeTransform.SetRotation(FQuat::Slerp_NotNormalized(FirstTransform.GetRotation(), (SecondTransform.Rotator() + FRotator(SpinCorrection, 0, 0)).Quaternion(), Alpha));
				RelativeTransform.SetScale3D(FMath::Lerp(FirstTransform.GetScale3D(), SecondTransform.GetScale3D(), Alpha));

				Bone.RelativeTransform = RelativeTransform;
				Bone.RelativeTransform.SetLocation(Bone.RelativeTransform.GetLocation() / Skeleton->PixelsPerUnrealUnit);
				if (!Bone.ParentBoneName.IsEmpty())
				{
					FSpriterBoneInstance* Parent = GetBone(Bone.ParentBoneName);
					if (Parent)
					{
						FTransform::Multiply(&Bone.WorldTransform, &Bone.RelativeTransform, &Parent->WorldTransform);
					}
				}
				else
				{
					Bone.WorldTransform = RelativeTransform;
				}
			}
		}
	}
}

void USpriterSkeletonComponent::UpdateSprites()
{
	if (IsInitialized(true))
	{
		TArray<FSpriterMainlineKey*>& MainKeys = *GetMainlineKeys();
		float Alpha = 0.f;
		float C1 = 0.f;
		float C2 = 0.f;

		for (FSpriterSpriteInstance& Sprite : Sprites)
		{
			TArray<FSpriterFatTimelineKey*>& Keys = *GetTimelineKeys(Sprite.Name);
			if (Keys.Num() >= 2)
			{
				if (AnimationState == ESpriterAnimationState::BLENDING)
				{
					C1 = CurrentBlendTimeMS;
					C2 = BlendDurationMS;
				}
				else if (AnimationState == ESpriterAnimationState::PLAYING)
				{
					if (Keys[1]->TimeInMS == 0 && CurrentAnimation)
					{
						C1 = (CurrentTimeMS - Keys[0]->TimeInMS);
						C2 = (CurrentAnimation->LengthInMS - Keys[0]->TimeInMS);
					}
					else
					{
						C1 = (CurrentTimeMS - Keys[0]->TimeInMS);
						C2 = (Keys[1]->TimeInMS - Keys[0]->TimeInMS);
					}
				}

				if (Keys[0]->TimeInMS == Keys[1]->TimeInMS && AnimationState == ESpriterAnimationState::PLAYING)
				{
					Alpha = 0;
				}
				else if (C2 == 0)
				{
					Alpha = 0;
				}
				else
				{
					Alpha = C1 / C2;
				}

				// Update Sprite From Mainline
				if (MainKeys.Num() >= 2)
				{
					FSpriterObjectRef* Ref;
					if (AnimationState == ESpriterAnimationState::BLENDING)
					{
						if (Alpha == 1)
						{
							Ref = GetObjectRef(*NextAnimation, *MainKeys[1], Sprite.Name);
						}
						else
						{
							Ref = GetObjectRef(*CurrentAnimation, *MainKeys[0], Sprite.Name);
						}
					}
					else
					{
						if (Alpha == 1)
						{
							Ref = GetObjectRef(*CurrentAnimation, *MainKeys[1], Sprite.Name);
						}
						else
						{
							Ref = GetObjectRef(*CurrentAnimation, *MainKeys[0], Sprite.Name);
						}
					}

					if (Ref)
					{
						Sprite.IsActive = true;
						Sprite.SpriteComponent->Activate(true);

						if (Sprite.IsActive)
						{
							FSpriterTimeline* ParentTimeline = nullptr;
							if (AnimationState == ESpriterAnimationState::BLENDING)
							{
								if (Alpha == 1)
								{
									ParentTimeline = GetTimeline(*NextAnimation, Ref->ParentTimelineIndex);
								}
							}
							else
							{
								ParentTimeline = GetTimeline(*CurrentAnimation, Ref->ParentTimelineIndex);
							}

							if (ParentTimeline)
							{
								Sprite.ParentBoneName = ParentTimeline->Name;
							}

							Sprite.ZIndex = Ref->ZIndex;
						}
					}
					else
					{
						Sprite.IsActive = false;
						Sprite.SpriteComponent->Activate(false);
					}
				}

				if (Sprite.IsActive)
				{
					FTransform RelativeTransform = FTransform();
					FTransform FirstTransform = Keys[0]->Info.ConvertToTransform();
					FTransform SecondTransform = Keys[1]->Info.ConvertToTransform();
					float SpinCorrection = 0;
					if (Keys[0]->Spin > 0 && (FirstTransform.Rotator().Pitch > SecondTransform.Rotator().Pitch))
					{
						SpinCorrection = 360;
					}
					else if (Keys[0]->Spin < 0 && (SecondTransform.Rotator().Pitch > FirstTransform.Rotator().Pitch))
					{
						SpinCorrection = -360;
					}

					RelativeTransform.SetLocation(FMath::Lerp(FirstTransform.GetLocation(), SecondTransform.GetLocation(), Alpha));
					RelativeTransform.SetRotation(FQuat::Slerp_NotNormalized(FirstTransform.GetRotation(), (SecondTransform.Rotator() + FRotator(SpinCorrection, 0, 0)).Quaternion(), Alpha));
					RelativeTransform.SetScale3D(FMath::Lerp(FirstTransform.GetScale3D(), SecondTransform.GetScale3D(), Alpha));

					Sprite.RelativeTransform = RelativeTransform;
					Sprite.RelativeTransform.SetLocation(Sprite.RelativeTransform.GetLocation() / Skeleton->PixelsPerUnrealUnit);
					if (!Sprite.ParentBoneName.IsEmpty())
					{
						FSpriterBoneInstance* Parent = GetBone(Sprite.ParentBoneName);
						if (Parent)
						{
							FTransform::Multiply(&Sprite.WorldTransform, &Sprite.RelativeTransform, &Parent->WorldTransform);
						}
					}
					else
					{
						Sprite.WorldTransform = RelativeTransform;
					}

					FTransform NewTransform = Sprite.WorldTransform;
					//NewTransform.AddToTranslation(PaperAxisZ * -(Sprite.ZIndex * SPRITER_ZOFFSET));
					Sprite.SpriteComponent->SetRelativeTransform(NewTransform);
					Sprite.SpriteComponent->SetTranslucentSortPriority(Sprite.ZIndex);

					UPaperSprite* PaperSprite = GetSpriteFromCharacterMap(*GetFile(Keys[0]->FolderIndex, Keys[0]->FileIndex));
					if (Sprite.SpriteComponent->GetSprite() != PaperSprite)
					{
						Sprite.SpriteComponent->SetSprite(PaperSprite);
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_UpdateSprites() : Couldnt Find 2 Timeline Keys!"));

				return;
			}
		}
	}
}

void USpriterSkeletonComponent::CleanupObjects()
{
	for (FSpriterSpriteInstance& Sprite : Sprites)
	{
		if (Sprite.SpriteComponent)
		{
			Sprite.SpriteComponent->DestroyComponent();
		}
	}

	Bones.Empty();
	Sprites.Empty();
}

// Blueprint Data Grabbers
void USpriterSkeletonComponent::GetEntity(FSpriterEntity& Entity)
{
	if (Skeleton  && Skeleton->ImportedData.Entities.Num() > 0)
	{
		Entity = Skeleton->ImportedData.Entities[0];
	}
}

void USpriterSkeletonComponent::GetAnimation(int32 AnimationIndex, FSpriterAnimation& Animation)
{
	if (Skeleton)
	{
		FSpriterEntity* Entity = GetEntity();
		if (Entity)
		{
			if (AnimationIndex >= 0 && AnimationIndex < Entity->Animations.Num())
			{
				Animation = Entity->Animations[AnimationIndex];
			}
		}
	}
}

void USpriterSkeletonComponent::GetAnimationByName(const FString& AnimationName, FSpriterAnimation& Animation)
{
	if (Skeleton && !AnimationName.IsEmpty())
	{
		FSpriterEntity* Entity = GetEntity();
		if (Entity)
		{
			for (FSpriterAnimation& Anim : Entity->Animations)
			{
				if (Anim.Name.Equals(AnimationName, ESearchCase::IgnoreCase))
				{
					Animation = Anim;
				}
			}
		}
	}
}

void USpriterSkeletonComponent::GetCurrentAnimation(FSpriterAnimation& Animation)
{
	if (CurrentAnimation)
	{
		Animation = *CurrentAnimation;
	}
}

void USpriterSkeletonComponent::GetNextAnimation(FSpriterAnimation& Animation)
{
	if (NextAnimation)
	{
		Animation = *NextAnimation;
	}
}

void USpriterSkeletonComponent::GetTimeline(UPARAM(ref)FSpriterAnimation& Animation, int32 TimelineIndex, FSpriterTimeline& Timeline)
{
	if (Skeleton && !Animation.Name.IsEmpty())
	{
		if (TimelineIndex >= 0 && TimelineIndex < Animation.Timelines.Num())
		{
			Timeline = Animation.Timelines[TimelineIndex];
		}
	}
}

void USpriterSkeletonComponent::GetTimelineByName(UPARAM(ref)FSpriterAnimation& Animation, const FString& Name, FSpriterTimeline& Timeline)
{
	if (Skeleton && !Animation.Name.IsEmpty() && !Name.IsEmpty())
	{
		for (FSpriterTimeline& TimeL : Animation.Timelines)
		{
			if (TimeL.Name.Equals(Name, ESearchCase::IgnoreCase))
			{
				Timeline = TimeL;
			}
		}
	}
}

void USpriterSkeletonComponent::GetObjectRef(UPARAM(ref)FSpriterAnimation& Animation, UPARAM(ref)FSpriterMainlineKey& Key, const FString& Name, FSpriterObjectRef& ObjectRef)
{
	if (Skeleton  && Key.TimeInMS != INDEX_NONE &&  !Name.IsEmpty())
	{
		for (FSpriterObjectRef& Ref : Key.ObjectRefs)
		{
			FSpriterTimeline* Timeline = GetTimeline(Animation, Ref.TimelineIndex);

			if (Timeline)
			{
				if (Timeline->Name.Equals(Name, ESearchCase::IgnoreCase))
				{
					ObjectRef = Ref;
				}
			}
		}
	}
}

void USpriterSkeletonComponent::GetObjectInfo(int32 ObjectIndex, FSpriterObjectInfo& ObjectInfo)
{
	if (Skeleton)
	{
		FSpriterEntity* Entity = GetEntity();
		if (Entity)
		{
			if (ObjectIndex >= 0 && ObjectIndex < Entity->Objects.Num())
			{
				ObjectInfo = Entity->Objects[ObjectIndex];
			}
		}
	}
}

void USpriterSkeletonComponent::GetObjectInfoByName(const FString & Name, FSpriterObjectInfo& ObjectInfo)
{
	if (Skeleton && !Name.IsEmpty())
	{
		FSpriterEntity* Entity = GetEntity();
		if (Entity)
		{
			for (FSpriterObjectInfo& Object : Entity->Objects)
			{
				if (Object.Name.Equals(Name, ESearchCase::IgnoreCase))
				{
					ObjectInfo = Object;
				}
			}
		}
	}
}

void USpriterSkeletonComponent::GetFile(int32 FolderIndex, int32 FileIndex, FSpriterFile& File)
{
	if (Skeleton)
	{
		if (FolderIndex >= 0 && FolderIndex < Skeleton->ImportedData.Folders.Num())
		{
			FSpriterFolder* ChosenFolder = &Skeleton->ImportedData.Folders[FolderIndex];
			if (ChosenFolder && FileIndex >= 0 && FileIndex < ChosenFolder->Files.Num())
			{
				File = ChosenFolder->Files[FileIndex];
			}
		}
	}
}

// Blueprint Instance Grabbers
void USpriterSkeletonComponent::GetBone(const FString& BoneName, FSpriterBoneInstance& Bone)
{
	if (Skeleton && !BoneName.IsEmpty())
	{
		for (FSpriterBoneInstance& Instance : Bones)
		{
			if (Instance.Name.Equals(BoneName, ESearchCase::IgnoreCase))
			{
				Bone = Instance;
			}
		}
	}
}

void USpriterSkeletonComponent::GetSprite(const FString& SpriteName, FSpriterSpriteInstance& Sprite)
{
	if (Skeleton && !SpriteName.IsEmpty())
	{
		for (FSpriterSpriteInstance& Instance : Sprites)
		{
			if (Instance.Name.Equals(SpriteName, ESearchCase::IgnoreCase))
			{
				Sprite = Instance;
			}
		}
	}
}

UPaperSprite* USpriterSkeletonComponent::GetSpriteFromCharacterMap(UPARAM(ref)FSpriterFile& File)
{
	if (File.Name.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_GetSpriteFromCharacterMap() : Invalid File!"));

		return nullptr;
	}

	if (Skeleton && CharacterMap)
	{
		TArray<FString> StringArray;
		TArray<FString> NameArray;
		FString AssociatedSprite;
		File.Name.ParseIntoArray(StringArray, TEXT("/"));
		StringArray.Last(0).ParseIntoArray(NameArray, TEXT("."));

		if (NameArray.Num() > 0)
		{
			AssociatedSprite = NameArray[0];
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_GetSpriteFromCharacterMap() : Couldnt Get Associated Name!"));

			return nullptr;
		}

		for (FSpriterCharacterMapEntry& Entry : CharacterMap->Entrys)
		{
			if (Entry.AssociatedSprite.Equals(AssociatedSprite, ESearchCase::IgnoreCase))
			{
				return Entry.ResultSprite;
			}
		}

	}

	return nullptr;
}

// Animation Dependant Grabbers
TArray<FSpriterMainlineKey*>* USpriterSkeletonComponent::GetMainlineKeys()
{
	if (Skeleton)
	{
		TArray<FSpriterMainlineKey*>* MainlineKeys  = new TArray<FSpriterMainlineKey*>();
		FSpriterMainlineKey* C1;
		FSpriterMainlineKey* C2;

		if (AnimationState == ESpriterAnimationState::BLENDING)
		{
			if (CurrentAnimation && NextAnimation)
			{
				for (FSpriterMainlineKey& Key : CurrentAnimation->MainlineKeys)
				{
					if (Key.TimeInMS <= CurrentTimeMS)
					{
						C1 = &Key;
					}
				}

				C2 = &NextAnimation->MainlineKeys[0];
			}
		}
		else if (AnimationState == ESpriterAnimationState::PLAYING)
		{
			if (CurrentAnimation)
			{
				for (int32 Key = 0; Key < CurrentAnimation->MainlineKeys.Num(); ++Key)
				{
					if (CurrentAnimation->MainlineKeys[Key].TimeInMS <= CurrentTimeMS)
					{
						C1 = &CurrentAnimation->MainlineKeys[Key];
						C2 = &CurrentAnimation->MainlineKeys[(Key + 1) % CurrentAnimation->MainlineKeys.Num()];
					}
				}
			}
		}

		MainlineKeys->Add(C1);
		MainlineKeys->Add(C2);

		return MainlineKeys;
	}

	return nullptr;
}

TArray<FSpriterFatTimelineKey*>* USpriterSkeletonComponent::GetTimelineKeys(const FString & Name)
{
	if (Skeleton && !Name.IsEmpty())
	{
		FSpriterTimeline* CurrentTimeline;
		FSpriterTimeline* NextTimeline;
		TArray<FSpriterFatTimelineKey*>* Keys = new TArray<FSpriterFatTimelineKey*>();
		FSpriterFatTimelineKey* C1;
		FSpriterFatTimelineKey* C2;

		if (AnimationState == ESpriterAnimationState::BLENDING)
		{
			if (CurrentAnimation && NextAnimation)
			{
				CurrentTimeline = GetTimeline(*CurrentAnimation, Name);
				NextTimeline = GetTimeline(*NextAnimation, Name);
				if (CurrentTimeline && NextTimeline)
				{
					for (FSpriterFatTimelineKey& Key : CurrentTimeline->Keys)
					{
						if (Key.TimeInMS <= CurrentTimeMS)
						{
							C1 = &Key;
						}
					}

					C2 = &NextTimeline->Keys[0];

					Keys->Add(C1);
					Keys->Add(C2);

					return Keys;
				}
			}
		}
		else
		{
			if (CurrentAnimation)
			{
				CurrentTimeline = GetTimeline(*CurrentAnimation, Name);
				if (CurrentTimeline)
				{
					for (int Key = 0; Key < CurrentTimeline->Keys.Num(); ++Key)
					{
						if (CurrentTimeline->Keys[Key].TimeInMS <= CurrentTimeMS)
						{
							C1 = &CurrentTimeline->Keys[Key];
							C2 = &CurrentTimeline->Keys[(Key + 1) % CurrentTimeline->Keys.Num()];
						}
					}

					Keys->Add(C1);
					Keys->Add(C2);

					return Keys;
				}
			}
		}
	}

	return nullptr;
}

//Data Grabbers
FSpriterEntity* USpriterSkeletonComponent::GetEntity()
{
	if (Skeleton  && Skeleton->ImportedData.Entities.Num() > 0)
	{
		return &Skeleton->ImportedData.Entities[0];
	}

	UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_GetEntity() : Coulld'nt find Entity, Returned Null!"));
	return nullptr;
}

FSpriterAnimation* USpriterSkeletonComponent::GetAnimation(int32 AnimationIndex)
{
	if (Skeleton)
	{
		FSpriterEntity* Entity = GetEntity();
		if (Entity)
		{
			if (AnimationIndex >= 0 && AnimationIndex < Entity->Animations.Num())
			{
				return &Entity->Animations[AnimationIndex];
			}
		}
	}


	UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_GetAnimation( AnimationIndex ) : Tried to search %d Index, returned Null!"), AnimationIndex);
	return nullptr;
}

FSpriterAnimation* USpriterSkeletonComponent::GetAnimation(const FString& AnimationName)
{
	if (Skeleton && !AnimationName.IsEmpty())
	{
		FSpriterEntity* Entity = GetEntity();
		if (Entity)
		{
			for (FSpriterAnimation& Anim : Entity->Animations)
			{
				if (Anim.Name.Equals(AnimationName, ESearchCase::IgnoreCase))
				{
					return &Anim;
				}
			}
		}
	}

	return nullptr;
}

FSpriterTimeline* USpriterSkeletonComponent::GetTimeline(FSpriterAnimation& Animation, int32 TimelineIndex)
{
	if (Skeleton && !Animation.Name.IsEmpty())
	{
		if (TimelineIndex >= 0 && TimelineIndex < Animation.Timelines.Num())
		{
			return &Animation.Timelines[TimelineIndex];
		}
	}

	return nullptr;
}

FSpriterTimeline* USpriterSkeletonComponent::GetTimeline(FSpriterAnimation& Animation, const FString & Name)
{
	if (Skeleton && !Animation.Name.IsEmpty() && !Name.IsEmpty())
	{
		for (FSpriterTimeline& TimeL : Animation.Timelines)
		{
			if (TimeL.Name.Equals(Name, ESearchCase::IgnoreCase))
				{
					return &TimeL;
				}
		}
	}

	return nullptr;
}

FSpriterObjectRef* USpriterSkeletonComponent::GetObjectRef(FSpriterAnimation& Animation, FSpriterMainlineKey& Key, const FString& Name)
{
	if (Skeleton  && Key.TimeInMS != INDEX_NONE &&  !Name.IsEmpty())
	{
		for (FSpriterObjectRef& Ref : Key.ObjectRefs)
		{
			FSpriterTimeline* Timeline = GetTimeline(Animation, Ref.TimelineIndex);

			if (Timeline)
			{
				if (Timeline->Name.Equals(Name, ESearchCase::IgnoreCase))
				{
					return &Ref;
				}
			}
		}
	}

	return nullptr;
}

FSpriterObjectInfo* USpriterSkeletonComponent::GetObjectInfo(int32 ObjectIndex)
{
	if (Skeleton)
	{
		FSpriterEntity* Entity = GetEntity();
		if (Entity)
		{
			if (ObjectIndex >= 0 && ObjectIndex < Entity->Objects.Num())
			{
				return &Entity->Objects[ObjectIndex];
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_GetObjectInfo( Object Index ) : Tried to find Object Info out of range!"));

	return nullptr;
}

FSpriterObjectInfo* USpriterSkeletonComponent::GetObjectInfo(const FString & Name)
{
	if (Skeleton && !Name.IsEmpty())
	{
		FSpriterEntity* Entity = GetEntity();
		if (Entity)
		{
			for (FSpriterObjectInfo& Object : Entity->Objects)
			{
				if (Object.Name.Equals(Name, ESearchCase::IgnoreCase))
				{
					return &Object;
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_GetObjectInfo( Object IName ) : Couldnt find s%'s Object Info!"));

	return nullptr;
}

FSpriterFile* USpriterSkeletonComponent::GetFile(int32 Folder, int32 File)
{
	if (Skeleton)
	{
		if (Folder >= 0 && Folder < Skeleton->ImportedData.Folders.Num())
		{
			FSpriterFolder* ChosenFolder = &Skeleton->ImportedData.Folders[Folder];
			if (ChosenFolder && File >= 0 && File < ChosenFolder->Files.Num())
			{
				return &ChosenFolder->Files[File];
			}
		}
	}

	return nullptr;
}

// Instance Grabbers
FSpriterBoneInstance* USpriterSkeletonComponent::GetBone(const FString & BoneName)
{
	if (Skeleton && !BoneName.IsEmpty())
	{
		for (FSpriterBoneInstance& Instance : Bones)
		{
			if (Instance.Name.Equals(BoneName, ESearchCase::IgnoreCase))
			{
				return &Instance;
			}
		}
	}

	return nullptr;
}

FSpriterSpriteInstance* USpriterSkeletonComponent::GetSprite(const FString& SpriteName)
{
	if (Skeleton && !SpriteName.IsEmpty())
	{
		for (FSpriterSpriteInstance& Instance : Sprites)
		{
			if (Instance.Name.Equals(SpriteName, ESearchCase::IgnoreCase))
			{
				return &Instance;
			}
		}
	}

	return nullptr;
}


bool USpriterSkeletonComponent::IsInitialized(bool bShouldInit)
{
	if (Skeleton)
	{
		if (Sprites.Num() > 0 || Bones.Num() > 0)
		{
			return true;
		}
		
		if(bShouldInit)
		{
			InitSkeleton();
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_IsInitialized() : Skeleton: Yes, Objects: No!"));
			return false;
		}
	}

	return false;
}

