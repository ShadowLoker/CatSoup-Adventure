#pragma once

#include "SGraphNode.h"

class UDialogueEntryGizmo;

class SDialogueEntryGizmo : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SDialogueEntryGizmo) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UDialogueEntryGizmo* InGizmo);

	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
};
