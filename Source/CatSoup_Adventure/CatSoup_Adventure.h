#pragma once

#include "Modules/ModuleManager.h"

class FCatSoup_Adventure : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<class FDialogueGraphNodeFactory> DialogueNodeFactory;
};