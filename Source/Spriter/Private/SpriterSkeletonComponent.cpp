// Fill out your copyright notice in the Description page of Project Settings.

#include "SpriterPrivatePCH.h"
#include "SpriterSkeletonComponent.h"

// Static's Initialization
const float USpriterSkeletonComponent::SPRITER_ZOFFSET = 2.0f;

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
	: IsActive(true)
	, Name("")
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
			if (ActiveAnimation && NextAnimation)
			{
				if (CurrentBlendTimeMS >= BlendDurationMS)
				{
					CurrentTimeMS = 0.f;
					CurrentBlendTimeMS = 0.f;
					BlendDurationMS = 0.f;
					ActiveAnimation = NextAnimation;
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
			if (ActiveAnimation)
			{
				if (CurrentTimeMS >= ActiveAnimation->LengthInMS)
				{
					UpdateBones();
					UpdateSprites();

					OnAnimationEnded.Broadcast(this, *ActiveAnimation, false);

					if (ActiveAnimation->bIsLooping)
					{
						CurrentTimeMS = 0;
					}
					else
					{
						AnimationState = ESpriterAnimationState::NONE;

						OnAnimationEnded.Broadcast(this, *ActiveAnimation, false);
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
						OnAnimationStarted.Broadcast(this, *ActiveAnimation, FirstTime);

						bAnimationWasStarted = false;
					}

					CurrentTimeMS = FMath::Min<int32>(ActiveAnimation->LengthInMS, (CurrentTimeMS + ToMS(DeltaTime)));
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

void USpriterSkeletonComponent::SetActiveEntity(int32 EntityIndex)
{
	if (Skeleton && Skeleton->ImportedData.Entities.IsValidIndex(EntityIndex))
	{
		FSpriterEntity* Entity = GetEntity(EntityIndex);
		if (Entity && ActiveEntity != Entity)
		{
			ActiveEntity = Entity;
			InitSkeleton();
		}
	}
}

void USpriterSkeletonComponent::SetActiveEntityByName(const FString & EntityName)
{
	if (Skeleton && !EntityName.IsEmpty())
	{
		FSpriterEntity* Entity = GetEntity(EntityName);
		if (Entity && ActiveEntity != Entity)
		{
			ActiveEntity = Entity;
			InitSkeleton();
		}
	}
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
			if (!ActiveAnimation)
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

				OnAnimationEnded.Broadcast(this, *ActiveAnimation, true);
			}
		}
		else
		{
			if (ActiveAnimation)
			{
				OnAnimationEnded.Broadcast(this, *ActiveAnimation, true);
			}

			ActiveAnimation = GetAnimation(AnimationName);
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
	if (IsInitialized(true) && ActiveAnimation)
	{	
		if (AnimationState == ESpriterAnimationState::NONE)
		{
			if (CurrentTimeMS <= ActiveAnimation->LengthInMS || ActiveAnimation->bIsLooping)
			{
				if (CurrentTimeMS >= ActiveAnimation->LengthInMS)
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
		if (!ActiveEntity)
		{
			ActiveEntity = GetEntity(0);
		}
		TArray<FString> SpritesToCreate = TArray<FString>();

		if (ActiveEntity)
		{
			// Loop through Object Infos, and create Bones
			for (FSpriterObjectInfo& Obj : ActiveEntity->Objects)
			{
				if (Obj.ObjectType == ESpriterObjectType::Bone)
				{
					FSpriterBoneInstance Bone = FSpriterBoneInstance();
					Bone.Name = Obj.Name;

					// Setup Bones Parent
					for (FSpriterAnimation& Animation : ActiveEntity->Animations)
					{
						FSpriterRef* BoneRef = GetBoneRef(Animation, Animation.MainlineKeys[0], Obj.Name);

						if (BoneRef)
						{
							if (BoneRef->ParentTimelineIndex != INDEX_NONE && BoneRef->TimelineIndex != INDEX_NONE)
							{
								FSpriterTimeline* Timeline = GetTimeline(Animation, BoneRef->TimelineIndex);
								FSpriterTimeline* ParentTimeline = GetTimeline(Animation, BoneRef->ParentTimelineIndex);
								if (Timeline && ParentTimeline)
								{
									if (Timeline->Name.Equals(Obj.Name, ESearchCase::IgnoreCase))
									{
										Bone.ParentBoneName = ParentTimeline->Name;
										break;
									}
								}
							}
						}
					}

					Bones.Add(Bone);
				}
			}

			// Loop Through all Timelines to find all Sprites that need to be Created
			for (FSpriterAnimation& Animation : ActiveEntity->Animations)
			{
				for (FSpriterTimeline& Timeline : Animation.Timelines)
				{
					if (Timeline.ObjectType == ESpriterObjectType::Sprite)
					{
						SpritesToCreate.AddUnique(Timeline.Name);
					}
				}
			}

			// Create all necessary Sprites
			for (FString& SpriteName : SpritesToCreate)
			{
				FSpriterSpriteInstance Sprite = FSpriterSpriteInstance();
				Sprite.Name = SpriteName;

				Sprite.SpriteComponent = NewObject<UPaperSpriteComponent>((UObject*)Owner, FName(*SpriteName));
				Sprite.SpriteComponent->AttachTo(this);
				Sprite.SpriteComponent->bWantsBeginPlay = true;
				Sprite.SpriteComponent->RegisterComponent();

				Sprites.Add(Sprite);
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
		ActiveAnimation = GetAnimation(0);

		UpdateBones();
		UpdateSprites();

		AnimationState = ESpriterAnimationState::NONE;
	}
}

void USpriterSkeletonComponent::UpdateBones()
{
	if (IsInitialized(true))
	{
		TArray<FSpriterMainlineKey*>& MainKeys = *GetMainlineKeys();
		float Alpha = 0.f;
		float C1 = 0.f;
		float C2 = 0.f;

		// Finding Alpha for Mainline Keys
		if (MainKeys.Num() >= 2)
		{
			if (AnimationState == ESpriterAnimationState::BLENDING)
			{
				C1 = CurrentBlendTimeMS;
				C2 = BlendDurationMS;
			}
			else if (AnimationState == ESpriterAnimationState::PLAYING)
			{
				if (MainKeys[1]->TimeInMS == 0 && ActiveAnimation)
				{
					C1 = (CurrentTimeMS - MainKeys[0]->TimeInMS);
					C2 = (ActiveAnimation->LengthInMS - MainKeys[0]->TimeInMS);
				}
				else
				{
					C1 = (CurrentTimeMS - MainKeys[0]->TimeInMS);
					C2 = (MainKeys[1]->TimeInMS - MainKeys[0]->TimeInMS);
				}
			}

			if (MainKeys[0]->TimeInMS == MainKeys[1]->TimeInMS && AnimationState == ESpriterAnimationState::PLAYING)
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
		}

		for (FSpriterBoneInstance& Bone : Bones)
		{
			// Check if Bone is Referenced in Mainline
			if (MainKeys.Num() >= 2)
			{
				FSpriterRef* Ref;
				if (AnimationState == ESpriterAnimationState::BLENDING)
				{
					if (Alpha == 1)
					{
						Ref = GetBoneRef(*NextAnimation, *MainKeys[1], Bone.Name);
					}
					else
					{
						Ref = GetBoneRef(*ActiveAnimation, *MainKeys[0], Bone.Name);
					}
				}
				else
				{
					if (Alpha == 1)
					{
						Ref = GetBoneRef(*ActiveAnimation, *MainKeys[1], Bone.Name);
					}
					else
					{
						Ref = GetBoneRef(*ActiveAnimation, *MainKeys[0], Bone.Name);
					}
				}

				if (Ref)
				{
					Bone.IsActive = true;
					
					if (Bone.IsActive)
					{
						/*FSpriterTimeline* ParentTimeline = nullptr;
						if (AnimationState == ESpriterAnimationState::BLENDING)
						{
							if (Alpha == 1)
							{
								ParentTimeline = GetTimeline(*NextAnimation, Ref->ParentTimelineIndex);
							}
						}
						else
						{
							ParentTimeline = GetTimeline(*ActiveAnimation, Ref->ParentTimelineIndex);
						}

						if (ParentTimeline)
						{
							Sprite.ParentBoneName = ParentTimeline->Name;
						}*/

					}
				}
				else
				{
					Bone.IsActive = false;
				}
			}

			// Update Bone if Referenced in Mainline
			if (Bone.IsActive)
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
						if (Keys[1]->TimeInMS == 0 && ActiveAnimation)
						{
							C1 = (CurrentTimeMS - Keys[0]->TimeInMS);
							C2 = (ActiveAnimation->LengthInMS - Keys[0]->TimeInMS);
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
}

void USpriterSkeletonComponent::UpdateSprites()
{
	if (IsInitialized(true))
	{
		TArray<FSpriterMainlineKey*>& MainKeys = *GetMainlineKeys();
		float Alpha = 0.f;
		float C1 = 0.f;
		float C2 = 0.f;

		// Finding Alpha for Mainline Keys
		if (MainKeys.Num() >= 2)
		{
			if (AnimationState == ESpriterAnimationState::BLENDING)
			{
				C1 = CurrentBlendTimeMS;
				C2 = BlendDurationMS;
			}
			else if (AnimationState == ESpriterAnimationState::PLAYING)
			{
				if (MainKeys[1]->TimeInMS == 0 && ActiveAnimation)
				{
					C1 = (CurrentTimeMS - MainKeys[0]->TimeInMS);
					C2 = (ActiveAnimation->LengthInMS - MainKeys[0]->TimeInMS);
				}
				else
				{
					C1 = (CurrentTimeMS - MainKeys[0]->TimeInMS);
					C2 = (MainKeys[1]->TimeInMS - MainKeys[0]->TimeInMS);
				}
			}

			if (MainKeys[0]->TimeInMS == MainKeys[1]->TimeInMS && AnimationState == ESpriterAnimationState::PLAYING)
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
		}

		for (FSpriterSpriteInstance& Sprite : Sprites)
		{
			// Check if Sprite is Referenced in Mainline
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
						Ref = GetObjectRef(*ActiveAnimation, *MainKeys[0], Sprite.Name);
					}
				}
				else
				{
					if (Alpha == 1)
					{
						Ref = GetObjectRef(*ActiveAnimation, *MainKeys[1], Sprite.Name);
					}
					else
					{
						Ref = GetObjectRef(*ActiveAnimation, *MainKeys[0], Sprite.Name);
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
							ParentTimeline = GetTimeline(*ActiveAnimation, Ref->ParentTimelineIndex);
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

			// Update Sprite if Referenced in Mainline
			if (Sprite.IsActive)
			{
				TArray<FSpriterFatTimelineKey*>& Keys = *GetTimelineKeys(Sprite.Name);
				if (Keys.Num() >= 2)
				{
					// Finding Alpha for Timeline Keys
					if (AnimationState == ESpriterAnimationState::BLENDING)
					{
						C1 = CurrentBlendTimeMS;
						C2 = BlendDurationMS;
					}
					else if (AnimationState == ESpriterAnimationState::PLAYING)
					{
						if (Keys[1]->TimeInMS == 0 && ActiveAnimation)
						{
							C1 = (CurrentTimeMS - Keys[0]->TimeInMS);
							C2 = (ActiveAnimation->LengthInMS - Keys[0]->TimeInMS);
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

					// Updating Sprite from Timeline Keys
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
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_UpdateSprites() : Couldnt Find 2 Timeline Keys!"));

					return;
				}
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
void USpriterSkeletonComponent::GetEntity(int32 EntityIndex, FSpriterEntity& Entity)
{
	if (Skeleton  && Skeleton->ImportedData.Entities.IsValidIndex(EntityIndex))
	{
		Entity = Skeleton->ImportedData.Entities[EntityIndex];
	}
}

void USpriterSkeletonComponent::GetEntityByName(const FString& EntityName, FSpriterEntity& Entity)
{
	if (Skeleton  && Skeleton->ImportedData.Entities.Num() > 0 && !EntityName.IsEmpty())
	{
		for (FSpriterEntity& Ent : Skeleton->ImportedData.Entities)
		{
			if (Ent.Name.Equals(EntityName, ESearchCase::IgnoreCase))
			{
				Entity = Ent;
			}
		}
	}
}

void USpriterSkeletonComponent::GetActiveEntity(FSpriterEntity& Entity)
{
	if (Skeleton  && ActiveEntity)
	{
		Entity = *ActiveEntity;
	}
}

void USpriterSkeletonComponent::GetAnimation(int32 AnimationIndex, FSpriterAnimation& Animation)
{
	if (Skeleton)
	{
		if (ActiveEntity)
		{
			if (ActiveEntity->Animations.IsValidIndex(AnimationIndex))
			{
				Animation = ActiveEntity->Animations[AnimationIndex];
			}
		}
	}
}

void USpriterSkeletonComponent::GetAnimationByName(const FString& AnimationName, FSpriterAnimation& Animation)
{
	if (Skeleton && !AnimationName.IsEmpty())
	{
		if (ActiveEntity)
		{
			for (FSpriterAnimation& Anim : ActiveEntity->Animations)
			{
				if (Anim.Name.Equals(AnimationName, ESearchCase::IgnoreCase))
				{
					Animation = Anim;
				}
			}
		}
	}
}

void USpriterSkeletonComponent::GetActiveAnimation(FSpriterAnimation& Animation)
{
	if (Skeleton && ActiveAnimation)
	{
		Animation = *ActiveAnimation;
	}
}

void USpriterSkeletonComponent::GetNextAnimation(FSpriterAnimation& Animation)
{
	if (Skeleton && NextAnimation)
	{
		Animation = *NextAnimation;
	}
}

void USpriterSkeletonComponent::GetTimeline(UPARAM(ref)FSpriterAnimation& Animation, int32 TimelineIndex, FSpriterTimeline& Timeline)
{
	if (Skeleton && !Animation.Name.IsEmpty())
	{
		if (Animation.Timelines.IsValidIndex(TimelineIndex))
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

void USpriterSkeletonComponent::GetBoneRef(UPARAM(ref)FSpriterAnimation& Animation, UPARAM(ref)FSpriterMainlineKey& Key, const FString& BoneName, FSpriterRef& BoneRef)
{
	if (Skeleton  && Key.TimeInMS != INDEX_NONE &&  !BoneName.IsEmpty())
	{
		for (FSpriterRef& Ref : Key.BoneRefs)
		{
			FSpriterTimeline* Timeline = GetTimeline(Animation, Ref.TimelineIndex);

			if (Timeline)
			{
				if (Timeline->Name.Equals(BoneName, ESearchCase::IgnoreCase))
				{
					BoneRef = Ref;
				}
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
		if (ActiveEntity)
		{
			if (ActiveEntity->Objects.IsValidIndex(ObjectIndex))
			{
				ObjectInfo = ActiveEntity->Objects[ObjectIndex];
			}
		}
	}
}

void USpriterSkeletonComponent::GetObjectInfoByName(const FString & Name, FSpriterObjectInfo& ObjectInfo)
{
	if (Skeleton && !Name.IsEmpty())
	{
		if (ActiveEntity)
		{
			for (FSpriterObjectInfo& Object : ActiveEntity->Objects)
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
		if (Skeleton->ImportedData.Folders.IsValidIndex(FolderIndex))
		{
			FSpriterFolder* ChosenFolder = &Skeleton->ImportedData.Folders[FolderIndex];
			if (ChosenFolder && ChosenFolder->Files.IsValidIndex(FileIndex))
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
			if (ActiveAnimation && NextAnimation)
			{
				for (FSpriterMainlineKey& Key : ActiveAnimation->MainlineKeys)
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
			if (ActiveAnimation)
			{
				for (int32 Key = 0; Key < ActiveAnimation->MainlineKeys.Num(); ++Key)
				{
					if (ActiveAnimation->MainlineKeys[Key].TimeInMS <= CurrentTimeMS)
					{
						C1 = &ActiveAnimation->MainlineKeys[Key];
						C2 = &ActiveAnimation->MainlineKeys[(Key + 1) % ActiveAnimation->MainlineKeys.Num()];
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

TArray<FSpriterFatTimelineKey*>* USpriterSkeletonComponent::GetTimelineKeys(const FString& TimelineName)
{
	if (Skeleton && !TimelineName.IsEmpty())
	{
		FSpriterTimeline* CurrentTimeline;
		FSpriterTimeline* NextTimeline;
		TArray<FSpriterFatTimelineKey*>* Keys = new TArray<FSpriterFatTimelineKey*>();
		FSpriterFatTimelineKey* C1;
		FSpriterFatTimelineKey* C2;

		if (AnimationState == ESpriterAnimationState::BLENDING)
		{
			if (ActiveAnimation && NextAnimation)
			{
				CurrentTimeline = GetTimeline(*ActiveAnimation, TimelineName);
				NextTimeline = GetTimeline(*NextAnimation, TimelineName);
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
			if (ActiveAnimation)
			{
				CurrentTimeline = GetTimeline(*ActiveAnimation, TimelineName);
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

// C++ Data Grabbers
FSpriterEntity* USpriterSkeletonComponent::GetEntity(int32 EntityIndex)
{
	if (Skeleton  && Skeleton->ImportedData.Entities.Num() > 0 && Skeleton->ImportedData.Entities.IsValidIndex(EntityIndex))
	{
		return &Skeleton->ImportedData.Entities[EntityIndex];
	}

	UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_GetEntity(EntityIndex) : Coulld'nt find Entity, Returned Null!"));
	return nullptr;
}

FSpriterEntity* USpriterSkeletonComponent::GetEntity(const FString& EntityName)
{
	if (Skeleton  && Skeleton->ImportedData.Entities.Num() > 0 && !EntityName.IsEmpty())
	{
		for (FSpriterEntity& Entity : Skeleton->ImportedData.Entities)
		{
			if (Entity.Name.Equals(EntityName, ESearchCase::IgnoreCase))
			{
				return &Entity;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_GetEntity(EntityName) : Coulld'nt find Entity, Returned Null!"));
	return nullptr;
}

FSpriterAnimation* USpriterSkeletonComponent::GetAnimation(int32 AnimationIndex)
{
	if (Skeleton)
	{
		if (ActiveEntity)
		{
			if (ActiveEntity->Animations.IsValidIndex(AnimationIndex))
			{
				return &ActiveEntity->Animations[AnimationIndex];
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
		if (ActiveEntity)
		{
			for (FSpriterAnimation& Anim : ActiveEntity->Animations)
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

FSpriterRef* USpriterSkeletonComponent::GetBoneRef(FSpriterAnimation& Animation, FSpriterMainlineKey& Key, const FString& BoneName)
{
	if (Skeleton  && Key.TimeInMS != INDEX_NONE &&  !BoneName.IsEmpty())
	{
		for (FSpriterRef& Ref : Key.BoneRefs)
		{
			FSpriterTimeline* Timeline = GetTimeline(Animation, Ref.TimelineIndex);

			if (Timeline)
			{
				if (Timeline->Name.Equals(BoneName, ESearchCase::IgnoreCase))
				{
					return &Ref;
				}
			}
		}
	}

	return nullptr;
}

FSpriterObjectRef* USpriterSkeletonComponent::GetObjectRef(FSpriterAnimation& Animation, FSpriterMainlineKey& Key, const FString& ObjectName)
{
	if (Skeleton  && Key.TimeInMS != INDEX_NONE &&  !ObjectName.IsEmpty())
	{
		for (FSpriterObjectRef& Ref : Key.ObjectRefs)
		{
			FSpriterTimeline* Timeline = GetTimeline(Animation, Ref.TimelineIndex);

			if (Timeline)
			{
				if (Timeline->Name.Equals(ObjectName, ESearchCase::IgnoreCase))
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
		if (ActiveEntity)
		{
			if (ActiveEntity->Objects.IsValidIndex(ObjectIndex))
			{
				return &ActiveEntity->Objects[ObjectIndex];
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
		if (ActiveEntity)
		{
			for (FSpriterObjectInfo& Object : ActiveEntity->Objects)
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

// C++ Instance Grabbers
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
			return false;
		}
	}

	return false;
}

