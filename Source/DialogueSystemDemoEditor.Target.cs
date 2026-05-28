using UnrealBuildTool;
using System.Collections.Generic;

public class DialogueSystemDemoEditorTarget : TargetRules
{
    public DialogueSystemDemoEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        bOverrideBuildEnvironment = true;
        ExtraModuleNames.Add("DialogueSystemDemo");
    }
}
