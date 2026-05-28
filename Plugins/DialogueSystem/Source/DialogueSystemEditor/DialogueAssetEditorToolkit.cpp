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
#include "DialogueGraphSchema.h"
#include "DialogueEndGizmo.h"
#include "DialogueEntryGizmo.h"

#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
    DialogueGraph = Cast<UDialogueGraph>(DialogueAsset->EditorGraph);

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
    RegisterMenus();
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

void FDialogueAssetEditorToolkit::RegisterMenus()
{
	// FAssetEditorToolkit::InitAssetEditor registers menus under:
	// "AssetEditor.<GetToolkitFName()>.MainMenu.Asset"  and
	// "AssetEditor.<GetToolkitFName()>.ToolBar"
	const FString ToolkitName = GetToolkitFName().ToString();
	const FName AssetMenuName  = FName(*(FString("AssetEditor.") + ToolkitName + FString(".MainMenu.Asset")));
	const FName ToolbarMenuName = FName(*(FString("AssetEditor.") + ToolkitName + FString(".ToolBar")));

	UE_LOG(LogTemp, Warning, TEXT("[DialogueEditor] Extending menu '%s' and toolbar '%s'"),
		*AssetMenuName.ToString(), *ToolbarMenuName.ToString());

	// ------- Asset Menu entries -------
	UToolMenu* AssetMenu = UToolMenus::Get()->ExtendMenu(AssetMenuName);
	if (AssetMenu)
	{
		FToolMenuSection& Section = AssetMenu->FindOrAddSection("MermaidRoundtripping");
		Section.Label = LOCTEXT("MermaidRoundtrippingSection", "Mermaid Roundtripping");

		Section.AddMenuEntry(
			"ImportMermaid",
			LOCTEXT("ImportMermaid_Label", "Import Mermaid"),
			LOCTEXT("ImportMermaid_Tooltip", "Import a Dialogue Graph from a Markdown/Mermaid file"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Import"),
			FToolUIActionChoice(FExecuteAction::CreateSP(this, &FDialogueAssetEditorToolkit::ImportMermaid))
		);
		Section.AddMenuEntry(
			"ExportMermaid",
			LOCTEXT("ExportMermaid_Label", "Export Mermaid"),
			LOCTEXT("ExportMermaid_Tooltip", "Export the current Dialogue Graph to a Markdown/Mermaid file"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Export"),
			FToolUIActionChoice(FExecuteAction::CreateSP(this, &FDialogueAssetEditorToolkit::ExportMermaid))
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[DialogueEditor] Menu '%s' not found — Mermaid menu entries skipped."), *AssetMenuName.ToString());
	}

	// ------- Toolbar buttons (visible regardless of menu name issues) -------
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(ToolbarMenuName);
	if (ToolbarMenu)
	{
		FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("MermaidRoundtripping");
		ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			"ImportMermaid",
			FUIAction(FExecuteAction::CreateSP(this, &FDialogueAssetEditorToolkit::ImportMermaid)),
			LOCTEXT("ImportMermaid_Label", "Import Mermaid"),
			LOCTEXT("ImportMermaid_Tooltip", "Import a Dialogue Graph from a Markdown/Mermaid file"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Import")
		));
		ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			"ExportMermaid",
			FUIAction(FExecuteAction::CreateSP(this, &FDialogueAssetEditorToolkit::ExportMermaid)),
			LOCTEXT("ExportMermaid_Label", "Export Mermaid"),
			LOCTEXT("ExportMermaid_Tooltip", "Export the current Dialogue Graph to a Markdown/Mermaid file"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Export")
		));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[DialogueEditor] Toolbar '%s' not found."), *ToolbarMenuName.ToString());
	}
}


void FDialogueAssetEditorToolkit::FillMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("MermaidRoundtripping", LOCTEXT("MermaidRoundtrippingSection", "Mermaid Roundtripping"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ImportMermaid_Label", "Import Mermaid"),
			LOCTEXT("ImportMermaid_Tooltip", "Import a Dialogue Graph from a Markdown/Mermaid file"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Import"),
			FUIAction(FExecuteAction::CreateSP(this, &FDialogueAssetEditorToolkit::ImportMermaid))
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ExportMermaid_Label", "Export Mermaid"),
			LOCTEXT("ExportMermaid_Tooltip", "Export the current Dialogue Graph to a Markdown/Mermaid file"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Export"),
			FUIAction(FExecuteAction::CreateSP(this, &FDialogueAssetEditorToolkit::ExportMermaid))
		);
	}
	MenuBuilder.EndSection();
}

void FDialogueAssetEditorToolkit::ImportMermaid()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return;

	TArray<FString> OpenFilenames;
	const FString DefaultPath = FPaths::ProjectContentDir();
	const FString FileTypes = TEXT("Markdown Files (*.md)|*.md|All Files (*.*)|*.*");

	bool bOpened = DesktopPlatform->OpenFileDialog(
		nullptr,
		TEXT("Import Dialogue from Mermaid Markdown"),
		DefaultPath,
		TEXT(""),
		FileTypes,
		EFileDialogFlags::None,
		OpenFilenames
	);

	if (!bOpened || OpenFilenames.Num() == 0) return;

	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *OpenFilenames[0])) return;

	// Extract Mermaid block
	FString MermaidContent;
	int32 StartIdx = FileContent.Find(TEXT("```mermaid"));
	if (StartIdx != INDEX_NONE)
	{
		int32 ContentStart = StartIdx + 10;
		int32 EndIdx = FileContent.Find(TEXT("```"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ContentStart);
		if (EndIdx != INDEX_NONE)
		{
			MermaidContent = FileContent.Mid(ContentStart, EndIdx - ContentStart);
		}
	}
	else
	{
		MermaidContent = FileContent;
	}

	MermaidContent.ReplaceInline(TEXT("\r"), TEXT(""));
	TArray<FString> Lines;
	MermaidContent.ParseIntoArray(Lines, TEXT("\n"), true);

	struct FParsedNode
	{
		FString Id;
		FString Text;
		FString Type; // "Start", "End", "Entry", "Dialogue"
		FString SpeakerId;
		FLinearColor Color = FLinearColor(-1.f, -1.f, -1.f, -1.f); // invalid = not set
	};

	struct FParsedConnection
	{
		FString FromId;
		FString ToId;
		FString Label;
	};

	TMap<FString, FParsedNode> ParsedNodes;
	TArray<FParsedConnection> Connections;

	// pos metadata (outside mermaid block)
	TMap<FString, FVector2D> ParsedPositions;
	// Parse %% pos: lines from the whole file (not just mermaid block)
	{
		FString FullContent;
		FFileHelper::LoadFileToString(FullContent, *OpenFilenames[0]);
		FullContent.ReplaceInline(TEXT("\r"), TEXT(""));
		TArray<FString> AllLines;
		FullContent.ParseIntoArray(AllLines, TEXT("\n"), true);
		for (const FString& PosLine : AllLines)
		{
			FString Trimmed = PosLine.TrimStartAndEnd();
			if (Trimmed.StartsWith(TEXT("%% pos:")))
			{
				// format: %% pos:NodeId:X:Y
				FString Data = Trimmed.Mid(7); // strip "%% pos:"
				TArray<FString> Parts;
				Data.ParseIntoArray(Parts, TEXT(":"), false);
				if (Parts.Num() == 3)
				{
					ParsedPositions.Add(Parts[0].TrimStartAndEnd(),
						FVector2D(FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2])));
				}
			}
		}
	}

	for (FString Line : Lines)
	{
		Line = Line.TrimStartAndEnd();
		if (Line.IsEmpty() || Line.StartsWith(TEXT("graph")) || Line.StartsWith(TEXT("flowchart")))
		{
			continue;
		}

		// Parse Mermaid style lines: "style NodeId fill:#rrggbb"
		if (Line.StartsWith(TEXT("style ")))
		{
			FString Rest = Line.Mid(6).TrimStart();
			int32 FillIdx = Rest.Find(TEXT("fill:#"));
			if (FillIdx != INDEX_NONE)
			{
				FString NodeId = Rest.Left(FillIdx).TrimStartAndEnd();
				FString HexStr = Rest.Mid(FillIdx + 6, 6); // 6 hex chars
				if (HexStr.Len() == 6)
				{
					uint32 R = FParse::HexNumber(*HexStr.Left(2));
					uint32 G = FParse::HexNumber(*HexStr.Mid(2, 2));
					uint32 B = FParse::HexNumber(*HexStr.Right(2));
					FLinearColor Col = FLinearColor(R / 255.f, G / 255.f, B / 255.f, 0.8f);
					if (FParsedNode* Found = ParsedNodes.Find(NodeId))
						Found->Color = Col;
					else
					{
						FParsedNode N; N.Id = NodeId; N.Color = Col;
						ParsedNodes.Add(NodeId, N);
					}
				}
			}
			continue; // style lines are not nodes/connections
		}

		// Skip %% comment lines
		if (Line.StartsWith(TEXT("%%"))) continue;

		// Delimiters to scan
		struct FDelimPair
		{
			FString OpenDelim;
			FString CloseDelim;
			FString NodeType;
		};

		TArray<FDelimPair> Delims = {
			{ TEXT("(["), TEXT("])"), TEXT("EntryOrStart") },
			{ TEXT("[["), TEXT("]]"), TEXT("End") },
			{ TEXT("[\""), TEXT("\"]"), TEXT("Dialogue") },
			{ TEXT("["), TEXT("]"), TEXT("Dialogue") },
			{ TEXT("{\""), TEXT("\"}"), TEXT("Dialogue") },
			{ TEXT("{"), TEXT("}"), TEXT("Dialogue") }
		};

		bool bFoundShape = true;
		while (bFoundShape)
		{
			bFoundShape = false;
			for (const FDelimPair& Pair : Delims)
			{
				int32 SIdx = Line.Find(Pair.OpenDelim);
				if (SIdx != INDEX_NONE)
				{
					int32 EIdx = Line.Find(Pair.CloseDelim, ESearchCase::CaseSensitive, ESearchDir::FromStart, SIdx + Pair.OpenDelim.Len());
					if (EIdx != INDEX_NONE)
					{
						FString Content = Line.Mid(SIdx + Pair.OpenDelim.Len(), EIdx - SIdx - Pair.OpenDelim.Len()).TrimStartAndEnd();
						if (Content.StartsWith(TEXT("\"")) && Content.EndsWith(TEXT("\"")))
						{
							Content = Content.Mid(1, Content.Len() - 2);
						}

						// Scan backwards for Node ID
						int32 IdEnd = SIdx;
						int32 IdStart = IdEnd;
						while (IdStart > 0)
						{
							TCHAR C = Line[IdStart - 1];
							if (FChar::IsAlnum(C) || C == '_' || C == '-')
							{
								IdStart--;
							}
							else
							{
								break;
							}
						}

						FString NodeId = Line.Mid(IdStart, IdEnd - IdStart).TrimStartAndEnd();
						if (!NodeId.IsEmpty())
						{
							FParsedNode Node;
							Node.Id = NodeId;
							Node.Text = Content;
							
							if (Pair.NodeType == TEXT("EntryOrStart"))
							{
								if (NodeId.Equals(TEXT("Start"), ESearchCase::IgnoreCase) || Content.Equals(TEXT("Start"), ESearchCase::IgnoreCase))
								{
									Node.Type = TEXT("Start");
								}
								else
								{
									Node.Type = TEXT("Entry");
								}
							}
							else
							{
								Node.Type = Pair.NodeType;
							}

							// Speaker parsing
							int32 ColonIdx = Content.Find(TEXT(":"));
							if (ColonIdx != INDEX_NONE && Node.Type == TEXT("Dialogue"))
							{
								Node.SpeakerId = Content.Left(ColonIdx).TrimStartAndEnd();
								Node.Text = Content.Mid(ColonIdx + 1).TrimStartAndEnd();
							}
							else
							{
								Node.SpeakerId = TEXT("");
								Node.Text = Content;
							}

							ParsedNodes.Add(NodeId, Node);
							Line = Line.Left(IdStart) + NodeId + Line.Mid(EIdx + Pair.CloseDelim.Len());
							bFoundShape = true;
							break;
						}
					}
				}
			}
		}

		// Parse connections
		FString FromNode, ToNode, ConnLabel;
		bool bIsConnection = false;

		if (Line.Contains(TEXT("-->|")))
		{
			int32 ArrowIdx = Line.Find(TEXT("-->|"));
			FromNode = Line.Left(ArrowIdx).TrimStartAndEnd();
			FString Right = Line.Mid(ArrowIdx + 4);
			int32 BarIdx = Right.Find(TEXT("|"));
			if (BarIdx != INDEX_NONE)
			{
				ConnLabel = Right.Left(BarIdx).TrimStartAndEnd();
				ToNode = Right.Mid(BarIdx + 1).TrimStartAndEnd();
				bIsConnection = true;
			}
		}
		else
		{
			int32 FirstDash = Line.Find(TEXT("--"));
			int32 LastArrow = Line.Find(TEXT("-->"));
			if (FirstDash != INDEX_NONE && LastArrow != INDEX_NONE && LastArrow > FirstDash + 2)
			{
				FromNode = Line.Left(FirstDash).TrimStartAndEnd();
				ConnLabel = Line.Mid(FirstDash + 2, LastArrow - FirstDash - 2).TrimStartAndEnd();
				ToNode = Line.Mid(LastArrow + 3).TrimStartAndEnd();
				bIsConnection = true;
			}
			else if (LastArrow != INDEX_NONE)
			{
				FromNode = Line.Left(LastArrow).TrimStartAndEnd();
				ToNode = Line.Mid(LastArrow + 3).TrimStartAndEnd();
				bIsConnection = true;
			}
		}

		if (bIsConnection)
		{
			if (ConnLabel.StartsWith(TEXT("\"")) && ConnLabel.EndsWith(TEXT("\"")))
			{
				ConnLabel = ConnLabel.Mid(1, ConnLabel.Len() - 2);
			}
			FParsedConnection Conn;
			Conn.FromId = FromNode;
			Conn.ToId = ToNode;
			Conn.Label = ConnLabel;
			Connections.Add(Conn);

			// Add nodes if they were not declared explicitly but referenced in connections
			if (!ParsedNodes.Contains(FromNode))
			{
				FParsedNode N;
				N.Id = FromNode;
				N.Type = FromNode.Equals(TEXT("Start"), ESearchCase::IgnoreCase) ? TEXT("Start") : (FromNode.Contains(TEXT("End")) ? TEXT("End") : (FromNode.Contains(TEXT("Start")) ? TEXT("Entry") : TEXT("Dialogue")));
				N.Text = FromNode;
				ParsedNodes.Add(FromNode, N);
			}
			if (!ParsedNodes.Contains(ToNode))
			{
				FParsedNode N;
				N.Id = ToNode;
				N.Type = ToNode.Equals(TEXT("Start"), ESearchCase::IgnoreCase) ? TEXT("Start") : (ToNode.Contains(TEXT("End")) ? TEXT("End") : (ToNode.Contains(TEXT("Start")) ? TEXT("Entry") : TEXT("Dialogue")));
				N.Text = ToNode;
				ParsedNodes.Add(ToNode, N);
			}
		}
	}

	// Rebuild graph nodes
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueGraph", "ImportMermaid", "Import Dialogue from Mermaid"));
	DialogueGraph->Modify();

	// Clear current nodes
	TArray<UEdGraphNode*> NodesToDelete = DialogueGraph->Nodes;
	for (UEdGraphNode* Node : NodesToDelete)
	{
		Node->Modify();
		Node->DestroyNode();
	}
	DialogueGraph->Nodes.Empty();

	TMap<FString, UEdGraphNode*> SpawnedNodes;

	// Group by independent branches to avoid node overlapping
	TArray<FString> Roots;
	for (auto& Elem : ParsedNodes)
	{
		if (Elem.Value.Type == TEXT("Start") || Elem.Value.Type == TEXT("Entry"))
		{
			Roots.Add(Elem.Key);
		}
	}
	if (Roots.Num() == 0 && ParsedNodes.Num() > 0)
	{
		Roots.Add(ParsedNodes.CreateIterator().Key());
	}

	TSet<FString> VisitedGlobal;
	TMap<FString, FVector2D> NodePositions;
	float CurrentYCenter = 0.0f;

	for (const FString& RootId : Roots)
	{
		if (VisitedGlobal.Contains(RootId)) continue;

		TMap<FString, int32> BranchDepths;
		BranchDepths.Add(RootId, 0);

		TArray<FString> LocalQueue;
		LocalQueue.Add(RootId);

		int32 LocalQIdx = 0;
		while (LocalQIdx < LocalQueue.Num())
		{
			FString Curr = LocalQueue[LocalQIdx++];
			int32 CurrDepth = BranchDepths[Curr];

			for (const FParsedConnection& Conn : Connections)
			{
				if (Conn.FromId == Curr)
				{
					FString ToId = Conn.ToId;
					if (!VisitedGlobal.Contains(ToId) && !BranchDepths.Contains(ToId))
					{
						BranchDepths.Add(ToId, CurrDepth + 1);
						LocalQueue.Add(ToId);
					}
				}
			}
		}

		for (auto& Elem : BranchDepths)
		{
			VisitedGlobal.Add(Elem.Key);
		}

		TMap<int32, TArray<FString>> BranchByDepth;
		for (auto& Elem : BranchDepths)
		{
			BranchByDepth.FindOrAdd(Elem.Value).Add(Elem.Key);
		}

		float MaxBranchY = CurrentYCenter;
		float MinBranchY = CurrentYCenter;

		for (auto& DepthElem : BranchByDepth)
		{
			int32 Depth = DepthElem.Key;
			const TArray<FString>& DepthNodes = DepthElem.Value;
			int32 Total = DepthNodes.Num();

			for (int32 i = 0; i < Total; ++i)
			{
				float CenteredY = CurrentYCenter + (i - (Total - 1) / 2.0f) * 450.0f;
				NodePositions.Add(DepthNodes[i], FVector2D(Depth * 800.0f, CenteredY));

				if (CenteredY > MaxBranchY) MaxBranchY = CenteredY;
				if (CenteredY < MinBranchY) MinBranchY = CenteredY;
			}
		}

		CurrentYCenter = MaxBranchY + 600.0f;
	}

	// Layout leftovers
	for (auto& Elem : ParsedNodes)
	{
		if (!NodePositions.Contains(Elem.Key))
		{
			NodePositions.Add(Elem.Key, FVector2D(0.f, CurrentYCenter));
			CurrentYCenter += 450.f;
		}
	}

	// Instantiate nodes
	for (auto& Elem : ParsedNodes)
	{
		const FString& Id = Elem.Key;
		const FParsedNode& PNode = Elem.Value;
		FVector2D Pos = NodePositions[Id];

		UEdGraphNode* NewNode = nullptr;

		if (PNode.Type == TEXT("Start"))
		{
			NewNode = NewObject<UDialogueStartGizmo>(DialogueGraph, UDialogueStartGizmo::StaticClass(), NAME_None, RF_Transactional);
		}
		else if (PNode.Type == TEXT("End"))
		{
			UDialogueEndGizmo* EndNode = NewObject<UDialogueEndGizmo>(DialogueGraph, UDialogueEndGizmo::StaticClass(), NAME_None, RF_Transactional);
			EndNode->EndNodeId = FName(*PNode.Text);
			NewNode = EndNode;
		}
		else if (PNode.Type == TEXT("Entry"))
		{
			UDialogueEntryGizmo* EntryNode = NewObject<UDialogueEntryGizmo>(DialogueGraph, UDialogueEntryGizmo::StaticClass(), NAME_None, RF_Transactional);
			EntryNode->EntryPointId = FName(*PNode.Text);
			NewNode = EntryNode;
		}
		else
		{
			UDialogueGraphNode* DialogueNode = NewObject<UDialogueGraphNode>(DialogueGraph, UDialogueGraphNode::StaticClass(), NAME_None, RF_Transactional);
			DialogueNode->NodeId = FName(*Id);
			DialogueNode->NodeData.SpeakerId = FName(*PNode.SpeakerId);
			DialogueNode->NodeData.Text = FText::FromString(PNode.Text);
			DialogueNode->NodeData.Outputs.Empty();
			NewNode = DialogueNode;
		}

		if (NewNode)
		{
			NewNode->Modify();
			NewNode->NodePosX = Pos.X;
			NewNode->NodePosY = Pos.Y;
			DialogueGraph->AddNode(NewNode, true, true);
			NewNode->AllocateDefaultPins();
			SpawnedNodes.Add(Id, NewNode);
		}
	}

	// Build connections
	for (const FParsedConnection& Conn : Connections)
	{
		UEdGraphNode* FromGraphNode = SpawnedNodes.FindRef(Conn.FromId);
		UEdGraphNode* ToGraphNode = SpawnedNodes.FindRef(Conn.ToId);

		if (!FromGraphNode || !ToGraphNode) continue;

		UEdGraphPin* OutPin = nullptr;
		UEdGraphPin* InPin = nullptr;

		// Find Input pin on target node
		for (UEdGraphPin* Pin : ToGraphNode->Pins)
		{
			if (Pin->Direction == EGPD_Input)
			{
				InPin = Pin;
				break;
			}
		}

		// Handle source pin selection based on node type
		if (UDialogueGraphNode* DFrom = Cast<UDialogueGraphNode>(FromGraphNode))
		{
			// If it's a dialogue node, find or create the output pin matching choice label
			// Sync with NodeData.Outputs
			int32 OutIdx = INDEX_NONE;
			for (int32 i = 0; i < DFrom->NodeData.Outputs.Num(); ++i)
			{
				if (DFrom->NodeData.Outputs[i].Text.ToString().Equals(Conn.Label, ESearchCase::IgnoreCase))
				{
					OutIdx = i;
					break;
				}
			}

			if (OutIdx == INDEX_NONE)
			{
				FDialogueOutput NewOut;
				NewOut.Text = FText::FromString(Conn.Label.IsEmpty() ? TEXT("Next") : Conn.Label);
				NewOut.bEnabled = true;
				OutIdx = DFrom->NodeData.Outputs.Add(NewOut);
				DFrom->ReconstructNode();
			}

			FName TargetPinName(*FString::Printf(TEXT("Out_%d"), OutIdx));
			for (UEdGraphPin* Pin : DFrom->Pins)
			{
				if (Pin->Direction == EGPD_Output && Pin->PinName == TargetPinName)
				{
					OutPin = Pin;
					break;
				}
			}
		}
		else
		{
			// For Start and Entry nodes, just get their first Output pin
			for (UEdGraphPin* Pin : FromGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					OutPin = Pin;
					break;
				}
			}
		}

		if (OutPin && InPin)
		{
			OutPin->MakeLinkTo(InPin);
		}
	}

	// Resolve NextStart auto-connections for End nodes to Entry nodes
	for (UEdGraphNode* Node : DialogueGraph->Nodes)
	{
		if (UDialogueEndGizmo* EndNode = Cast<UDialogueEndGizmo>(Node))
		{
			// Try to find a matching entry point
			UDialogueEntryGizmo* MatchingEntry = nullptr;
			for (UEdGraphNode* OtherNode : DialogueGraph->Nodes)
			{
				if (UDialogueEntryGizmo* EntryNode = Cast<UDialogueEntryGizmo>(OtherNode))
				{
					if (EntryNode->EntryPointId == EndNode->EndNodeId)
					{
						MatchingEntry = EntryNode;
						break;
					}
				}
			}

			if (MatchingEntry)
			{
				UEdGraphPin* EndNextStartPin = nullptr;
				for (UEdGraphPin* Pin : EndNode->Pins)
				{
					if (Pin->Direction == EGPD_Output && Pin->PinName == TEXT("NextStart"))
					{
						EndNextStartPin = Pin;
						break;
					}
				}

				UEdGraphPin* EntryNextStartPin = nullptr;
				for (UEdGraphPin* Pin : MatchingEntry->Pins)
				{
					if (Pin->Direction == EGPD_Input && Pin->PinName == TEXT("NextStart"))
					{
						EntryNextStartPin = Pin;
						break;
					}
				}

				if (EndNextStartPin && EntryNextStartPin)
				{
					EndNextStartPin->MakeLinkTo(EntryNextStartPin);
				}
			}
		}
	}

	// Sync outputs' NextNodeId properties
	for (UEdGraphNode* Node : DialogueGraph->Nodes)
	{
		if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
		{
			for (UEdGraphPin* Pin : DNode->Pins)
			{
				if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
				{
					FString PinNameStr = Pin->PinName.ToString();
					if (PinNameStr.StartsWith(TEXT("Out_")))
					{
						int32 OutIdx = FCString::Atoi(*PinNameStr.Mid(4));
						if (DNode->NodeData.Outputs.IsValidIndex(OutIdx))
						{
							UEdGraphNode* TargetNode = Pin->LinkedTo[0]->GetOwningNode();
							if (UDialogueGraphNode* TargetDNode = Cast<UDialogueGraphNode>(TargetNode))
							{
								DNode->NodeData.Outputs[OutIdx].NextNodeId = TargetDNode->NodeId;
							}
							else
							{
								DNode->NodeData.Outputs[OutIdx].NextNodeId = NAME_None;
							}
						}
					}
				}
			}
		}
	}

	// Apply colors and positions from parsed metadata
	for (UEdGraphNode* Node : DialogueGraph->Nodes)
	{
		FString Key;
		if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
			Key = DNode->NodeId.ToString();
		else if (UDialogueStartGizmo* S = Cast<UDialogueStartGizmo>(Node))
			Key = TEXT("Start");
		else if (UDialogueEndGizmo* E = Cast<UDialogueEndGizmo>(Node))
			Key = FString(TEXT("End_")) + (E->EndNodeId.IsNone() ? TEXT("End") : E->EndNodeId.ToString());
		else if (UDialogueEntryGizmo* En = Cast<UDialogueEntryGizmo>(Node))
			Key = En->EntryPointId.IsNone() ? TEXT("Entry") : En->EntryPointId.ToString();

		if (!Key.IsEmpty())
		{
			if (const FParsedNode* PN = ParsedNodes.Find(Key))
			{
				if (PN->Color.A >= 0.f)
				{
					if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
						DNode->NodeColor = PN->Color;
				}
			}
			if (const FVector2D* Pos = ParsedPositions.Find(Key))
			{
				Node->NodePosX = (int32)Pos->X;
				Node->NodePosY = (int32)Pos->Y;
			}
		}
	}

	// Run validation start/end checks
	EnsureStartGizmo(DialogueGraph);
	EnsureEndGizmo(DialogueGraph);

	DialogueGraph->NotifyGraphChanged();
}

