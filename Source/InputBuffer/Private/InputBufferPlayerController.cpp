// Copyright 2017 Isaac Hsu. MIT License

#include "InputBufferPrivatePCH.h"
#include "Engine/InputDelegateBinding.h"
#include "DisplayDebugHelpers.h"

#include "InputBufferComponent.h"
#include "InputBufferPlayerController.h"


FName AInputBufferPlayerController::InputBufferComponentName(TEXT("InputBuffer"));

AInputBufferPlayerController::AInputBufferPlayerController()
{
	InputBuffer = CreateDefaultSubobject<UInputBufferComponent>(InputBufferComponentName);
}

void AInputBufferPlayerController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);

	check(InputBuffer);
	InputBuffer->OnPreProcessInput(PlayerInput, bGamePaused);
}

void AInputBufferPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PostProcessInput(DeltaTime, bGamePaused);

	check(InputBuffer);
	InputBuffer->OnPostProcessInput(PlayerInput, bGamePaused);
}

void AInputBufferPlayerController::DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	static FName NAME_InputBuffer = FName(TEXT("InputBuffer"));
	if (DebugDisplay.IsDisplayOn(NAME_InputBuffer))
	{
		FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
		check(InputBuffer);
		DisplayDebugManager.DrawString(FString::Printf(TEXT("InputBuffer: %s"), *InputBuffer->Print(MAX_INPUT_HISTORY_TO_DEBUG_DISPLAY, true)));
	}
}

void AInputBufferPlayerController::PostBufferInput_Implementation()
{
}

FName AInputBufferPlayerController::TranslateInputEvent_Implementation(FName Event)
{
	return Event;
}
