#include "SDialogueEndGizmo.h"
#include "Dialogue/Graph/DialogueEndGizmo.h"
#include "EdGraph/EdGraphPin.h"
#include "SGraphPin.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
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

	UDialogueEndGizmo* Gizmo = Cast<UDialogueEndGizmo>(GraphNode);
	if (!Gizmo) return;

	TWeakObjectPtr<UDialogueEndGizmo> WeakGizmo(Gizmo);

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
				// No editable text field on End node header; keep layout simple
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8, 0, 0, 0)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.MinDesiredWidth(0.f)
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
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 2)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Events")))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(EventsListBox, SVerticalBox)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 4, 0, 0)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("+ Add Event")))
						.OnClicked_Lambda([this]()
						{
							if (UDialogueEndGizmo* G = Cast<UDialogueEndGizmo>(GraphNode))
							{
								G->Modify();
								G->EventNames.Add(NAME_None);
								if (UEdGraph* Graph = G->GetGraph()) Graph->NotifyGraphChanged();
								UpdateGraphNode();
							}
							return FReply::Handled();
						})
					]
				]
			]
		]
	];

	CreatePinWidgets();
	PopulateEventsList();
}

void SDialogueEndGizmo::PopulateEventsList()
{
	if (!EventsListBox.IsValid()) return;

	UDialogueEndGizmo* Gizmo = Cast<UDialogueEndGizmo>(GraphNode);
	if (!Gizmo) return;

	EventsListBox->ClearChildren();
	TWeakObjectPtr<UDialogueEndGizmo> WeakGizmo(Gizmo);

	for (int32 i = 0; i < Gizmo->EventNames.Num(); ++i)
	{
		const int32 Index = i;
		EventsListBox->AddSlot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox)
					.MinDesiredWidth(80.f)
					.Text(TAttribute<FText>::CreateLambda([WeakGizmo, Index]()
					{
						UDialogueEndGizmo* G = WeakGizmo.Get();
						if (!G || !G->EventNames.IsValidIndex(Index)) return FText::GetEmpty();
						return FText::FromName(G->EventNames[Index]);
					}))
					.OnTextCommitted(FOnTextCommitted::CreateLambda([WeakGizmo, Index](const FText& NewText, ETextCommit::Type)
					{
						if (UDialogueEndGizmo* G = WeakGizmo.Get())
						{
							if (!G->EventNames.IsValidIndex(Index)) return;
							G->Modify();
							G->EventNames[Index] = FName(*NewText.ToString());
							if (UEdGraph* Graph = G->GetGraph()) Graph->NotifyGraphChanged();
						}
					}))
					.HintText(FText::FromString(TEXT("Event name")))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4, 0, 0, 0)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("×")))
					.OnClicked_Lambda([this, Index]()
					{
						if (UDialogueEndGizmo* G = Cast<UDialogueEndGizmo>(GraphNode))
						{
							if (G->EventNames.IsValidIndex(Index))
							{
								G->Modify();
								G->EventNames.RemoveAt(Index);
								if (UEdGraph* Graph = G->GetGraph()) Graph->NotifyGraphChanged();
								UpdateGraphNode();
							}
						}
						return FReply::Handled();
					})
				]
			];
	}
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
