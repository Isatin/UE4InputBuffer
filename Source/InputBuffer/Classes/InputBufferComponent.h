// Copyright 2017 Isaac Hsu. MIT License

#pragma once

#include "Components/ActorComponent.h"
#include "BufferedInputEventKit.h"
#include "CyclicBuffer.h"
#include "InputHistoryRecordArray.h"
#include "InputBufferComponent.generated.h"


UENUM(BlueprintType)
enum class FBufferedInputEventType : uint8
{
	Pressed = 0,
	Released = 1,
	Held = 2,
};

USTRUCT()
struct FBufferedInputEventSetup
{
	GENERATED_BODY()

	FBufferedInputEventSetup()
		: bEnabled(true)
		, Name(NAME_None)
		, Type(FBufferedInputEventType::Pressed)
		, bExecuteWhenPaused(false)
		, KeyMappingName(NAME_None)
	{}

	/** Whether this event is enabled. */
	UPROPERTY(EditAnywhere)
	bool bEnabled;

	/** Name of input event, e.g "Jump" */
	UPROPERTY(EditAnywhere)
	FName Name;

	/** Type of the input event. */
	UPROPERTY(EditAnywhere)
	FBufferedInputEventType Type;

	/** Should the event get triggered even when the game is paused? */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bExecuteWhenPaused;

	/** Name of key mapping to use for this event. */
	UPROPERTY(EditAnywhere)
	FName KeyMappingName;

	/** Keys to bind it to. */
	UPROPERTY(EditAnywhere)
	TArray<FKey> Keys;
};

USTRUCT()
struct FBufferedInputEventKeyMapping
{
	GENERATED_BODY()

	FBufferedInputEventKeyMapping() : Name(NAME_None) {}

	/** Name of this mapping. */
	UPROPERTY(EditAnywhere)
	FName Name;

	/** Keys to bind it to. */
	UPROPERTY(EditAnywhere)
	TArray<FKey> Keys;
};
 
/* Record stored in input buffer representing the same input status over one or several frames. */
struct FInputBufferRecord
{
	FInputBufferRecord()
		: bValid(false)
		, StartTime(0.f)
		, EndTime(0.f)
		, Events(0)
		, TranslatedEvents(0)
	{}

	FInputBufferRecord(float InStarTime, float InEndTime, uint64 InEvents, uint64 InTranslatedEvents, bool bInValid = true)
		: bValid(bInValid)
		, StartTime(InStarTime)
		, EndTime(InEndTime)
		, Events(InEvents)
		, TranslatedEvents(InTranslatedEvents)
	{}

	/** Whether this record is valid. */
	bool bValid;

	/** Time when we start to record it. */
	float StartTime;

	/** Time when we stop recording it. */
	float EndTime;

	/** Bit flags of input events. */
	uint64 Events;

	/** Input events that are translated from. */
	uint64 TranslatedEvents;

	/** Input event capacity = the number of bits of event flags. */
	static const int32 MAX_EVENTS = sizeof(uint64) * 8;
};

/**
* A component used to store input data for input buffering.
*
* Caution: Should be used with AInputBufferPlayerController.
**/
UCLASS( BlueprintType, Blueprintable, ClassGroup=(Input) )
class INPUTBUFFER_API UInputBufferComponent : public UActorComponent, public FBufferedInputEventKit
{
	GENERATED_BODY()

public:		
	UInputBufferComponent();

public:

	/* Input events set-up information. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
	TArray<FBufferedInputEventSetup> EventSetups;

	/* A list of input events into which others could be translated. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
	TArray<FName> TranslatedEvents;

	/* Optional key mappings that can be referenced in input event set-up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Buffer")
	TArray<FBufferedInputEventKeyMapping> KeyMappings;

	/* The maximal capacity of the input buffer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Buffer", Meta = (ClampMin = 0, UIMin = 0))
	int32 MaxInputHistory;

public:

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	//~ End UActorComponent Interface

	/**
	* Resets internal data structures according to EventSetups. Should be called after changes to EventSetups are made.
	*
	* Caution: Calling this function will clear the input buffer.
	*
	* @return The number of registered input events.
	**/
	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	int32 Initialize();

	/* Called by the owner controller's PreProcessInput. */
	void OnPreProcessInput(class UPlayerInput* PlayerInput, const bool bGamePaused);

	/* Called by the owner controller's PostProcessInput. */
	void OnPostProcessInput(class UPlayerInput* PlayerInput, const bool bGamePaused);

	/* Clears the input buffer. */
	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	void ClearHistory();

