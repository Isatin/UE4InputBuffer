// Copyright 2017 Isaac Hsu. MIT License

#include "InputBufferEditor.h"
#include "AutomationTest.h"
#include "AutomationEditorCommon.h"
#include "InputBufferComponent.h"
#include "InputBufferPlayerController.h"
#include "InputCommand.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputBufferTest, "Plugins.InputBuffer.InputBuffer", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInputBufferTest::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	World->Tick(LEVELTICK_All, 1.f);

	auto PlayerController = World->SpawnActor<AInputBufferPlayerController>();
	auto InputBuffer = PlayerController->InputBuffer;

	// Test maximal setup of input events
	{
		const int32 TestSetupCount = FInputBufferRecord::MAX_EVENTS + 1;
		InputBuffer->EventSetups.Reset(TestSetupCount);
		for (int32 Idx = 0; Idx < TestSetupCount; Idx++)
		{
			int32 EventIndex = InputBuffer->EventSetups.AddDefaulted();
			auto* Setup = &InputBuffer->EventSetups[EventIndex];
			Setup->Name = *FString::FromInt(Idx);
			Setup->Type = FBufferedInputEventType::Pressed;
			Setup->Keys.Add(EKeys::LeftMouseButton);
		}

		TestTrue(FString::Printf(TEXT("The number of registered input events must be equal to or less than %d."), FInputBufferRecord::MAX_EVENTS), InputBuffer->Initialize() <= FInputBufferRecord::MAX_EVENTS);
	}

	// Test maximal setup of input events
	{
		InputBuffer->EventSetups.Reset(FInputBufferRecord::MAX_EVENTS);
		for (int32 Idx = 0; Idx < FInputBufferRecord::MAX_EVENTS; Idx++)
		{
			int32 EventIndex = InputBuffer->EventSetups.AddDefaulted();
			auto* Setup = &InputBuffer->EventSetups[EventIndex];
			Setup->Name = *FString::FromInt(Idx);
			Setup->Type = FBufferedInputEventType::Pressed;
			Setup->Keys.Add(EKeys::LeftMouseButton);
		}

		InputBuffer->TranslatedEvents.Reset();
		InputBuffer->TranslatedEvents.Add(TEXT("Forward"));
		InputBuffer->TranslatedEvents.Add(TEXT("Back"));

		TestTrue(FString::Printf(TEXT("The number of registered input events must be equal to or less than %d."), FInputBufferRecord::MAX_EVENTS), InputBuffer->Initialize() <= FInputBufferRecord::MAX_EVENTS);
	}

	// Set up input events
	{
		InputBuffer->EventSetups.Reset();

		int32 Index = InputBuffer->EventSetups.AddDefaulted();
		auto* Setup = &InputBuffer->EventSetups[Index];
		Setup->Name = TEXT("Punch");
		Setup->Type = FBufferedInputEventType::Pressed;
		Setup->Keys.Add(EKeys::LeftMouseButton);

		Index = InputBuffer->EventSetups.AddDefaulted();
		Setup = &InputBuffer->EventSetups[Index];
		Setup->Name = TEXT("Kick");
		Setup->Type = FBufferedInputEventType::Pressed;
		Setup->Keys.Add(EKeys::LeftMouseButton);

		Index = InputBuffer->EventSetups.AddDefaulted();
		Setup = &InputBuffer->EventSetups[Index];
		Setup->Name = TEXT("Up");
		Setup->Type = FBufferedInputEventType::Held;
		Setup->Keys.Add(EKeys::Up);

		Index = InputBuffer->EventSetups.AddDefaulted();
		Setup = &InputBuffer->EventSetups[Index];
		Setup->Name = TEXT("Down");
		Setup->Type = FBufferedInputEventType::Held;
		Setup->Keys.Add(EKeys::Down);

		Index = InputBuffer->EventSetups.AddDefaulted();
		Setup = &InputBuffer->EventSetups[Index];
		Setup->Name = TEXT("Left");
		Setup->Type = FBufferedInputEventType::Held;
		Setup->Keys.Add(EKeys::Left);

		Index = InputBuffer->EventSetups.AddDefaulted();
		Setup = &InputBuffer->EventSetups[Index];
		Setup->Name = TEXT("Right");
		Setup->Type = FBufferedInputEventType::Held;
		Setup->Keys.Add(EKeys::Right);

		InputBuffer->TranslatedEvents.Reset();
		InputBuffer->TranslatedEvents.Add(TEXT("Forward"));
		InputBuffer->TranslatedEvents.Add(TEXT("Back"));

		TestEqual(TEXT("The number of registered input events must be the same as input event set-up."), InputBuffer->Initialize(), InputBuffer->EventSetups.Num() + InputBuffer->TranslatedEvents.Num());
	}

	// Input history getter/setter
	{
		TArray<FInputHistoryRecord> InRecords;

		int32 Index = InRecords.AddDefaulted();
		InRecords[Index].Events.Add(TEXT("Punch"));
		InRecords[Index].Events.Add(TEXT("Forward"));
		InRecords[Index].TranslatedEvents.Add(TEXT("Right"));
		InRecords[Index].StartTime = 0.8;
		InRecords[Index].EndTime = 0.8;

		Index = InRecords.AddDefaulted();
		InRecords[Index].StartTime = 0.9;
		InRecords[Index].EndTime = 1;

		TestTrue(TEXT("Input records assignment should succeed if all input is valid."), InputBuffer->SetHistoryRecords(InRecords));

		TArray<FInputHistoryRecord> OutRecords;
		InputBuffer->GetHistoryRecords(OutRecords);
		TestEqual(TEXT("Input history must be the same as given input records after assignment."), InRecords, OutRecords);

		// Get last events
		{
			TArray<FName> Events;
			TestEqual(TEXT("GetLastEvents should return the last events' end time."), InputBuffer->GetLastEvents(Events, 0.5, true), InRecords[0].EndTime);
			TestEqual(TEXT("GetLastEvents should return the last events in the input buffer."), Events, InRecords[0].Events);
		}

		// Get last events
		{
			TArray<FName> Events;
			TestEqual(TEXT("GetLastEvents should return the last events' end time."), InputBuffer->GetLastEvents(Events, 0.5, false), InRecords[1].EndTime);
			TestEqual(TEXT("GetLastEvents should return the last events in the input buffer."), Events, InRecords[1].Events);
		}

		// Event matching
		{
			TArray<FName> EventsToMatch;
			EventsToMatch.Add(TEXT("Punch"));

			TestFalse(TEXT("Event matching should fail if last events mismatch."), InputBuffer->MatchEvents(EventsToMatch, TArray<FName>(), 0.5, true));
		}

		// Event matching
		{
			TArray<FName> EventsToMatch;
			EventsToMatch.Add(TEXT("Punch"));

			TArray<FName> EventsToIgnore;
			EventsToIgnore.Add(TEXT("Forward"));

			TestTrue(TEXT("Event matching should succeed if last events match."), InputBuffer->MatchEvents(EventsToMatch, EventsToIgnore, 0.5, true));
		}
	}

	// Input history assignment with an unknown event
	{
		TArray<FInputHistoryRecord> InRecords;

		int32 Index = InRecords.AddDefaulted();
		InRecords[Index].Events.Add(TEXT("Punch"));
		InRecords[Index].TranslatedEvents.Add(TEXT("Right"));
		InRecords[Index].StartTime = 0.8;
		InRecords[Index].EndTime = 0.8;

		Index = InRecords.AddDefaulted();
		InRecords[Index].Events.Add(TEXT("Fire"));
		InRecords[Index].StartTime = 0.9;
		InRecords[Index].EndTime = 1;

		TestFalse(TEXT("Input records assignment should return failed if any input is invalid."), InputBuffer->SetHistoryRecords(InRecords));
	}

	TestFalse(TEXT("Command recognition should fail if given input command is NULL."), InputBuffer->MatchCommand(nullptr));

	// Post-conditions after invalidation
	{
		TArray<FInputHistoryRecord> BeforeRecords;
		InputBuffer->GetHistoryRecords(BeforeRecords, 0, true);

		InputBuffer->InvalidateHistory();

		TArray<FInputHistoryRecord> AfterRecords;
		InputBuffer->GetHistoryRecords(AfterRecords, 0, true);

		TestEqual(TEXT("The number of input history must be the same after invalidation."), BeforeRecords.Num(), AfterRecords.Num());

		for (const auto& Record : AfterRecords)
		{
			TestFalse(TEXT("Input records must be invalid after invalidation."), Record.bValid);
		}
	}

	// Post-conditions after clearance
	{
		InputBuffer->ClearHistory();

		TArray<FInputHistoryRecord> Records;
		InputBuffer->GetHistoryRecords(Records);
		TestEqual(TEXT("The number of input history must be zero after clearance."), Records.Num(), 0);

		auto InputCommand = NewObject<UInputCommand>();
		InputCommand->Sequences.AddDefaulted();
		InputCommand->Sequences[0].Entries.AddDefaulted();
		InputCommand->Sequences[0].Entries[0].EventsToMatch.Add(TEXT("Punch"));
		InputCommand->Sequences[0].Entries[0].EventsToIgnore.Add(TEXT("Kick"));

		TestFalse(TEXT("Command recognition should fail if input history is empty."), InputBuffer->MatchCommand(InputCommand));
	}

	return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS