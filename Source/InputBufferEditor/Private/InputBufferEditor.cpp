// Copyright 2017 Isaac Hsu. MIT License

#include "InputBufferEditor.h"
#include "InputCommand.h"
#include "InputCommandThumbnailRenderer.h"
#include "InputCommandAssetTypeActions.h"

IMPLEMENT_GAME_MODULE(FInputBufferEditorModule, InputBufferEditor);

DEFINE_LOG_CATEGORY(InputBufferEditorLog)

#define LOCTEXT_NAMESPACE "InputBufferEditor"

FInputBufferEditorModule::FInputBufferEditorModule()
	: AssetCategoryBit(EAssetTypeCategories::Misc)
{
}

void FInputBufferEditorModule::StartupModule()
{
	UThumbnailManager::Get().RegisterCustomRenderer(UInputCommand::StaticClass(), UInputCommandThumbnailRenderer::StaticClass());

	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	AssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Input")), LOCTEXT("InputCategory", "Input"));

	RegisterAssetTypeAction(AssetTools, MakeShareable(new FInputCommandAssetTypeActions(AssetCategoryBit)));
}

void FInputBufferEditorModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UInputCommand::StaticClass());
	}

	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}
	CreatedAssetTypeActions.Empty();
}

void FInputBufferEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE