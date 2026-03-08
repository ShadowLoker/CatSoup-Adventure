#if WITH_EDITOR
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
	LeftNodeBox.Reset();
	RightNodeBox.Reset();

	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	UDialogueEntryGizmo* Gizmo = Cast<UDialogueEntryGizmo>(GraphNode);
	if (!Gizmo) return;

	TWeakObjectPtr<UDialogueEntryGizmo> WeakGizmo(Gizmo);

	// Layout: [Input pin] [ID label] [Output pin]. Input = "next start" from End node.
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
			.MinWidth(28.f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(LeftNodeBox, SVerticalBox)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8, 0)
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
						FString S = NewText.ToString().TrimStartAndEnd();
						if (S.Equals(TEXT("Default"), ESearchCase::IgnoreCase))
						{
							return; // Reserved: cannot name entry point "Default"
						}
						G->Modify();
						G->EntryPointId = FName(*S);
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

void SDialogueEntryGizmo::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
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
					SNew(SBox)
					.MinDesiredWidth(28.f)
					.MinDesiredHeight(24.f)
					[
						PinToAdd
					]
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
					SNew(SBox)
					.MinDesiredWidth(28.f)
					.MinDesiredHeight(24.f)
					[
						PinToAdd
					]
				];
		}
		OutputPins.Add(PinToAdd);
	}
}

#endif
