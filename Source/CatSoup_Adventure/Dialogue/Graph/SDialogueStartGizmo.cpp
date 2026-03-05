#include "SDialogueStartGizmo.h"
#include "Dialogue/Graph/DialogueStartGizmo.h"
#include "EdGraph/EdGraphPin.h"
#include "SGraphPin.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

void SDialogueStartGizmo::Construct(const FArguments& InArgs, UDialogueStartGizmo* InGizmo)
{
	GraphNode = InGizmo;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SDialogueStartGizmo::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	// Minimal gizmo: compact box with "Start" label and output pin (min width so pin is easy to grab)
	this->GetOrAddSlot(ENodeZone::Center)
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
		.BorderBackgroundColor(FLinearColor(0.15f, 0.4f, 0.15f, 0.9f))
		.Padding(FMargin(12, 6))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Start")))
				.Font(FAppStyle::GetFontStyle("NormalFontBold"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.MinWidth(28.f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(RightNodeBox, SVerticalBox)
			]
		]
	];

	CreatePinWidgets();
}

void SDialogueStartGizmo::CreatePinWidgets()
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
