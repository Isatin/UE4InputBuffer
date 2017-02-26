// Copyright 2017 Isaac Hsu. MIT License

#pragma once

#include "Engine.h"
#include "ModuleManager.h"
#include "UnrealEd.h"
#include "IAssetTools.h"
#include "IAssetTypeActions.h"
#include "AssetTypeCategories.h"

DECLARE_LOG_CATEGORY_EXTERN(InputBufferEditorLog, All, All)

class FInputBufferEditorModule : public IModuleInterface
{
public:

	FInputBufferEditorModule();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	/** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;

	EAssetTypeCategories::Type AssetCategoryBit;

private:

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);
};