#pragma once

#include "SGraphNode.h"

class UDialogueEndGizmo;

class SDialogueEndGizmo : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SDialogueEndGizmo) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UDialogueEndGizmo* InGizmo);

	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
};
