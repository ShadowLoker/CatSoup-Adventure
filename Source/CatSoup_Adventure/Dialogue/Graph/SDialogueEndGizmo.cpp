#if WITH_EDITOR
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
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.MinWidth(28.f)
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
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.MinWidth(28.f)
				.Padding(8, 0, 0, 0)
				.VAlign(VAlign_Center)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 6, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Actions are edited in the Details panel.")))
				.Font(FAppStyle::GetFontStyle("SmallFont"))
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

void SDialogueEndGizmo::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	if (!PinObj) return;

	if (PinToAdd->GetDirection() == EGPD_Input)
	{
		if (LeftNodeBox.IsValid())
		{
			LeftNodeBox->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					PinToAdd
				];
		}
		InputPins.Add(PinToAdd);
	}
	else
	{
		if (RightNodeBox.IsValid())
		{
			RightNodeBox->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					PinToAdd
				];
		}
		OutputPins.Add(PinToAdd);
	}
}

#endif