void FDialogueAssetEditorToolkit::ExportMermaid()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return;

	TArray<FString> SaveFilenames;
	const FString DefaultPath = FPaths::ProjectContentDir();
	const FString FileTypes = TEXT("Markdown Files (*.md)|*.md|All Files (*.*)|*.*");

	bool bSaved = DesktopPlatform->SaveFileDialog(
		nullptr,
		TEXT("Export Dialogue to Mermaid Markdown"),
		DefaultPath,
		TEXT("dialogue.md"),
		FileTypes,
		EFileDialogFlags::None,
		SaveFilenames
	);

	if (!bSaved || SaveFilenames.Num() == 0) return;

	FString Mermaid;
	Mermaid += TEXT("```mermaid\n");
	Mermaid += TEXT("flowchart TD\n");

	// Export Nodes
	for (UEdGraphNode* Node : DialogueGraph->Nodes)
	{
		if (UDialogueStartGizmo* StartNode = Cast<UDialogueStartGizmo>(Node))
		{
			Mermaid += TEXT("    Start([Start])\n");
		}
		else if (UDialogueEndGizmo* EndNode = Cast<UDialogueEndGizmo>(Node))
		{
			FString Name = EndNode->EndNodeId.IsNone() ? TEXT("End") : EndNode->EndNodeId.ToString();
			Mermaid += FString::Printf(TEXT("    End_%s[[%s]]\n"), *Name, *Name);
		}
		else if (UDialogueEntryGizmo* EntryNode = Cast<UDialogueEntryGizmo>(Node))
		{
			FString Name = EntryNode->EntryPointId.IsNone() ? TEXT("Entry") : EntryNode->EntryPointId.ToString();
			Mermaid += FString::Printf(TEXT("    %s([%s])\n"), *Name, *Name);
		}
		else if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
		{
			FString Id = DNode->NodeId.ToString();
			FString Speaker = DNode->NodeData.SpeakerId.ToString();
			FString TextVal = DNode->NodeData.Text.ToString().Replace(TEXT("\""), TEXT("\\\""));

			if (Speaker.IsEmpty())
			{
				Mermaid += FString::Printf(TEXT("    %s[\"%s\"]\n"), *Id, *TextVal);
			}
			else
			{
				Mermaid += FString::Printf(TEXT("    %s[\"%s: %s\"]\n"), *Id, *Speaker, *TextVal);
			}
		}
	}

	Mermaid += TEXT("\n");

	// Color styles — standard Mermaid syntax, renders in any viewer
	const FLinearColor DefaultColor(0.05f, 0.05f, 0.05f, 0.8f);
	for (UEdGraphNode* Node : DialogueGraph->Nodes)
	{
		if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
		{
			if (!DNode->NodeColor.Equals(DefaultColor, 0.01f))
			{
				FString Id = DNode->NodeId.ToString();
				uint8 R = (uint8)(DNode->NodeColor.R * 255.f);
				uint8 G = (uint8)(DNode->NodeColor.G * 255.f);
				uint8 B = (uint8)(DNode->NodeColor.B * 255.f);
				Mermaid += FString::Printf(TEXT("    style %s fill:#%02x%02x%02x\n"), *Id, R, G, B);
			}
		}
	}
	Mermaid += TEXT("\n");

	// Export Connections
	for (UEdGraphNode* Node : DialogueGraph->Nodes)
	{
		if (UDialogueStartGizmo* StartNode = Cast<UDialogueStartGizmo>(Node))
		{
			for (UEdGraphPin* Pin : StartNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					for (UEdGraphPin* Linked : Pin->LinkedTo)
					{
						if (Linked)
						{
							UEdGraphNode* TargetNode = Linked->GetOwningNode();
							if (UDialogueGraphNode* TargetDNode = Cast<UDialogueGraphNode>(TargetNode))
							{
								Mermaid += FString::Printf(TEXT("    Start --> %s\n"), *TargetDNode->NodeId.ToString());
							}
						}
					}
				}
			}
		}
		else if (UDialogueEntryGizmo* EntryNode = Cast<UDialogueEntryGizmo>(Node))
		{
			FString Name = EntryNode->EntryPointId.IsNone() ? TEXT("Entry") : EntryNode->EntryPointId.ToString();
			for (UEdGraphPin* Pin : EntryNode->Pins)
			{
				if (Pin->Direction == EGPD_Output && Pin->PinName == TEXT("Out"))
				{
					for (UEdGraphPin* Linked : Pin->LinkedTo)
					{
						if (Linked)
						{
							UEdGraphNode* TargetNode = Linked->GetOwningNode();
							if (UDialogueGraphNode* TargetDNode = Cast<UDialogueGraphNode>(TargetNode))
							{
								Mermaid += FString::Printf(TEXT("    %s --> %s\n"), *Name, *TargetDNode->NodeId.ToString());
							}
						}
					}
				}
			}
		}
		else if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
		{
			FString Id = DNode->NodeId.ToString();
			for (UEdGraphPin* Pin : DNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					FString PinNameStr = Pin->PinName.ToString();
					if (PinNameStr.StartsWith(TEXT("Out_")))
					{
						int32 OutIdx = FCString::Atoi(*PinNameStr.Mid(4));
						if (DNode->NodeData.Outputs.IsValidIndex(OutIdx))
						{
							FString Label = DNode->NodeData.Outputs[OutIdx].Text.ToString();
							for (UEdGraphPin* Linked : Pin->LinkedTo)
							{
								if (Linked)
								{
									UEdGraphNode* TargetNode = Linked->GetOwningNode();
									if (UDialogueGraphNode* TargetDNode = Cast<UDialogueGraphNode>(TargetNode))
									{
										if (Label.IsEmpty() || Label.Equals(TEXT("Next"), ESearchCase::IgnoreCase))
										{
											Mermaid += FString::Printf(TEXT("    %s --> %s\n"), *Id, *TargetDNode->NodeId.ToString());
										}
										else
										{
											Mermaid += FString::Printf(TEXT("    %s -->|\"%s\"| %s\n"), *Id, *Label, *TargetDNode->NodeId.ToString());
										}
									}
									else if (UDialogueEndGizmo* TargetEndNode = Cast<UDialogueEndGizmo>(TargetNode))
									{
										FString EndName = TargetEndNode->EndNodeId.IsNone() ? TEXT("End") : TargetEndNode->EndNodeId.ToString();
										if (Label.IsEmpty() || Label.Equals(TEXT("Next"), ESearchCase::IgnoreCase))
										{
											Mermaid += FString::Printf(TEXT("    %s --> End_%s\n"), *Id, *EndName);
										}
										else
										{
											Mermaid += FString::Printf(TEXT("    %s -->|\"%s\"| End_%s\n"), *Id, *Label, *EndName);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else if (UDialogueEndGizmo* EndNode = Cast<UDialogueEndGizmo>(Node))
		{
			FString Name = EndNode->EndNodeId.IsNone() ? TEXT("End") : EndNode->EndNodeId.ToString();
			for (UEdGraphPin* Pin : EndNode->Pins)
			{
				if (Pin->Direction == EGPD_Output && Pin->PinName == TEXT("NextStart"))
				{
					for (UEdGraphPin* Linked : Pin->LinkedTo)
					{
						if (Linked)
						{
							UEdGraphNode* TargetNode = Linked->GetOwningNode();
							if (UDialogueEntryGizmo* TargetEntryNode = Cast<UDialogueEntryGizmo>(TargetNode))
							{
								FString EntryName = TargetEntryNode->EntryPointId.IsNone() ? TEXT("Entry") : TargetEntryNode->EntryPointId.ToString();
								Mermaid += FString::Printf(TEXT("    End_%s --> %s\n"), *Name, *EntryName);
							}
						}
					}
				}
			}
		}
	}

	Mermaid += TEXT("```\n");

	// Node positions — stored as %% comments outside the mermaid block so the diagram still renders
	FString PosMeta;
	for (UEdGraphNode* Node : DialogueGraph->Nodes)
	{
		FString Key;
		if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
			Key = DNode->NodeId.ToString();
		else if (Cast<UDialogueStartGizmo>(Node))
			Key = TEXT("Start");
		else if (UDialogueEndGizmo* E = Cast<UDialogueEndGizmo>(Node))
			Key = FString(TEXT("End_")) + (E->EndNodeId.IsNone() ? TEXT("End") : E->EndNodeId.ToString());
		else if (UDialogueEntryGizmo* En = Cast<UDialogueEntryGizmo>(Node))
			Key = En->EntryPointId.IsNone() ? TEXT("Entry") : En->EntryPointId.ToString();

		if (!Key.IsEmpty())
			PosMeta += FString::Printf(TEXT("%% pos:%s:%d:%d\n"), *Key, Node->NodePosX, Node->NodePosY);
	}

	FString FinalContent = FString::Printf(
		TEXT("# Dialogue Flowchart\n\nGenerated from Dialogue Asset Editor.\n\n%s\n%s"),
		*Mermaid, *PosMeta);
	FFileHelper::SaveStringToFile(FinalContent, *SaveFilenames[0], FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

#undef LOCTEXT_NAMESPACE

#endif
