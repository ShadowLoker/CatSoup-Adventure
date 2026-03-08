// cpp
// Source/CatSoup_Adventure/Dialogue/Graph/DialogueAssetEditor.cpp
#if WITH_EDITOR
#include "DialogueAssetEditorToolkit.h"

#include "GraphEditor.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "ScopedTransaction.h"
#include "Dialogue/Schema/DialogueGraphSchema.h"
#include "Dialogue/Graph/DialogueEndGizmo.h"
#include "Dialogue/Graph/DialogueEntryGizmo.h"

#include "IDetailsView.h"
#include "Framework/Commands/GenericCommands.h"

#define LOCTEXT_NAMESPACE "DialogueAssetEditor"

const FName DialogueEditorAppIdentifier = TEXT("DialogueAssetEditorApp");
const FName DialogueEditorGraphTabId = TEXT("DialogueAssetEditor_Graph");
const FName FDialogueAssetEditorToolkit::DialogueEditorDetailsTabId(TEXT("DialogueEditor_Details"));

static void EnsureEndGizmo(UDialogueGraph* Graph)
{
    if (!Graph) return;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Cast<UDialogueEndGizmo>(Node))
        {
            return; // End gizmo already exists
        }
    }

    Graph->Modify();

    UDialogueEndGizmo* Gizmo = NewObject<UDialogueEndGizmo>(Graph, UDialogueEndGizmo::StaticClass(), NAME_None, RF_Transactional);
    Gizmo->Modify();
    Gizmo->NodePosX = 200;
    Gizmo->NodePosY = 0;

    Graph->AddNode(Gizmo, true, true);
    Gizmo->AllocateDefaultPins();
    Graph->NotifyGraphChanged();
}

static void EnsureStartGizmo(UDialogueGraph* Graph)
{
    if (!Graph) return;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Cast<UDialogueStartGizmo>(Node))
        {
            return; // Gizmo already exists
        }
    }

    Graph->Modify();

    UDialogueStartGizmo* Gizmo = NewObject<UDialogueStartGizmo>(Graph, UDialogueStartGizmo::StaticClass(), NAME_None, RF_Transactional);
    Gizmo->Modify();
    Gizmo->NodePosX = 0;
    Gizmo->NodePosY = 0;

    Graph->AddNode(Gizmo, true, true);
    Gizmo->AllocateDefaultPins();
    Graph->NotifyGraphChanged();
}

void FDialogueAssetEditorToolkit::Initialize(UDialogueAsset* InDialogueAsset)
{
    check(InDialogueAsset);
    DialogueAsset = InDialogueAsset;

    GraphCommandList = MakeShared<FUICommandList>();

    GraphCommandList->MapAction(
        FGenericCommands::Get().Delete,
        FExecuteAction::CreateRaw(this, &FDialogueAssetEditorToolkit::DeleteSelectedNodes),
        FCanExecuteAction::CreateRaw(this, &FDialogueAssetEditorToolkit::CanDeleteSelectedNodes)
    );

    // Ensure we have a graph object owned by the asset and a valid schema
    if (!DialogueAsset->EditorGraph)
    {
        DialogueAsset->EditorGraph = NewObject<UDialogueGraph>(DialogueAsset.Get(), NAME_None, RF_Transactional);
        DialogueAsset->EditorGraph->Schema = UDialogueGraphSchema::StaticClass();
    }
    DialogueGraph = DialogueAsset->EditorGraph;

    EnsureStartGizmo(DialogueGraph);
    EnsureEndGizmo(DialogueGraph);

    for (UEdGraphNode* Node : DialogueGraph->Nodes)
    {
        if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
        {
            DNode->ReconstructNode();
        }
        else if (UDialogueEndGizmo* EndGizmo = Cast<UDialogueEndGizmo>(Node))
        {
            EndGizmo->ReconstructNode();
        }
        else if (UDialogueEntryGizmo* EntryGizmo = Cast<UDialogueEntryGizmo>(Node))
        {
            EntryGizmo->ReconstructNode();
        }
    }
    DialogueGraph->NotifyGraphChanged();

    FPropertyEditorModule& PropertyEditorModule =
    FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.bAllowSearch = true;
    DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);

    // Setup the editor layout: one primary vertical tab containing the graph editor
    const TSharedRef<FTabManager::FLayout> Layout =
    FTabManager::NewLayout("Standalone_DialogueEditor_Layout_v2")
    ->AddArea(
        FTabManager::NewPrimaryArea()
        ->SetOrientation(Orient_Horizontal)
        ->Split(
            FTabManager::NewStack()
            ->AddTab(DialogueEditorGraphTabId, ETabState::OpenedTab)
            ->SetHideTabWell(true)
            ->SetSizeCoefficient(0.7f)
        )
        ->Split(
            FTabManager::NewStack()
            ->AddTab(DialogueEditorDetailsTabId, ETabState::OpenedTab)
            ->SetHideTabWell(true)
            ->SetSizeCoefficient(0.3f)
        )
    );

    // InitAssetEditor opens the editor UI with supplied layout and assets
    InitAssetEditor(
        EToolkitMode::Standalone,
        nullptr,
        DialogueEditorAppIdentifier,
        Layout,
        true,	// bCreateDefaultStandaloneMenu
        true,	// bCreateDefaultToolbar
        InDialogueAsset
    );
    WorkspaceMenuCategory = FWorkspaceItem::NewGroup(LOCTEXT("DialogueCategory", "Dialogue"));
}

