// DialogueGraph.h
#pragma once
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "DialogueGraph.generated.h"

UCLASS()
class CATSOUP_ADVENTURE_API UDialogueGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	// Graph class for dialogue; schema will be assigned when the graph is created.
	// You can extend with helper functions if desired
};