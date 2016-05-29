// Fill out your copyright notice in the Description page of Project Settings.

#include "SpriterPrivatePCH.h"
#include "SpriterSkeletonComponent.h"


// Static's Initialization
const float USpriterSkeletonComponent::SPRITER_ZOFFSET = 2.0f;


// Class's Initialization

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

FSpriterBoxInstance::FSpriterBoxInstance()
	: IsActive(true)
	, Name("")
	, ParentBoneName("")
	, RelativeTransform()
	, WorldTransform()
	, ZIndex(0)
{
}

FSpriterPointInstance::FSpriterPointInstance()
	: IsActive(true)
	, Name("")
	, ParentBoneName("")
	, RelativeTransform()
	, WorldTransform()
	, ZIndex(0)
{
}

FSpriterEventInstance::FSpriterEventInstance()
	: Name("")
	, PreviousCallTimeMS(INDEX_NONE)
{
}


// Component Overrides

void USpriterSkeletonComponent::BeginPlay()
{
	Super::BeginPlay();

	// Init our default Skeleton
	IsInitialized(true);
}

void USpriterSkeletonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsInitialized(true) && AnimationState != ESpriterAnimationState::NONE)
	{
		// Update all Objects

		UpdateBones();
		UpdateSprites();
		if (AnimationState == ESpriterAnimationState::PLAYING)
		{
			UpdateBoxs();
			UpdatePoints();
			UpdateEvents();
		}


		// Update Animation State

		if (AnimationState == ESpriterAnimationState::BLENDING)
		{
			CurrentBlendTimeMS = FMath::Min<int32>(BlendDurationMS, (CurrentBlendTimeMS + ToMS(DeltaTime)));

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

					bFirstTime = true;

					return;
				}
			}
		}
		else if (AnimationState == ESpriterAnimationState::PLAYING)
		{
			if (ActiveAnimation)
			{
				if (CurrentTimeMS == 0)
				{
					OnAnimationStarted.Broadcast(this, *ActiveAnimation, bFirstTime);

					bFirstTime = false;
				}
			}

			CurrentTimeMS = FMath::Min<int32>(ActiveAnimation->LengthInMS, (CurrentTimeMS + ToMS(DeltaTime)));

			if (ActiveAnimation)
			{
				if (CurrentTimeMS >= ActiveAnimation->LengthInMS)
				{
					if (ActiveAnimation->bIsLooping)
					{
						CurrentTimeMS = 0;

						OnAnimationEnded.Broadcast(this, *ActiveAnimation, false);
						CleanupObjectData();
					}
					else
					{
						AnimationState = ESpriterAnimationState::NONE;

						OnAnimationEnded.Broadcast(this, *ActiveAnimation, false);
						CleanupObjectData();
					}
				}
			}
		}
	}

}


// Init Methods

