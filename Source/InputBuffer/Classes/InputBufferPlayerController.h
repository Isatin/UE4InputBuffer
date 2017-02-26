// Copyright 2017 Isaac Hsu. MIT License

#pragma once

#include "GameFramework/PlayerController.h"
#include "InputBufferComponent.h"
#include "InputBufferPlayerController.generated.h"


/**
* A player controller with an input buffer.
*
* Caution: Developers should reparent their player controllers to this class if they need input buffering. 
*
**/
UCLASS( BlueprintType, Blueprintable )
class INPUTBUFFER_API AInputBufferPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	AInputBufferPlayerController();

public:

	static const int32 MAX_INPUT_HISTORY_TO_DEBUG_DISPLAY = 20;

	/** Name of the input buffer component. Use this name if you want to use a different class (with ObjectInitializer.SetDefaultSubobjectClass). */
	static FName InputBufferComponentName;

	/** Component of input buffer */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input Buffer")
	class UInputBufferComponent* InputBuffer;

public:

	//~ Begin AActor Interface
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
	//~ End AActor Interface

	//~ Begin APlayerController Interface
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~ End APlayerController Interface

	/* Called when a new input is just buffered.	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Input Buffer")
	void PostBufferInput();

	/* Developers can override this function to translate a received input event. */
	UFUNCTION(BlueprintNativeEvent, Category = "Input Buffer")
	FName TranslateInputEvent(FName Event);

};
