// Copyright 2017 Isaac Hsu. MIT License

#include "InputBufferEditor.h"
#include "InputCommandThumbnailRenderer.h"
#include "InputCommand.h"

UTexture2D* UInputCommandThumbnailRenderer::DefaultTexture;

UInputCommandThumbnailRenderer::UInputCommandThumbnailRenderer()
{
	if (DefaultTexture == nullptr)
	{
		DefaultTexture = LoadObject<UTexture2D>(nullptr, TEXT("/InputBuffer/T_InputCommand"), nullptr, LOAD_None, nullptr);
	}
}

void UInputCommandThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas)
{
	UInputCommand* InputCommand = Cast<UInputCommand>(Object);
	if (InputCommand == nullptr) return;

	if (InputCommand->Thumbnail)
	{
		DrawTexture(InputCommand->Thumbnail, X, Y, Width, Height, Canvas);
	}
	else
	{
		DrawTexture(DefaultTexture, X, Y, Width, Height, Canvas);
	}
}

void UInputCommandThumbnailRenderer::DrawTexture(UTexture2D* Texture, int32 X, int32 Y, uint32 Width, uint32 Height, FCanvas* Canvas)
{
	if (Texture)
	{
		Canvas->DrawTile(
			(float)X,
			(float)Y,
			(float)Width,
			(float)Height,
			0.0f,
			0.0f,
			1.0f,
			1.0f,
			FLinearColor::White,
			Texture->Resource,
			true);
	}
}

