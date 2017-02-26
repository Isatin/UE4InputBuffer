// Copyright 2017 Isaac Hsu. MIT License

#include "InputBufferPrivatePCH.h"
#include "InputBufferComponent.h"
#include "InputBufferPlayerController.h"
#include "InputCommand.h"

//////////////////////////////////////////////////////////////////////////
// UInputBufferComponent

// Sets default values for this component's properties
UInputBufferComponent::UInputBufferComponent()
{
	MaxInputHistory = 10;
}

void UInputBufferComponent::BeginPlay()
{
	Super::BeginPlay();

	Initialize();
}

int32 UInputBufferComponent::Initialize()
{
	RuntimeEvents.Reset(EventSetups.Num() + TranslatedEvents.Num());
	EventIndexMap.Empty(RuntimeEvents.Num());

	KeyStates1.Reset();
	KeyStates2.Reset();

	TMap<FName, int32> KeyMappingIndexMap;
	for (int Idx = 0; Idx < KeyMappings.Num(); Idx++)
	{
		auto& Mapping = KeyMappings[Idx];
		if (Mapping.Name != NAME_None && Mapping.Keys.Num() > 0 && KeyMappingIndexMap.Find(Mapping.Name) == nullptr)
		{
			KeyMappingIndexMap.Add(Mapping.Name, Idx);
		}
	}

	for (const auto& Setup : EventSetups)
	{
		if (Setup.bEnabled && Setup.Name != NAME_None && EventIndexMap.Find(Setup.Name) == nullptr)
		{
			if (RuntimeEvents.Num() >= FInputBufferRecord::MAX_EVENTS)
			{
				UE_LOG(InputBufferLog, Warning, TEXT("Cannot register input events more than %d."), FInputBufferRecord::MAX_EVENTS);
				break;
			}

			// The use of a set makes sure there will be no duplicate keys.
			TSet<FKey> KeySet;

			for (const FKey& Key : Setup.Keys)
			{
				if (Key.IsValid())
				{
					KeySet.Add(Key);
				}
			}

			if (Setup.KeyMappingName != NAME_None)
			{
				int32* Index = KeyMappingIndexMap.Find(Setup.KeyMappingName);
				if (Index)
				{
					for (const FKey& Key : KeyMappings[*Index].Keys)
					{
						if (Key.IsValid())
						{
							KeySet.Add(Key);
						}
					}
				}
			}

			if (KeySet.Num() > 0)
			{
				for (FKey& Key : KeySet)
				{
					if (KeyIndexMap.Find(Key) == nullptr)
					{
						int32 Index1 = KeyStates1.Add(false);
						int32 Index2 = KeyStates2.Add(false);
						check(Index1 == Index2);
						KeyIndexMap.Add(Key, Index1);
					}
				}

				int32 Index = RuntimeEvents.Add(Setup);
				EventIndexMap.Add(Setup.Name, Index);
				RuntimeEvents[Index].Keys = KeySet.Array();
			}
			else
			{
				UE_LOG(InputBufferLog, Warning, TEXT("Input event '%s' does not have any valid key mappings."), *Setup.Name.ToString());
			}
		}
	}

	for (FName Event : TranslatedEvents)
	{
		if (Event != NAME_None && EventIndexMap.Find(Event) == nullptr)
		{
			if (RuntimeEvents.Num() >= FInputBufferRecord::MAX_EVENTS)
			{
				UE_LOG(InputBufferLog, Warning, TEXT("Cannot register input events more than %d."), FInputBufferRecord::MAX_EVENTS);
				break;
			}

			FBufferedInputEventSetup Setup;
			Setup.Name = Event;
			int32 Index = RuntimeEvents.Add(Setup);
			EventIndexMap.Add(Setup.Name, Index);
		}
	}

	check(KeyStates1.Num() == KeyStates2.Num());

	PreviousKeyStates = &KeyStates1;
	CurrentKeyStates = &KeyStates2;

	InputHistory.Reset(MaxInputHistory);

	return EventIndexMap.Num();
}

void UInputBufferComponent::OnPreProcessInput(UPlayerInput* PlayerInput, const bool bGamePaused)
{
}

void UInputBufferComponent::OnPostProcessInput(UPlayerInput* PlayerInput, const bool bGamePaused)
{
	ProcessInput(PlayerInput, bGamePaused);
}