TSharedRef<SDockTab> FDialogueAssetEditorToolkit::SpawnTab_GraphEditor(const FSpawnTabArgs& Args)
{
    check(DialogueGraph);

    if (!GraphEditor.IsValid())
    {
        SGraphEditor::FGraphEditorEvents Events;
        Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateRaw(
            this, &FDialogueAssetEditorToolkit::OnGraphSelectionChanged);

        GraphEditor = SNew(SGraphEditor)
            .AdditionalCommands(GraphCommandList)
            .GraphToEdit(DialogueGraph)
            .IsEditable(true)
            .ShowGraphStateOverlay(false)
            .GraphEvents(Events);
    }

    return SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
        [
            GraphEditor.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FDialogueAssetEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .Label(LOCTEXT("DialogueDetailsTabTitle", "Details"))
        .TabRole(ETabRole::PanelTab)
        [
            DetailsView.ToSharedRef()
        ];
}

void FDialogueAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    InTabManager->RegisterTabSpawner(
        DialogueEditorGraphTabId,
        FOnSpawnTab::CreateRaw(this, &FDialogueAssetEditorToolkit::SpawnTab_GraphEditor)
    );

    InTabManager->RegisterTabSpawner(
        DialogueEditorDetailsTabId,
        FOnSpawnTab::CreateRaw(this, &FDialogueAssetEditorToolkit::SpawnTab_Details)
    );
}

void FDialogueAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    InTabManager->UnregisterTabSpawner(DialogueEditorGraphTabId);
    InTabManager->UnregisterTabSpawner(DialogueEditorDetailsTabId);

    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

// Required overrides to avoid abstract class error
FName FDialogueAssetEditorToolkit::GetToolkitFName() const
{
    return FName("DialogueEditor");
}

FText FDialogueAssetEditorToolkit::GetBaseToolkitName() const
{
    return LOCTEXT("AppLabel", "Dialogue Editor");
}

FString FDialogueAssetEditorToolkit::GetWorldCentricTabPrefix() const
{
    return TEXT("DialogueEditor");
}

FLinearColor FDialogueAssetEditorToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor::White;
}

void FDialogueAssetEditorToolkit::OnGraphSelectionChanged(const TSet<UObject*>& NewSelection)
{
    if (!DetailsView.IsValid())
        return;

    if (NewSelection.Num() == 1)
    {
        UObject* Selected = *NewSelection.CreateConstIterator();
        DetailsView->SetObject(Selected);
    }
    else
    {
        DetailsView->SetObject(nullptr);
    }
}

bool FDialogueAssetEditorToolkit::CanDeleteSelectedNodes() const
{
    return GraphEditor.IsValid() && GraphEditor->GetSelectedNodes().Num() > 0;
}

void FDialogueAssetEditorToolkit::DeleteSelectedNodes()
{
    if (!GraphEditor.IsValid() || !DialogueGraph) return;

    const FScopedTransaction Transaction(NSLOCTEXT("DialogueGraph", "DeleteNodes", "Delete Dialogue Nodes"));
    DialogueGraph->Modify();

    const FGraphPanelSelectionSet Selected = GraphEditor->GetSelectedNodes();

    for (UObject* Obj : Selected)
    {
        if (UEdGraphNode* Node = Cast<UEdGraphNode>(Obj))
        {
            if (!Node->CanUserDeleteNode())
                continue;

            Node->Modify();
            Node->DestroyNode();          // removes from graph + breaks links
        }
    }

    EnsureStartGizmo(DialogueGraph);
    EnsureEndGizmo(DialogueGraph);
}

#undef LOCTEXT_NAMESPACE

#endif
