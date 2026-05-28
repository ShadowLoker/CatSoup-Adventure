#pragma once

#include "Modules/ModuleManager.h"

class FDialogueSystemEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<class FDialogueAssetTypeActions> DialogueAssetTypeActions;
	TSharedPtr<class FDialogueGraphNodeFactory> DialogueNodeFactory;
};