	/* Invalidates the input buffer. The records in the buffer are still there but they will be no longer valid for command recognition. */
	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	void InvalidateHistory();

	bool ConvertEventsToFlags(const TArray<FName>& Events, uint64& Flags) const;
	void ConvertFlagsToEvents(uint64 Flags, TArray<FName>& Events) const;

	/**
	* Prints the content of the input buffer to a string.
	*
	* @param MaxRecords Maximal number of latest records to print.
	* @param bIncludeInvalidRecords Whether invalidated records are included.
	* @param bReverseChronological Whether records are printed in chronological order or reverse chronological order.
	* @return An output string.
	*/
	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	FString Print(int32 MaxRecords = 0, bool bIncludeInvalidRecords = false, bool bReverseChronological = false) const;

	/**
	* Retrieves the current input events.
	*
	* Caution: Output input events are only valid after player input is processed in this frame. 
	*
	* @param Events An output array of the input events in this frame.
	*/
	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	void GetCurrentEvents(TArray<FName>& Events) const;

	/**
	* Retrieves the last input events in the input buffer.
	*
	* @param Events An output array of the last input events.
	* @param TimeLimit If the time of the last events exceeds the time limit, nothing will be retrieved. Zero means no time limit.
	* @param bSkipEmptyTrail Whether a trailing empty record should be skipped.
	* @return The time when the last events were buffered.
	*/
	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	float GetLastEvents(TArray<FName>& Events, float TimeLimit = 0.f, bool bSkipEmptyTrail = true) const;

	/**
	* Retrieves input records in the input buffer in chronological order.
	*
	* @param Records An output array of input records.
	* @param TimeLimit A time limit used to exclude outdated input records. Zero means no time limit.
	* @param bIncludeInvalidRecords Whether invalidated records are included.
	*/
	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	void GetHistoryRecords(TArray<FInputHistoryRecord>& Records, float TimeLimit = 0, bool bIncludeInvalidRecords = false) const;

	/**
	* Sets input history to given records. 
	* The given records must be in chronological order, and their timespan cannot overlap.
	*
	* Caution: The given records MUST be valid. If incorrect data is inputted, the input buffer may malfunction until those records are flushed out. 
	*
	* @param Records An array of given input records.
	* @return Whether there is an error during the process.
	*/
	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	bool SetHistoryRecords(const TArray<FInputHistoryRecord>& Records);

	/* Returns whether the last input record matches given input events.
	*
	* @param EventsToMatch Input events to match.
	* @param EventsToIgnore Input events to ignore.
	* @param TimeLimit If the time of the last record exceeds the time limit, always return false. Zero means no time limit.
	* @param bSkipEmptyTrail Whether a trailing empty record should be skipped.
	* @return Whether the last input record matches given input events.
	*/
	UFUNCTION(BlueprintCallable, Category = "Input Buffer", Meta = (AutoCreateRefTerm = "EventsToIgnore"))
	bool MatchEvents(const TArray<FName>& EventsToMatch, const TArray<FName>& EventsToIgnore, float TimeLimit = 0.f, bool bSkipEmptyTrail = true) const;

	/* Returns whether the latest input history matches a given input command. */
	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	bool MatchCommand(class UInputCommand* Command) const;

protected:

	FInputBufferRecord CurrentRecord;

	TCyclicBuffer<FInputBufferRecord> InputHistory;

	TArray<FBufferedInputEventSetup> RuntimeEvents;

	TMap<FName, int32> EventIndexMap;

	TMap<FKey, int32> KeyIndexMap;

	TBitArray<> KeyStates1;
	TBitArray<> KeyStates2;

	TBitArray<>* PreviousKeyStates;
	TBitArray<>* CurrentKeyStates;

protected:

	/* Returns the current time used internally in the input buffer. Override this if you wish to use another time function other than GetWorld()->GetRealTimeSeconds(). */
	virtual float GetCurrentTime() const;

	/* Note returned record is valid only before new records are added to the input buffer. */
	const FInputBufferRecord* GetLastRecord(float TimeLimit, bool bSkipEmptyTrail) const;
	const FInputBufferRecord* GetLastRecord(float TimeLimit) const;
	const FInputBufferRecord* GetLastNonEmptyRecord(float TimeLimit) const;

	FString EventFlagsToString(uint64 Actions, const FString& Separator = ", ") const;

	void ProcessInput(UPlayerInput* PlayerInput, const bool bGamePaused);

	void RecordEvent(uint64 EventIndex, class AInputBufferPlayerController* Controller);

};

