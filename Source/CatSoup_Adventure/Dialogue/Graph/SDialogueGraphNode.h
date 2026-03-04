#pragma once

#include "SGraphNode.h"

class UDialogueGraphNode;

class SDialogueGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SDialogueGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UDialogueGraphNode* InNode);

	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;

private:
	UDialogueGraphNode* DNode = nullptr;
	TSharedPtr<SVerticalBox> OutputLabelsBox;

	FReply OnAddOutputClicked();
};