void UInputBufferComponent::ProcessInput(UPlayerInput* PlayerInput, const bool bGamePaused)
{
	// Reset the current record because we may add it to the input buffer later.
	CurrentRecord.bValid = true;
	CurrentRecord.StartTime = GetCurrentTime();
	CurrentRecord.EndTime = CurrentRecord.StartTime;
	CurrentRecord.Events = 0;
	CurrentRecord.TranslatedEvents = 0;

	auto Controller = Cast<AInputBufferPlayerController>(GetOwner());

	if (PlayerInput)
	{
		Swap(PreviousKeyStates, CurrentKeyStates);

		// Update key states so we can determine if a key is just pressed or released.
		for (const auto& Pair : KeyIndexMap)
		{
			FKeyState* State = PlayerInput->GetKeyState(Pair.Key);
			(*CurrentKeyStates)[Pair.Value] = State ? (State->bDown != 0) : false;
		}

		// Set event flags according key states
		for (int32 Idx = 0; Idx < RuntimeEvents.Num(); Idx++)
		{
			const auto& Event = RuntimeEvents[Idx];

			if (!bGamePaused || Event.bExecuteWhenPaused)
			{
				if (Event.Type == FBufferedInputEventType::Pressed)
				{
					for (const FKey& Key : Event.Keys)
					{
						int32* KeyIndex = KeyIndexMap.Find(Key);
						if (KeyIndex)
						{
							if (!(*PreviousKeyStates)[*KeyIndex] && (*CurrentKeyStates)[*KeyIndex])
							{
								RecordEvent(Idx, Controller);
								break;
							}
						}
					}
				}
				else if (Event.Type == FBufferedInputEventType::Released)
				{
					for (const FKey& Key : Event.Keys)
					{
						int32* KeyIndex = KeyIndexMap.Find(Key);
						if (KeyIndex)
						{
							if ((*PreviousKeyStates)[*KeyIndex] && !(*CurrentKeyStates)[*KeyIndex])
							{
								RecordEvent(Idx, Controller);
								break;
							}
						}
					}
				}
				else if (Event.Type == FBufferedInputEventType::Held)
				{
					for (const FKey& Key : Event.Keys)
					{
						int32* KeyIndex = KeyIndexMap.Find(Key);
						if (KeyIndex)
						{
							if ((*CurrentKeyStates)[*KeyIndex])
							{
								RecordEvent(Idx, Controller);
								break;
							}
						}
					}
				}
			}
		}
	}

	// Add the current record to the input buffer if the input events are different from the previous. Otherwise, just prolong the last record.
	auto LastRecord = InputHistory.LastOrNull();
	if (LastRecord && LastRecord->Events == CurrentRecord.Events && LastRecord->TranslatedEvents == CurrentRecord.TranslatedEvents)
	{
		LastRecord->EndTime = CurrentRecord.StartTime;
	}
	else
	{
		InputHistory.Add(CurrentRecord);
	}

	// Trigger PostBufferInput event when the current events are not empty.
	if (CurrentRecord.Events != 0 && Controller)
	{
		Controller->PostBufferInput();
	}
}

void UInputBufferComponent::RecordEvent(uint64 EventIndex, AInputBufferPlayerController* Controller)
{
	check(EventIndex < RuntimeEvents.Num());
	check(Controller == GetOwner());

	// Translate given input event and find out an event index for translated event
	if (Controller)
	{
		FName OriginalEvent = RuntimeEvents[EventIndex].Name;
		FName TranslatedEvent = Controller->TranslateInputEvent(OriginalEvent);
		if (OriginalEvent != TranslatedEvent)
		{
			// Set the original event bit
			CurrentRecord.TranslatedEvents |= (1LL << EventIndex);

			int32* FoundIndex = EventIndexMap.Find(TranslatedEvent);
			if (FoundIndex == nullptr)
			{
				if (TranslatedEvent == NAME_None)
				{
					return; // Skip event flag recording
				}
				else
				{
					UE_LOG(InputBufferLog, Warning, TEXT("Unknown input event '%s' translated from '%s'."), *TranslatedEvent.ToString(), *OriginalEvent.ToString());
				}
			}
			else
			{
				EventIndex = *FoundIndex;
			}
		}
	}

	// Set the event bit
	CurrentRecord.Events |= (1LL << EventIndex);
}

void UInputBufferComponent::ClearHistory()
{
	InputHistory.Reset(MaxInputHistory);
}

void UInputBufferComponent::InvalidateHistory()
{
	auto Record = InputHistory.GetData();
	auto RecordEnd = Record + InputHistory.Num();
	for (; Record != RecordEnd; Record++)
	{
		Record->bValid = false;
	}
}

bool UInputBufferComponent::ConvertEventsToFlags(const TArray<FName>& Events, uint64& Flags) const
{
	bool bSucceeded = true;
	for (int32 Idx = 0; Idx < Events.Num(); Idx++)
	{
		const int32* Index = EventIndexMap.Find(Events[Idx]);
		if (Index)
		{
			Flags |= (1LL << *Index); // Set the event bit
		}
		else
		{
			bSucceeded = false; // Any unknown event means a failure.
		}
	}

	return bSucceeded;
}

void UInputBufferComponent::ConvertFlagsToEvents(uint64 Flags, TArray<FName>& Events) const
{
	// TODO: Reduce complexity
	for (int Idx = 0; Idx < RuntimeEvents.Num() && Flags != 0; Idx++)
	{
		if (Flags & 0x1)
		{
			Events.Add(RuntimeEvents[Idx].Name);
		}

		Flags >>= 1;
	}
}

void UInputBufferComponent::GetCurrentEvents(TArray<FName>& Events) const
{
	ConvertFlagsToEvents(CurrentRecord.Events, Events);
}

const FInputBufferRecord* UInputBufferComponent::GetLastRecord(float TimeLimit, bool bSkipEmptyTrail) const
{
	if (bSkipEmptyTrail)
	{
		return GetLastNonEmptyRecord(TimeLimit);
	}
	else
	{
		return GetLastRecord(TimeLimit);
	}
}

const FInputBufferRecord* UInputBufferComponent::GetLastRecord(float TimeLimit) const
{
	float CurrTime = GetCurrentTime();

	for (auto It = InputHistory.CreateConstReverseIterator(); It; ++It)
	{
		const FInputBufferRecord& Record = *It;
		if (Record.bValid && (CurrTime - Record.EndTime <= TimeLimit || TimeLimit == 0.f))
		{
			return &Record;
		}
		else
		{
			break;
		}
	}

	return nullptr;
}

const FInputBufferRecord* UInputBufferComponent::GetLastNonEmptyRecord(float TimeLimit) const
{
	float CurrTime = GetCurrentTime();

	for (auto It = InputHistory.CreateConstReverseIterator(); It; ++It)
	{
		const FInputBufferRecord& Record = *It;
		if (Record.bValid && (CurrTime - Record.EndTime <= TimeLimit || TimeLimit == 0.f))
		{
			if (Record.Events != 0)
			{
				return &Record;
			}
		}
		else
		{
			break;
		}
	}

	return nullptr;
}

float UInputBufferComponent::GetLastEvents(TArray<FName>& Events, float TimeLimit, bool bSkipEmptyTrail) const
{
	const FInputBufferRecord* Record = GetLastRecord(TimeLimit, bSkipEmptyTrail);
	if (Record)
	{
		ConvertFlagsToEvents(Record->Events, Events);
		return Record->EndTime;
	}
	else
	{
		return 0.f;
	}
}

void UInputBufferComponent::GetHistoryRecords(TArray<FInputHistoryRecord>& Records, float TimeLimit, bool bIncludeInvalidRecords) const
{
	float CurrTime = GetCurrentTime();

	for (auto It = InputHistory.CreateConstReverseIterator(); It; ++It)
	{
		const auto& Record = *It;
		if ((Record.bValid || bIncludeInvalidRecords) && (CurrTime - Record.EndTime <= TimeLimit || TimeLimit == 0.f))
		{
			FInputHistoryRecord Copy(Record.StartTime, Record.EndTime, Record.bValid);
			ConvertFlagsToEvents(Record.Events, Copy.Events);
			ConvertFlagsToEvents(Record.TranslatedEvents, Copy.TranslatedEvents);

			Records.Add(Copy);
		}
		else
		{
			break;
		}
	}

	Algo::Reverse(Records);
}

bool UInputBufferComponent::SetHistoryRecords(const TArray<FInputHistoryRecord>& Records)
{
	bool AllSucceeded = true;
	InputHistory.Reset(Records.Num());

	for (const auto& Record : Records)
	{
		uint64 Flags = 0;
		uint64 TranslatedFlags = 0;
		AllSucceeded = AllSucceeded && ConvertEventsToFlags(Record.Events, Flags);
		AllSucceeded = AllSucceeded && ConvertEventsToFlags(Record.TranslatedEvents, TranslatedFlags);

		InputHistory.Add(FInputBufferRecord(Record.StartTime, Record.EndTime, Flags, TranslatedFlags, Record.bValid));
	}

	return AllSucceeded;
}

