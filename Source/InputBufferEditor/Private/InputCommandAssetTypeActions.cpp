// Copyright 2017 Isaac Hsu. MIT License

#include "InputBufferEditor.h"

#include "InputCommandAssetTypeActions.h"
#include "InputCommand.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FInputCommandAssetTypeActions

FInputCommandAssetTypeActions::FInputCommandAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: MyAssetCategory(InAssetCategory)
{
}

FText FInputCommandAssetTypeActions::GetName() const
{
	return LOCTEXT("FInputCommandAssetTypeActionsName", "Input Command");
}

FColor FInputCommandAssetTypeActions::GetTypeColor() const
{
	return FColor(196, 129, 115);
}

UClass* FInputCommandAssetTypeActions::GetSupportedClass() const
{
	return UInputCommand::StaticClass();
}

uint32 FInputCommandAssetTypeActions::GetCategories()
{
	return MyAssetCategory;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE