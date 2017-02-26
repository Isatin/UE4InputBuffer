// Copyright 2017 Isaac Hsu. MIT License

#pragma once

#include "Factories/Factory.h"
#include "InputCommandFactory.generated.h"

/**
 * 
 */
UCLASS()
class INPUTBUFFEREDITOR_API UInputCommandFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface	
};
