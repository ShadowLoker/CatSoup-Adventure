#include "DialogueGraphSchema.h"
#include "Dialogue/Graph/DialogueGraphNode.h"
#include "Dialogue/Graph/DialogueGraphPins.h"


#define LOCTEXT_NAMESPACE "DialogueGraphSchema"

namespace DialogueSchemaActions
{
    struct FDialogueNewNodeAction : public FEdGraphSchemaAction
    {
        EDialogueGraphNodeType Type;

        FDialogueNewNodeAction(
            const FText& InCategory,
            const FText& InMenuDesc,
            const FText& InToolTip,
            int32 InGrouping,
            EDialogueGraphNodeType InType)
            : FEdGraphSchemaAction(InCategory, InMenuDesc, InToolTip, InGrouping)
            , Type(InType)
        {}

        virtual UEdGraphNode* PerformAction(
            UEdGraph* ParentGraph,
            UEdGraphPin* FromPin,
            const FVector2D Location,
            bool bSelectNewNode) override
        {
            if (!ParentGraph)
            {
                return nullptr;
            }

            // Enforce single Start node
            if (Type == EDialogueGraphNodeType::Start)
            {
                for (UEdGraphNode* Node : ParentGraph->Nodes)
                {
                    const UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node);
                    if (DNode && DNode->NodeType == EDialogueGraphNodeType::Start)
                    {
                        return nullptr;
                    }
                }
            }

            const FScopedTransaction Transaction(
                NSLOCTEXT("DialogueGraph", "AddDialogueNode", "Add Dialogue Node"));

            ParentGraph->Modify();

            // Create node (Outer = graph)
            UDialogueGraphNode* NewNode = NewObject<UDialogueGraphNode>(
                ParentGraph,
                UDialogueGraphNode::StaticClass(),
                NAME_None,
                RF_Transactional);

            if (!NewNode)
            {
                return nullptr;
            }

            NewNode->Modify();
            NewNode->NodeType = Type;

            // Stable id
            if (NewNode->NodeId.IsNone())
            {
                NewNode->NodeId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
            }

            // Defaults
            if (Type == EDialogueGraphNodeType::Dialogue)
            {
                if (NewNode->NodeData.Text.IsEmpty())
                {
                    NewNode->NodeData.Text = FText::FromString(TEXT("..."));
                }

                if (NewNode->NodeData.Outputs.Num() == 0)
                {
                    NewNode->NodeData.Outputs.AddDefaulted();
                    NewNode->NodeData.Outputs[0].Text = FText::FromString(TEXT("Next"));
                }
            }
            else if (Type == EDialogueGraphNodeType::Start)
            {
                // Start node doesn't need dialogue outputs data (pins are handled by AllocateDefaultPins)
                NewNode->NodeData.Outputs.Empty();
            }

            // Position
            NewNode->NodePosX = (int32)Location.X;
            NewNode->NodePosY = (int32)Location.Y;

            // Add + build pins
            ParentGraph->AddNode(NewNode, /*bFromUI=*/true, bSelectNewNode);
            NewNode->AllocateDefaultPins();

            // If created from an existing pin, try to connect it
            NewNode->AutowireNewNode(FromPin);

            // Notify editor
            NewNode->PostEditChange();
            ParentGraph->NotifyGraphChanged();

            return NewNode;
        }
    };
}

void UDialogueGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    const FText Category = LOCTEXT("DialogueCategory", "Dialogue");

    ContextMenuBuilder.AddAction(MakeShared<DialogueSchemaActions::FDialogueNewNodeAction>(
        Category,
        LOCTEXT("AddStartNode", "Add Start Node"),
        LOCTEXT("AddStartNodeTooltip", "Creates the start node of the dialogue."),
        0,
        EDialogueGraphNodeType::Start));

    ContextMenuBuilder.AddAction(MakeShared<DialogueSchemaActions::FDialogueNewNodeAction>(
        Category,
        LOCTEXT("AddDialogueNode", "Add Dialogue Node"),
        LOCTEXT("AddDialogueNodeTooltip", "Creates a dialogue node."),
        0,
        EDialogueGraphNodeType::Dialogue));
}

const FPinConnectionResponse UDialogueGraphSchema::CanCreateConnection(
    const UEdGraphPin* A, const UEdGraphPin* B) const
{
    if (A->PinType.PinCategory != DialogueGraphPins::Flow ||
    B->PinType.PinCategory != DialogueGraphPins::Flow)
    {
        UE_LOG(LogTemp, Warning, TEXT("A=%s (%s)  B=%s (%s)"),
        *A->PinName.ToString(), *A->PinType.PinCategory.ToString(),
        *B->PinName.ToString(), *B->PinType.PinCategory.ToString());
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Only dialogue flow connections allowed"));
    }

    if (A->Direction == B->Direction)
    {
        UE_LOG(LogTemp, Warning, TEXT("A=%s (%s)  B=%s (%s)"),
        *A->PinName.ToString(), *A->PinType.PinCategory.ToString(),
        *B->PinName.ToString(), *B->PinType.PinCategory.ToString());
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Pins must be opposite directions"));
    }

    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

#undef LOCTEXT_NAMESPACE
