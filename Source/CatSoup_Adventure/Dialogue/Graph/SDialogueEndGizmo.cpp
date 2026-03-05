#include "SDialogueEndGizmo.h"
#include "Dialogue/Graph/DialogueEndGizmo.h"
#include "EdGraph/EdGraphPin.h"
#include "SGraphPin.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

void SDialogueEndGizmo::Construct(const FArguments& InArgs, UDialogueEndGizmo* InGizmo)
{
	GraphNode = InGizmo;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SDialogueEndGizmo::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	this->GetOrAddSlot(ENodeZone::Center)
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
		.BorderBackgroundColor(FLinearColor(0.4f, 0.15f, 0.15f, 0.9f))
		.Padding(FMargin(12, 6))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SAssignNew(LeftNodeBox, SVerticalBox)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(12, 0, 0, 0)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("End")))
				.Font(FAppStyle::GetFontStyle("NormalFontBold"))
			]
		]
	];

	CreatePinWidgets();
}

void SDialogueEndGizmo::CreatePinWidgets()
{
	for (UEdGraphPin* CurPin : GraphNode->Pins)
	{
		if (!CurPin) continue;

		TSharedPtr<SGraphPin> NewPin = CreatePinWidget(CurPin);
		if (NewPin.IsValid())
		{
			NewPin->SetShowLabel(false);
			AddPin(NewPin.ToSharedRef());
		}
	}
}