bool UInputBufferComponent::MatchEvents(const TArray<FName>& EventsToMatch, const TArray<FName>& EventsToIgnore, float TimeLimit, bool bSkipEmptyTrail) const
{
	const FInputBufferRecord* Record = GetLastRecord(TimeLimit, bSkipEmptyTrail);
	if (Record)
	{
		uint64 MatchingFlags = 0; // The bit flags of events to match.
		if (ConvertEventsToFlags(EventsToMatch, MatchingFlags))
		{
			uint64 IgnoringFlags = 0; // The bit flags of events to ignore.
			ConvertEventsToFlags(EventsToIgnore, IgnoringFlags);

			if (CompareEventFlags(Record->Events, MatchingFlags, IgnoringFlags))
			{
				return true;
			}
		}
	}

	return false;
}

bool UInputBufferComponent::MatchCommand(class UInputCommand* Command) const
{
	if (Command == nullptr || InputHistory.Num() == 0)
	{
		return false; // because of nothing to match
	}

	uint64 OuterIgnoreFlags = 0;
	ConvertEventsToFlags(Command->EventsToIgnore, OuterIgnoreFlags);

	const float CurrTime = GetCurrentTime();

	for (FInputCommandSequence& Sequence : Command->Sequences)
	{
		if (Sequence.bEnabled)
		{
			bool bRepeating = false; // Are we trying to repeat the current entry?
			bool bCanRecede = false; // Can we rollback to the previous entry?
			float CurrEntryStartTime = 0; // The start time of the oldest matching record for the current entry. Used to check durations of entries.
			float CurrEntryEndTime = 0; // The end time of the latest matching record for the current entry. Used to check durations of entries.
			float PrevEntryStartTime = 0; // The start time of the oldest matching record for the previous entry. Used to check durations and interval of entries.
			float PrevEntryEndTime = 0; // The end time of the latest matching record for the previous entry. Used to check durations of entries.
			int32 EntryIdx = Sequence.Entries.Num() - 1; // The index of the command entry to match in the current iteration.
			auto It = InputHistory.CreateConstReverseIterator(); // Input history iterator.

			while (EntryIdx >= 0)
			{
				const auto& Entry = Sequence.Entries[EntryIdx];
				//if (Entry.EventsToMatch.Num() == 0)
				//{
				//	break; // because of nothing to match
				//}

				const auto& Record = *It;
				if (!Record.bValid)
				{
					if (bRepeating && EntryIdx == 0)
					{
						if (!Entry.CheckDuration(CurrEntryEndTime - CurrEntryStartTime))
						{
							break;
						}
						// Even if we failed to repeat the first entry, command recognition still succeeds since we have found matching records for all entries.
						return true;
					}
					else
					{
						break; // Need not check the previous records since they should be invalid too.
					}
				}

				if (CurrTime - Record.EndTime > Command->TimeLimit && Command->TimeLimit != 0.f && !bRepeating)
				{
					break;
				}

				//TArray<FName> RecordedEvents;
				//ConvertFlagsToEvents(Record.Events, RecordedEvents);

				bool bMatched = false; // Whether the current record matches the current entry?
				bool bNextEntry = true; // Should we advance to the next entry in the next iteraion?
				bool bNextRecord = true; // Should we advance to the next record in the next iteraion?
				uint64 MatchingFlags = 0; // The bit flags of events to match.
				uint64 IgnoringFlags = 0; // The bit flags of events to ignore.
				if (ConvertEventsToFlags(Entry.EventsToMatch, MatchingFlags))
				{
					// For events to match, unknown events means mismatch. But for events to ignore, unknown events are omitted.
					ConvertEventsToFlags(Entry.EventsToIgnore, IgnoringFlags);

					if (Entry.bIgnoreOthers)
					{
						bMatched = HasEventFlags(Record.Events, MatchingFlags);
					}
					else
					{
						IgnoringFlags |= OuterIgnoreFlags;
						bMatched = CompareEventFlags(Record.Events, MatchingFlags, IgnoringFlags);
					}
				}
				else
				{
					break; // Fails since we cannot find flags for unknown events.
				}

				if (bMatched)
				{
					if (CurrEntryEndTime == 0)
					{
						// Check limits of the duration of the previous entry.
						if (PrevEntryEndTime != 0.f)
						{
							const auto& PrevEntry = Sequence.Entries[EntryIdx + 1];
							if (!PrevEntry.CheckDuration(PrevEntryEndTime - PrevEntryStartTime))
							{
								break;
							}
						}

						// Check limits of the internal between the current entry and previous entry.
						if (PrevEntryStartTime != 0.f && !Entry.CheckInterval(PrevEntryStartTime - Record.EndTime))
						{
							break;
						}

						CurrEntryEndTime = Record.EndTime;
					}
					CurrEntryStartTime = Record.StartTime;

					if (EntryIdx > 0)
					{
						bCanRecede = true;
						bRepeating = false;
					}
					else
					{
						bCanRecede = false;
						bRepeating = true;
						bNextEntry = false;
					}
				}
				else if (bRepeating && EntryIdx == 0)
				{
					if (!Entry.CheckDuration(CurrEntryEndTime - CurrEntryStartTime))
					{
						break;
					}
					// Even if we failed to repeat the first entry, command recognition still succeeds since we have found matching records for all entries.
					return true;
				}
				else if (Record.Events == 0)
				{
					// Skip the current record when no input.
					bCanRecede = false;
					bRepeating = false;
					bNextEntry = false;
				}
				else if (bCanRecede)
				{
					// When failing to match the current entry, we try the previous entry if possible.
					bCanRecede = false;
					bRepeating = true;
					bNextRecord = false;
					bNextEntry = false;
					EntryIdx++;
					check(EntryIdx < Sequence.Entries.Num());

					CurrEntryStartTime = PrevEntryStartTime;
					CurrEntryEndTime = PrevEntryEndTime;
					PrevEntryStartTime = 0.f;
					PrevEntryEndTime = 0.f;
				}
				else
				{
					break; // Fails due to mismatch.
				}

				if (bNextRecord)
				{
					++It;
					if (!It) // If there is no remaining history.
					{
						if (EntryIdx == 0 && bMatched)
						{
							if (!Entry.CheckDuration(CurrEntryEndTime - CurrEntryStartTime))
							{
								break;
							}

							return true; // since we have checked all the entries and didn't fail
						}
						else
						{
							break; // Fails because of mismatch or no remaining history to match the next entry.
						}
					}
				}

				if (bNextEntry)
				{
					if (CurrEntryEndTime != 0)
					{
						PrevEntryStartTime = CurrEntryStartTime;
						PrevEntryEndTime = CurrEntryEndTime;
						CurrEntryStartTime = 0.f;
						CurrEntryEndTime = 0.f;
					}

					EntryIdx--;
				}
			}

			if (EntryIdx == -1)
			{
				return true; // since we have checked all the entries and didn't fail
			}
		}
	}

	return false;
}

