// Copyright 2017 Isaac Hsu. MIT License

#pragma once

#include "InputCommand.generated.h"


USTRUCT(BlueprintType)
struct FInputCommandEntry
{
	GENERATED_BODY()

	FInputCommandEntry()
		: MinDuration(0.f)
		, MaxDuration(0.f)
		, MinInterval(0.f)
		, MaxInterval(0.f)
		, bIgnoreOthers(false)
	{}

	/* Input events to match. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> EventsToMatch;

	/* Input events to ignore. Unused if bIgnoreOthers is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> EventsToIgnore;

	/* If true, ignore the presence of the other input events except EventsToMatch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIgnoreOthers;

	/* Minimal duration for input to be considered as valid. Should be used for input events of held type. Unused if zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, UIMin = 0))
	float MinDuration;

	/* Maximal duration for input to be considered as valid. Should be used for input events of held type. Unused if zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, UIMin = 0))
	float MaxDuration;

	/* Minimal time limit between input for this entry and the next one to be considered as valid. Unused if zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, UIMin = 0))
	float MinInterval;

	/* Maximal time limit between input for this entry and the next one to be considered as valid. Unused if zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, UIMin = 0))
	float MaxInterval;

	FORCEINLINE bool CheckDuration(float Duration) const
	{
		if (MaxDuration != 0.f && Duration > MaxDuration)
		{
			return false;
		}
		if (MinDuration != 0.f && Duration < MinDuration)
		{
			return false;
		}

		return true;
	}

	FORCEINLINE bool CheckInterval(float Interval) const
	{
		if (MinInterval != 0.f && Interval < MinInterval)
		{
			return false;
		}
		if (MaxInterval != 0.f && Interval > MaxInterval)
		{
			return false;
		}

		return true;
	}
};

USTRUCT(BlueprintType)
struct FInputCommandSequence
{
	GENERATED_BODY()

	FInputCommandSequence() : bEnabled(true) {}

	/** Whether this sequence is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled;

	/* A list of input snapshots to match. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FInputCommandEntry> Entries;
};

/**
* Represents an input command commonly found in fighting games, such as Quarter-Circle-Forward Punch.
* Can be used with UInputBufferComponent to detect combinations of buttons.
**/
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Input))
class INPUTBUFFER_API UInputCommand : public UObject
{
	GENERATED_BODY()

public:

	/* Time limit of valid input. Unused if zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, Meta = (ClampMin = 0, UIMin = 0))
	float TimeLimit;

	/* Input events to ignore. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> EventsToIgnore;

	/* Each sequence contains a series of input snapshots to match. A command is considered matched if any of its sequences matches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FInputCommandSequence> Sequences;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Thumbnail")
	class UTexture2D* Thumbnail;
#endif

};