#include "SDialogueEntryGizmo.h"
#include "Dialogue/Graph/DialogueEntryGizmo.h"
#include "EdGraph/EdGraphPin.h"
#include "SGraphPin.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

void SDialogueEntryGizmo::Construct(const FArguments& InArgs, UDialogueEntryGizmo* InGizmo)
{
	GraphNode = InGizmo;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SDialogueEntryGizmo::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	UDialogueEntryGizmo* Gizmo = Cast<UDialogueEntryGizmo>(GraphNode);
	if (!Gizmo) return;

	TWeakObjectPtr<UDialogueEntryGizmo> WeakGizmo(Gizmo);

	// Match Start node layout: label on left, output pin on right (easy to drag wire from)
	this->GetOrAddSlot(ENodeZone::Center)
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
		.BorderBackgroundColor(FLinearColor(0.1f, 0.25f, 0.55f, 0.95f))
		.Padding(FMargin(12, 6))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.MinDesiredWidth(80.f)
				.Text(TAttribute<FText>::CreateLambda([WeakGizmo]()
				{
					UDialogueEntryGizmo* G = WeakGizmo.Get();
					return G ? FText::FromName(G->EntryPointId) : FText::GetEmpty();
				}))
				.OnTextCommitted(FOnTextCommitted::CreateLambda([WeakGizmo](const FText& NewText, ETextCommit::Type)
				{
					if (UDialogueEntryGizmo* G = WeakGizmo.Get())
					{
						G->Modify();
						G->EntryPointId = FName(*NewText.ToString());
						if (UEdGraph* Graph = G->GetGraph()) Graph->NotifyGraphChanged();
					}
				}))
				.HintText(FText::FromString(TEXT("Return, Continue...")))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.MinWidth(36.f)
			.Padding(8, 0, 0, 0)
			.VAlign(VAlign_Center)
			[
				SAssignNew(RightNodeBox, SVerticalBox)
			]
		]
	];

	CreatePinWidgets();
}

void SDialogueEntryGizmo::CreatePinWidgets()
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
