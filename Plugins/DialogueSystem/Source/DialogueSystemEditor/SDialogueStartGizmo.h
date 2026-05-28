#pragma once

#include "SGraphNode.h"

class UDialogueStartGizmo;

class SDialogueStartGizmo : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SDialogueStartGizmo) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UDialogueStartGizmo* InGizmo);

	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
};
