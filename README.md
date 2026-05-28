# Dialogue System Plugin for Unreal Engine

A standalone, lightweight, node-based **Dialogue System Plugin** developed for **Unreal Engine 5.7**. This plugin provides a highly extensible editor toolkit for visually authoring branching dialogues and a runtime module to execute them seamlessly in gameplay.

---

## 🚀 Features

* **Node-Based Editor**: A fully integrated, custom visual graph editor inside the Unreal Engine Editor.
* **Branching Conversations**: Easily author complex dialogue trees with conditional pathways and choices.
* **Decoupled Architecture**: Strictly separated runtime (`DialogueSystem`) and editor-only (`DialogueSystemEditor`) modules to ensure optimal runtime performance and clean builds.
* **Event Actions Integration**: Trigger custom C++ or Blueprint-based actions (`UDialogueAction`) at any point in the dialogue flow (e.g., giving items, opening doors, triggering quests).
* **Blueprint Support**: Complete API access for triggering, rendering, and processing dialogue choices in Blueprints.

---

## 🛠️ Module Structure

### 1. DialogueSystem (Runtime)
The core runtime library containing data assets, components, and sessions used by your game.
* **`UDialogueAsset`**: The primary data asset containing dialogue nodes, entry points, and compiled flow data.
* **`UDialogueComponent`**: Actor component that manages active conversations, current state, and visual presentation triggers.
* **`UDialogueSession`**: Handles logic execution, choice evaluation, and action execution for a dialogue instance.
* **`UDialogueAction`**: Extensible class for executing gameplay events from dialogue nodes.

### 2. DialogueSystemEditor (Editor-Only)
The editor-only module that powers the visual editing tools.
* **Asset Actions**: Registers custom factory and asset types for `.uasset` creation.
* **Graph & Schema**: Integrates with Unreal's GraphEditor using a custom schema (`UDialogueGraphSchema`) to validate connections.
* **Gizmos & Widgets**: Custom visual representation of nodes, start/entry points, and connection pins.
* **On-Save Compiler**: Binds to the asset save pipeline to compile the visual node tree down to a lightweight runtime map structure.

---

## 📦 Installation & Setup

1. Copy the `DialogueSystem` folder into your project's `Plugins/` directory:
   ```
   MyProject/
   ├── Plugins/
   │   └── DialogueSystem/   <-- Place this repository here
   ```
2. Re-generate project files and compile your project.
3. Enable the **DialogueSystem** plugin in the Unreal Editor (`Edit -> Plugins`).

---

## 💡 How to Use

### 1. Creating a Dialogue Asset
* Right-click in the **Content Browser** -> **Miscellaneous** (or **Data Asset**) -> Choose **Dialogue Asset**.
* Double-click the asset to open the custom Dialogue Graph Editor.
* Right-click in the graph to add **Start**, **Dialogue Nodes**, and **End Nodes**.
* Drag lines to link output pins to dialogue nodes to form conversation paths.

### 2. Running Dialogue in Gameplay
* Add a `UDialogueComponent` to your Player Character or Player Controller.
* Bind UI widgets to the component's event delegates:
  * `OnDialogueStarted`
  * `OnDialogueNodeReached`
  * `OnDialogueEnded`
* Call `StartDialogueSession` on the component, passing in the `UDialogueAsset`.
