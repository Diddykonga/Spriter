#pragma once

#include "PaperSprite.h"
#include "SpriterCharacterMap.generated.h"

USTRUCT(BlueprintType)
struct SPRITER_API FSpriterCharacterMapEntry
{
	GENERATED_USTRUCT_BODY();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		FString AssociatedSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
		UPaperSprite* ResultSprite;

	FSpriterCharacterMapEntry();
};


UCLASS(BlueprintType)
class SPRITER_API USpriterCharacterMap : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spriter")
	TArray<FSpriterCharacterMapEntry> Entrys;
};