bool USpriterSkeletonComponent::IsInitialized(bool bShouldInit)
{
	if (Skeleton)
	{
		if (Sprites.Num() > 0 || Bones.Num() > 0 || Boxs.Num() > 0 || Points.Num() > 0 || Events.Num() > 0)
		{
			return true;
		}

		if (bShouldInit)
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

void USpriterSkeletonComponent::InitSkeleton()
{
	if (Skeleton)
	{
		if (!ActiveEntity)
		{
			ActiveEntity = GetEntity(0);
		}

		// We do this because Spriter doesnt export Sprites or Points to the Object Info array, so we have to search for them in all Timelines
		TArray<FString> SpritesToCreate = TArray<FString>();
		TArray<FString> PointsToCreate = TArray<FString>();

		if (ActiveEntity)
		{
			// Loop through Object Infos, and create Bones, Boxs, and Events
			for (FSpriterObjectInfo& Obj : ActiveEntity->Objects)
			{
				if (Obj.ObjectType == ESpriterObjectType::Bone)
				{
					FSpriterBoneInstance Bone = FSpriterBoneInstance();
					Bone.Name = Obj.Name;

					// Setup Bones Static Parent (The plugin doesnt currently support dynamiclly reparenting bones)
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
				else if (Obj.ObjectType == ESpriterObjectType::Box)
				{
					FSpriterBoxInstance Box = FSpriterBoxInstance();
					Box.Name = Obj.Name;

					Box.Pivot = (Obj.PivotX * PaperAxisX) + (Obj.PivotY * PaperAxisY);
					Box.Scale = (Obj.Width * PaperAxisX) + (Obj.Height * PaperAxisY);

					Boxs.Add(Box);
				}
				else if (Obj.ObjectType == ESpriterObjectType::Event)
				{
					FSpriterEventInstance Event = FSpriterEventInstance();
					Event.Name = Obj.Name;

					Events.Add(Event);
				}
			}

			// Loop Through all Timelines to find all Sprites and Points that need to be Created
			for (FSpriterAnimation& Animation : ActiveEntity->Animations)
			{
				for (FSpriterTimeline& Timeline : Animation.Timelines)
				{
					if (Timeline.ObjectType == ESpriterObjectType::Sprite)
					{
						SpritesToCreate.AddUnique(Timeline.Name);
					}
					else if (Timeline.ObjectType == ESpriterObjectType::Point)
					{
						PointsToCreate.AddUnique(Timeline.Name);
					}
				}
			}

			// Create all needed Sprites 
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

			// Create all needed Points
			for (FString& PointName : PointsToCreate)
			{
				FSpriterPointInstance Point = FSpriterPointInstance();
				Point.Name = PointName;

				Points.Add(Point);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_InitSkeleton() : Tried to initialize without a Skeleton!"));
	}
}


// Gameplay Methods

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
				CleanupObjectData();
			}
		}
		else
		{
			if (ActiveAnimation)
			{
				OnAnimationEnded.Broadcast(this, *ActiveAnimation, true);
				CleanupObjectData();
			}

			ActiveAnimation = GetAnimation(AnimationName);
			NextAnimation = nullptr;
			CurrentTimeMS = 0.f;
			CurrentBlendTimeMS = 0.f;
			BlendDurationMS = 0.f;
			AnimationState = ESpriterAnimationState::PLAYING;

			bFirstTime = true;
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
		UpdateBoxs();
		UpdatePoints();
		UpdateEvents();

		AnimationState = ESpriterAnimationState::NONE;
	}
}

void USpriterSkeletonComponent::UpdateBones()
{
	if (IsInitialized(true))
	{
		const TArray<FSpriterMainlineKey*>& MainKeys = GetMainlineKeys();
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
				const TArray<FSpriterFatTimelineKey*>& Keys = GetTimelineKeys(Bone.Name);
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
		const TArray<FSpriterMainlineKey*>& MainKeys = GetMainlineKeys();
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
				const TArray<FSpriterFatTimelineKey*>& Keys = GetTimelineKeys(Sprite.Name);
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

					UPaperSprite* PaperSprite = GetSpriteFromCharacterMap(*GetFile(Keys[0]->FolderIndex, Keys[0]->FileIndex));
					if (Sprite.SpriteComponent->GetSprite() != PaperSprite)
					{
						Sprite.SpriteComponent->SetSprite(PaperSprite);
					}

					FTransform NewTransform = Sprite.WorldTransform;
					FLinearColor NewColor = FMath::Lerp<FLinearColor>(Keys[0]->Info.Color, Keys[1]->Info.Color, Alpha);
					//NewTransform.AddToTranslation(PaperAxisZ * -(Sprite.ZIndex * SPRITER_ZOFFSET));
					UPaperSprite* SpriteFile = Sprite.SpriteComponent->GetSprite();
					if (!Keys[0]->bUseDefaultPivot && SpriteFile)
					{
						const float PivotInPixelsX = SpriteFile->GetSourceSize().X * Keys[0]->PivotX;
						const float PivotInPixelsY = SpriteFile->GetSourceSize().Y * (1.0f - Keys[0]->PivotY);

						SpriteFile->SetPivotMode(ESpritePivotMode::Custom, FVector2D(PivotInPixelsX, PivotInPixelsY));
					}
					Sprite.SpriteComponent->SetRelativeTransform(NewTransform);
					Sprite.SpriteComponent->SetTranslucentSortPriority(Sprite.ZIndex);
					Sprite.SpriteComponent->SetSpriteColor(NewColor);
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

void USpriterSkeletonComponent::UpdateBoxs()
{
	if (IsInitialized(true))
	{
		const TArray<FSpriterMainlineKey*>& MainKeys = GetMainlineKeys();
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

		for (FSpriterBoxInstance& Box : Boxs)
		{
			// Check if Box is Referenced in Mainline
			if (MainKeys.Num() >= 2)
			{
				FSpriterObjectRef* Ref;
				if (AnimationState == ESpriterAnimationState::BLENDING)
				{
					if (Alpha == 1)
					{
						Ref = GetObjectRef(*NextAnimation, *MainKeys[1], Box.Name);
					}
					else
					{
						Ref = GetObjectRef(*ActiveAnimation, *MainKeys[0], Box.Name);
					}
				}
				else
				{
					if (Alpha == 1)
					{
						Ref = GetObjectRef(*ActiveAnimation, *MainKeys[1], Box.Name);
					}
					else
					{
						Ref = GetObjectRef(*ActiveAnimation, *MainKeys[0], Box.Name);
					}
				}

				if (Ref)
				{
					Box.IsActive = true;

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
							Box.ParentBoneName = ParentTimeline->Name;
						}

					Box.ZIndex = Ref->ZIndex;
				}
				else
				{
					Box.IsActive = false;
				}
			}

			// Update Box if Referenced in Mainline
			if (Box.IsActive)
			{
				const TArray<FSpriterFatTimelineKey*>& Keys = GetTimelineKeys(Box.Name);
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

					// Updating Box from Timeline Keys
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
					Box.Pivot = (Keys[0]->PivotX * PaperAxisX) + (Keys[0]->PivotY * PaperAxisY);
					FVector FirstPivotOffset = Keys[0]->Info.ConvertToTransform().GetRotation().RotateVector(Box.Pivot);
					FVector SecondPivotOffset = Keys[1]->Info.ConvertToTransform().GetRotation().RotateVector(Box.Pivot);

					RelativeTransform.SetLocation(FMath::Lerp(FirstTransform.GetLocation() + FirstPivotOffset, SecondTransform.GetLocation() + SecondPivotOffset, Alpha));
					RelativeTransform.SetRotation(FQuat::Slerp_NotNormalized(FirstTransform.GetRotation(), (SecondTransform.Rotator() + FRotator(SpinCorrection, 0, 0)).Quaternion(), Alpha));
					RelativeTransform.SetScale3D(FMath::Lerp(Box.Scale * FirstTransform.GetScale3D(), Box.Scale * SecondTransform.GetScale3D(), Alpha));

					Box.RelativeTransform = RelativeTransform;
					Box.RelativeTransform.SetLocation(Box.RelativeTransform.GetLocation() / Skeleton->PixelsPerUnrealUnit);
					Box.RelativeTransform.SetScale3D(Box.RelativeTransform.GetScale3D() / Skeleton->PixelsPerUnrealUnit);
					if (!Box.ParentBoneName.IsEmpty())
					{
						FSpriterBoneInstance* Parent = GetBone(Box.ParentBoneName);
						if (Parent)
						{
							FTransform::Multiply(&Box.WorldTransform, &Box.RelativeTransform, &Parent->WorldTransform);
						}
					}
					else
					{
						Box.WorldTransform = RelativeTransform;
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

void USpriterSkeletonComponent::UpdatePoints()
{
	if (IsInitialized(true))
	{
		const TArray<FSpriterMainlineKey*>& MainKeys = GetMainlineKeys();
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

		for (FSpriterPointInstance& Point : Points)
		{
			// Check if Point is Referenced in Mainline
			if (MainKeys.Num() >= 2)
			{
				FSpriterObjectRef* Ref;
				if (AnimationState == ESpriterAnimationState::BLENDING)
				{
					if (Alpha == 1)
					{
						Ref = GetObjectRef(*NextAnimation, *MainKeys[1], Point.Name);
					}
					else
					{
						Ref = GetObjectRef(*ActiveAnimation, *MainKeys[0], Point.Name);
					}
				}
				else
				{
					if (Alpha == 1)
					{
						Ref = GetObjectRef(*ActiveAnimation, *MainKeys[1], Point.Name);
					}
					else
					{
						Ref = GetObjectRef(*ActiveAnimation, *MainKeys[0], Point.Name);
					}
				}

				if (Ref)
				{
					Point.IsActive = true;

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
						Point.ParentBoneName = ParentTimeline->Name;
					}

					Point.ZIndex = Ref->ZIndex;
				}
				else
				{
					Point.IsActive = false;
				}
			}

			// Update Point if Referenced in Mainline
			if (Point.IsActive)
			{
				const TArray<FSpriterFatTimelineKey*>& Keys = GetTimelineKeys(Point.Name);
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

					// Updating Point from Timeline Keys
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

					Point.RelativeTransform = RelativeTransform;
					Point.RelativeTransform.SetLocation(Point.RelativeTransform.GetLocation() / Skeleton->PixelsPerUnrealUnit);
					if (!Point.ParentBoneName.IsEmpty())
					{
						FSpriterBoneInstance* Parent = GetBone(Point.ParentBoneName);
						if (Parent)
						{
							FTransform::Multiply(&Point.WorldTransform, &Point.RelativeTransform, &Parent->WorldTransform);
						}
					}
					else
					{
						Point.WorldTransform = RelativeTransform;
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

void USpriterSkeletonComponent::UpdateEvents()
{
	if (IsInitialized(true))
	{

		// Update Events
		for (FSpriterEventInstance& Event : Events)
		{
			const TArray<FSpriterEventLineKey*>& Keys = GetEventLineKeys(Event.Name);

			if (Keys.Num() >= 2)
			{
				// Check for Event Call
				if (Event.PreviousCallTimeMS < Keys[0]->TimeInMS)
				{
					OnEvent.Broadcast(this, Event.Name);
					Event.PreviousCallTimeMS = Keys[0]->TimeInMS;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("USpriterSkeletonComponent_UpdateEvents() : Couldnt Find 2 Event Line Keys!"));

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
	Boxs.Empty();
	Points.Empty();
	Events.Empty();
}

void USpriterSkeletonComponent::CleanupObjectData()
{
	for (FSpriterEventInstance& Event : Events)
	{
		Event.PreviousCallTimeMS = INDEX_NONE;
	}
}


// Blueprint Data Grabbers

void USpriterSkeletonComponent::GetEntity(int32 EntityIndex, FSpriterEntity& Entity)
{
	FSpriterEntity* EntityP = GetEntity(EntityIndex);
	if (EntityP)
	{
		Entity = *EntityP;
	}
}

void USpriterSkeletonComponent::GetEntityByName(const FString& EntityName, FSpriterEntity& Entity)
{
	FSpriterEntity* EntityP = GetEntity(EntityName);
	if (EntityP)
	{
		Entity = *EntityP;
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
	FSpriterAnimation* AnimationP = GetAnimation(AnimationIndex);
	if (AnimationP)
	{
		Animation = *AnimationP;
	}
}

void USpriterSkeletonComponent::GetAnimationByName(const FString& AnimationName, FSpriterAnimation& Animation)
{
	FSpriterAnimation* AnimationP = GetAnimation(AnimationName);
	if (AnimationP)
	{
		Animation = *AnimationP;
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
	FSpriterTimeline* TimelineP = GetTimeline(Animation, TimelineIndex);
	if (TimelineP)
	{
		Timeline = *TimelineP;
	}
}

void USpriterSkeletonComponent::GetTimelineByName(UPARAM(ref)FSpriterAnimation& Animation, const FString& TimelineName, FSpriterTimeline& Timeline)
{
	FSpriterTimeline* TimelineP = GetTimeline(Animation, TimelineName);
	if (TimelineP)
	{
		Timeline = *TimelineP;
	}
}

void USpriterSkeletonComponent::GetBoneRef(UPARAM(ref)FSpriterAnimation& Animation, UPARAM(ref)FSpriterMainlineKey& Key, const FString& BoneName, FSpriterRef& BoneRef)
{
	FSpriterRef* RefP = GetBoneRef(Animation, Key, BoneName);
	if (RefP)
	{
		BoneRef = *RefP;
	}
}

void USpriterSkeletonComponent::GetObjectRef(UPARAM(ref)FSpriterAnimation& Animation, UPARAM(ref)FSpriterMainlineKey& Key, const FString& ObjectName, FSpriterObjectRef& ObjectRef)
{
	FSpriterObjectRef* RefP = GetObjectRef(Animation, Key, ObjectName);
	if (RefP)
	{
		ObjectRef = *RefP;
	}
}

void USpriterSkeletonComponent::GetObjectInfo(int32 ObjectIndex, FSpriterObjectInfo& ObjectInfo)
{
	FSpriterObjectInfo* InfoP = GetObjectInfo(ObjectIndex);
	if (InfoP)
	{
		ObjectInfo = *InfoP;
	}
}

void USpriterSkeletonComponent::GetObjectInfoByName(const FString& ObjectName, FSpriterObjectInfo& ObjectInfo)
{
	FSpriterObjectInfo* InfoP = GetObjectInfo(ObjectName);
	if (InfoP)
	{
		ObjectInfo = *InfoP;
	}
}

void USpriterSkeletonComponent::GetFile(int32 FolderIndex, int32 FileIndex, FSpriterFile& File)
{
	FSpriterFile* FileP = GetFile(FolderIndex, FileIndex);
	if (FileP)
	{
		File = *FileP;
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


// Blueprint Instance Grabbers

void USpriterSkeletonComponent::GetBone(int32 BoneIndex, FSpriterBoneInstance& Bone)
{
	FSpriterBoneInstance* BoneP = GetBone(BoneIndex);
	if (BoneP)
	{
		Bone = *BoneP;
	}
}

void USpriterSkeletonComponent::GetBoneByName(const FString& BoneName, FSpriterBoneInstance& Bone)
{
	FSpriterBoneInstance* BoneP = GetBone(BoneName);
	if (BoneP)
	{
		Bone = *BoneP;
	}
}

void USpriterSkeletonComponent::GetSprite(int32 SpriteIndex, FSpriterSpriteInstance& Sprite)
{
	FSpriterSpriteInstance* SpriteP = GetSprite(SpriteIndex);
	if (SpriteP)
	{
		Sprite = *SpriteP;
	}
}

void USpriterSkeletonComponent::GetSpriteByName(const FString& SpriteName, FSpriterSpriteInstance& Sprite)
{
	FSpriterSpriteInstance* SpriteP = GetSprite(SpriteName);
	if (SpriteP)
	{
		Sprite = *SpriteP;
	}
}

void USpriterSkeletonComponent::GetBox(int32 BoxIndex, FSpriterBoxInstance& Box)
{
	FSpriterBoxInstance* BoxP = GetBox(BoxIndex);
	if (BoxP)
	{
		Box = *BoxP;
	}
}

void USpriterSkeletonComponent::GetBoxByName(const FString& BoxName, FSpriterBoxInstance& Box)
{
	FSpriterBoxInstance* BoxP = GetBox(BoxName);
	if (BoxP)
	{
		Box = *BoxP;
	}
}

void USpriterSkeletonComponent::GetPoint(int32 PointIndex, FSpriterPointInstance& Point)
{
	FSpriterPointInstance* PointP = GetPoint(PointIndex);
	if (PointP)
	{
		Point = *PointP;
	}
}

void USpriterSkeletonComponent::GetPointByName(const FString& PointName, FSpriterPointInstance& Point)
{
	FSpriterPointInstance* PointP = GetPoint(PointName);
	if (PointP)
	{
		Point = *PointP;
	}
}

void USpriterSkeletonComponent::GetEvent(int32 EventIndex, FSpriterEventInstance& Event)
{
	FSpriterEventInstance* EventP = GetEvent(EventIndex);
	if (EventP)
	{
		Event = *EventP;
	}
}

void USpriterSkeletonComponent::GetEventByName(const FString& EventName, FSpriterEventInstance& Event)
{
	FSpriterEventInstance* EventP = GetEvent(EventName);
	if (EventP)
	{
		Event = *EventP;
	}
}


// Animation Dependant Grabbers

TArray<FSpriterMainlineKey*> USpriterSkeletonComponent::GetMainlineKeys()
{
	if (Skeleton)
	{
		TArray<FSpriterMainlineKey*> MainlineKeys  = TArray<FSpriterMainlineKey*>();
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

		MainlineKeys.Add(C1);
		MainlineKeys.Add(C2);

		return MainlineKeys;
	}

	return TArray<FSpriterMainlineKey*>();
}

TArray<FSpriterFatTimelineKey*> USpriterSkeletonComponent::GetTimelineKeys(const FString& ObjectName)
{
	if (Skeleton && !ObjectName.IsEmpty())
	{
		FSpriterTimeline* CurrentTimeline;
		FSpriterTimeline* NextTimeline;
		TArray<FSpriterFatTimelineKey*> Keys = TArray<FSpriterFatTimelineKey*>();
		FSpriterFatTimelineKey* C1;
		FSpriterFatTimelineKey* C2;

		if (AnimationState == ESpriterAnimationState::BLENDING)
		{
			if (ActiveAnimation && NextAnimation)
			{
				CurrentTimeline = GetTimeline(*ActiveAnimation, ObjectName);
				NextTimeline = GetTimeline(*NextAnimation, ObjectName);
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

					Keys.Add(C1);
					Keys.Add(C2);

					return Keys;
				}
			}
		}
		else
		{
			if (ActiveAnimation)
			{
				CurrentTimeline = GetTimeline(*ActiveAnimation, ObjectName);
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

					Keys.Add(C1);
					Keys.Add(C2);

					return Keys;
				}
			}
		}
	}

	return TArray<FSpriterFatTimelineKey*>();
}

TArray<FSpriterEventLineKey*> USpriterSkeletonComponent::GetEventLineKeys(const FString& EventName)
{
	if (Skeleton && !EventName.IsEmpty())
	{
		FSpriterEventLine* CurrentTimeline;
		FSpriterEventLine* NextTimeline;
		TArray<FSpriterEventLineKey*> Keys = TArray<FSpriterEventLineKey*>();
		FSpriterEventLineKey* C1;
		FSpriterEventLineKey* C2;

		if (AnimationState == ESpriterAnimationState::BLENDING)
		{
			if (ActiveAnimation && NextAnimation)
			{
				CurrentTimeline = GetEventLine(*ActiveAnimation, EventName);
				NextTimeline = GetEventLine(*NextAnimation, EventName);
				if (CurrentTimeline && NextTimeline)
				{
					for (FSpriterEventLineKey& Key : CurrentTimeline->Keys)
					{
						if (Key.TimeInMS <= CurrentTimeMS)
						{
							C1 = &Key;
						}
					}

					C2 = &NextTimeline->Keys[0];

					Keys.Add(C1);
					Keys.Add(C2);

					return Keys;
				}
			}
		}
		else
		{
			if (ActiveAnimation)
			{
				CurrentTimeline = GetEventLine(*ActiveAnimation, EventName);
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

					Keys.Add(C1);
					Keys.Add(C2);

					return Keys;
				}
			}
		}
	}

	return TArray<FSpriterEventLineKey*>();
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

FSpriterTimeline* USpriterSkeletonComponent::GetTimeline(FSpriterAnimation& Animation, const FString & ObjectName)
{
	if (Skeleton && !Animation.Name.IsEmpty() && !ObjectName.IsEmpty())
	{
		for (FSpriterTimeline& TimeL : Animation.Timelines)
		{
			if (TimeL.Name.Equals(ObjectName, ESearchCase::IgnoreCase))
				{
					return &TimeL;
				}
		}
	}

	return nullptr;
}

FSpriterEventLine* USpriterSkeletonComponent::GetEventLine(FSpriterAnimation& Animation, int32 EventLineIndex)
{
	if (Skeleton && !Animation.Name.IsEmpty())
	{
		if (Animation.EventLines.IsValidIndex(EventLineIndex))
		{
			return &Animation.EventLines[EventLineIndex];
		}
	}

	return nullptr;
}

FSpriterEventLine* USpriterSkeletonComponent::GetEventLine(FSpriterAnimation& Animation, const FString & EventName)
{
	if (Skeleton && !Animation.Name.IsEmpty() && !EventName.IsEmpty())
	{
		for (FSpriterEventLine& EventL : Animation.EventLines)
		{
			if (EventL.Name.Equals(EventName, ESearchCase::IgnoreCase))
			{
				return &EventL;
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

FSpriterBoneInstance* USpriterSkeletonComponent::GetBone(int32 BoneIndex)
{
	if (Skeleton && Bones.IsValidIndex(BoneIndex))
	{
		return &Bones[BoneIndex];
	}

	return nullptr;
}

FSpriterBoneInstance* USpriterSkeletonComponent::GetBone(const FString& BoneName)
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

FSpriterSpriteInstance* USpriterSkeletonComponent::GetSprite(int32 SpriteIndex)
{
	if (Skeleton && Sprites.IsValidIndex(SpriteIndex))
	{
		return &Sprites[SpriteIndex];
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

FSpriterBoxInstance * USpriterSkeletonComponent::GetBox(int32 BoxIndex)
{
	if (Skeleton && Boxs.IsValidIndex(BoxIndex))
	{
		return &Boxs[BoxIndex];
	}

	return nullptr;
}

FSpriterBoxInstance * USpriterSkeletonComponent::GetBox(const FString & BoxName)
{
	if (Skeleton && !BoxName.IsEmpty())
	{
		for (FSpriterBoxInstance& Instance : Boxs)
		{
			if (Instance.Name.Equals(BoxName, ESearchCase::IgnoreCase))
			{
				return &Instance;
			}
		}
	}

	return nullptr;
}

FSpriterPointInstance * USpriterSkeletonComponent::GetPoint(int32 PointIndex)
{
	if (Skeleton && Points.IsValidIndex(PointIndex))
	{
		return &Points[PointIndex];
	}

	return nullptr;
}

FSpriterPointInstance * USpriterSkeletonComponent::GetPoint(const FString & PointName)
{
	if (Skeleton && !PointName.IsEmpty())
	{
		for (FSpriterPointInstance& Instance : Points)
		{
			if (Instance.Name.Equals(PointName, ESearchCase::IgnoreCase))
			{
				return &Instance;
			}
		}
	}

	return nullptr;
}

FSpriterEventInstance * USpriterSkeletonComponent::GetEvent(int32 EventIndex)
{
	if (Skeleton && Events.IsValidIndex(EventIndex))
	{
		return &Events[EventIndex];
	}

	return nullptr;
}

FSpriterEventInstance * USpriterSkeletonComponent::GetEvent(const FString & EventName)
{
	if (Skeleton && !EventName.IsEmpty())
	{
		for (FSpriterEventInstance& Instance : Events)
		{
			if (Instance.Name.Equals(EventName, ESearchCase::IgnoreCase))
			{
				return &Instance;
			}
		}
	}

	return nullptr;
}


//Utility Methods

int32 USpriterSkeletonComponent::ToMS(float Seconds)
{
	return FMath::FloorToInt(Seconds * 1000);
}

float USpriterSkeletonComponent::ToSeconds(int32 MS)
{
	return MS / 1000;
}