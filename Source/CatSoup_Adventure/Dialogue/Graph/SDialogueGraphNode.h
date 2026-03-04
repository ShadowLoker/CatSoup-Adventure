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

	FText GetSpeakerText() const;
	void OnSpeakerCommitted(const FText& NewText, ETextCommit::Type CommitType);

	FText GetLineText() const;
	void OnLineCommitted(const FText& NewText, ETextCommit::Type CommitType);

	FReply OnAddOutputClicked();

	FText GetOutputText(int32 Index) const;
	void OnOutputTextCommitted(const FText& NewText, ETextCommit::Type CommitType, int32 Index);
};