float UInputBufferComponent::GetCurrentTime() const
{
	UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : 0.f;
}

FString UInputBufferComponent::EventFlagsToString(uint64 Events, const FString& Separator) const
{
	FString Result;
	bool bFirst = true;
	for (int Idx = 0; Idx < RuntimeEvents.Num(); Idx++)
	{
		if (Events & 0x1)
		{
			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				Result += Separator;
			}

			Result += RuntimeEvents[Idx].Name.ToString();
		}

		Events >>= 1;
	}

	return Result;
}

FString UInputBufferComponent::Print(int32 MaxRecords, bool bIncludeInvalidRecords, bool bReverseChronological) const
{
	FString Result;

	if (MaxRecords == 0)
	{
		MaxRecords = InputHistory.Num();
	}
	else
	{
		MaxRecords = FMath::Clamp(MaxRecords, 0, InputHistory.Num());
	}

	if (bReverseChronological)
	{
		int32 Count = 0;
		for (auto It = InputHistory.CreateConstReverseIterator(); It && Count < MaxRecords; ++It, ++Count)
		{
			if (It->bValid)
			{
				Result += FString::Printf(TEXT("[%s] "), *EventFlagsToString(It->Events));
			}
			else if (bIncludeInvalidRecords)
			{
				Result += FString::Printf(TEXT("(%s) "), *EventFlagsToString(It->Events));
			}
		}
	}
	else
	{
		int32 StartIndex = InputHistory.Num() - MaxRecords;
		for (auto It = InputHistory.CreateConstIterator(StartIndex); It; ++It)
		{
			if (It->bValid)
			{
				Result += FString::Printf(TEXT("[%s] "), *EventFlagsToString(It->Events));
			}
			else if (bIncludeInvalidRecords)
			{
				Result += FString::Printf(TEXT("(%s) "), *EventFlagsToString(It->Events));
			}
		}
	}

	return Result;
}
