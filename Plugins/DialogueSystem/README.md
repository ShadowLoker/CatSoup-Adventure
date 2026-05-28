# Dialogue System Plugin for Unreal Engine 5.7

This plugin is a self-contained, lightweight, node-based **Dialogue System** for visual branching conversations. It includes C++ graph compilation tools, editor integration, Markdown/Mermaid round-trip parsing, and ready-to-use Blueprint UI templates.

---

## 📦 What's Included

1. **C++ Core Module (`DialogueSystem`):** Runtime state machine, actor component, and data structures.
2. **C++ Editor Module (`DialogueSystemEditor`):** Visual graph editor, custom visual node schemas, and a Markdown/Mermaid round-trip importer/exporter.
3. **Plugin Content (`DialogueSystem Content`):**
   * **`DialogueBox`**: A beautiful UMG User Widget that displays dialogue text, speaker names, and dynamically generated choice buttons.
   * **`UW_DialogueTest`**: A test HUD/controller widget.
   * **`BP_DialogueTest`**: A template NPC actor pre-configured with a `DialogueComponent`.

---

## ⚡ How to Start Dialogue (Drag-and-Drop Setup)

The plugin is designed to be 100% self-contained and require minimal setup:

### 1. Add Component to NPC
* Drag the `DialogueComponent` from the **Component Palette** onto any NPC actor.
* In the Details panel of the component, assign your authored **Dialogue Asset** in the `Dialogue Asset` slot.

### 2. Trigger the Dialogue
* When your player interacts with the NPC (e.g., via collision overlap or an interaction key), get a reference to the NPC's `DialogueComponent` and call:
  ```blueprint
  BeginDialogue(Interactor = PlayerCharacter, EntryPointOverride = NAME_None)
  ```
  *(Pass an optional custom start point ID to `EntryPointOverride` if you want the NPC to react differently based on prior quest states).*

---

## 🎨 How the UI is Wired Up (Under the Hood)

If you wish to use the built-in template widget (`DialogueBox`) or build your own from scratch, here is how the communication works between the state machine and the UI:

### The Flow:
1. **Creation:** When `BeginDialogue` is called on the C++ Component, it spawns an active **Dialogue Session** (`UDialogueSession`) and stores it in `ActiveSession`.
2. **UI Spawning:** The player's HUD or interaction handler spawns the `DialogueBox` widget and passes the `ActiveSession` reference to it.
3. **Binding to Delegates:** The UI widget binds to these core delegates on the `ActiveSession`:
   * **`OnLineStarted(Payload)`**: Fired when a line is reached. The `Payload` contains the `SpeakerId` (Speaker Name), `LineText` (Dialogue Line), and an array of `Choices` (Index & Text).
   * **`OnDialogueEnded`**: Fired when the conversation finishes. The UI widget uses this event to destroy itself and return player controls.
4. **Rendering Text & Choices:**
   * In the `OnLineStarted` event handler, set your UMG Text blocks to `Payload.LineText` and `Payload.SpeakerId`.
   * Clear any previous choice buttons.
   * Loop through the `Choices` array. For each choice, construct a button widget, set its text label, and store its `Index`.
5. **Advancing Dialogue:**
   * When a choice button is clicked, call `ActiveSession->Advance(ChoiceIndex)` on the session object.
   * The state machine will evaluate the next node and automatically trigger the next `OnLineStarted` event (or `OnDialogueEnded` if it reaches an exit).

---

## 🤖 AI Workflow Integration

A generic prompt is included in the plugin contents under `Content/prompt.md`.
* Copy this prompt and paste it into any LLM (ChatGPT, Claude, Gemini, etc.).
* Tell the AI what scenario you want (e.g., "A tavern keeper who refuses to sell drink until you find his key").
* The AI will output a clean Mermaid flowchart block.
* Save the flowchart to a `.md` file, click **File > Import Mermaid** in the Dialogue Editor, and your entire visual tree is instantly compiled and routed.
