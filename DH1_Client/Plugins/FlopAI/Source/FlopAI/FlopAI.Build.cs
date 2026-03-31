// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FlopAI : ModuleRules
{
    public FlopAI(ReadOnlyTargetRules Target) : base(Target)
    {
        // Distribution builds ship without Private/ source -- use precompiled binaries
        if (!System.IO.Directory.Exists(System.IO.Path.Combine(ModuleDirectory, "Private")))
        {
            bUsePrecompiled = true;
            return;
        }

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        
        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "SlateCore",
                "InputCore",
                "ApplicationCore",
                "Slate"
				// ... add other public dependencies that you statically link with here ...
			}
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "EditorSubsystem",
                "WebSockets",
                "SSL",
                "Json",
                "JsonUtilities",
                "UnrealEd",
                "LevelEditor",
                "PythonScriptPlugin",
                "HTTP",
                "DeveloperSettings",
                "Projects",
                "WebBrowser",
                "WebBrowserWidget",
                "ToolMenus",
                "GraphEditor",
                "Kismet",
                "KismetCompiler",
                "PropertyEditor",
                "BlueprintGraph",
                "AssetRegistry",
                "ContentBrowserData",
                "DesktopPlatform",
                "SQLiteCore",
                "AnimGraph",
                "EnhancedInput",
                "InputBlueprintNodes",
                "ImageCore",
                "EditorFramework",
                "MessageLog",
                "ClassViewer",
                "InterchangeEngine",
                "UMG",
                "UMGEditor",
                "AIModule",
                "GameplayTasks",
                "NavigationSystem",
                "AIGraph",
                "BehaviorTreeEditor",
                "AnimationBlueprintLibrary",
                "AnimationModifiers",
                "Persona",
                "AssetTools",
                "EditorScriptingUtilities",
                "AnimationBlueprintEditor",
                "AnimGraphRuntime",
                "MovieScene",
                "MovieSceneTracks",
                "EnvironmentQueryEditor",
                "GameplayTags",
                "GameplayTagsEditor",
                "MaterialEditor"
			// ... add private dependencies that you statically link with here ...
		}
        );

        // === Optional plugin dependencies (conditional compilation) ===
        // These only compile when the corresponding plugin is enabled in the project.
        // Each gets a preprocessor define so C++ code can #if guard the implementations.

        SetupOptionalModule(Target, "SmartObjectsModule", "WITH_SMART_OBJECTS");
        SetupOptionalModule(Target, "StateTreeModule", "WITH_STATE_TREE");
        SetupOptionalModule(Target, "StateTreeEditorModule", "WITH_STATE_TREE_EDITOR");
        SetupOptionalModule(Target, "GameplayAbilities", "WITH_GAMEPLAY_ABILITIES");
        SetupOptionalModule(Target, "CommonUI", "WITH_COMMON_UI");

        AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");

        PrivateIncludePathModuleNames.AddRange(
            new string[]
            {
                "UnrealEd"
            }
        );

        PublicDefinitions.AddRange(
            new string[]
            {
                "BPTE_WITH_EDITOR_ONLY=1"
            }
        );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }

    private void SetupOptionalModule(ReadOnlyTargetRules Target, string ModuleName, string DefineName)
    {
        bool bModuleAvailable = false;
        try
        {
            string BuildFileName = ModuleName + ".Build.cs";
            string EngineDir = System.IO.Path.GetFullPath(System.IO.Path.Combine(EngineDirectory, ".."));
            string ProjectDir = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "..", "..", "..", ".."));

            string[] SearchRoots = new string[]
            {
                System.IO.Path.Combine(EngineDir, "Plugins"),
                System.IO.Path.Combine(ProjectDir, "Plugins"),
                System.IO.Path.Combine(ProjectDir, "Source"),
                System.IO.Path.Combine(EngineDir, "Source"),
            };

            foreach (string Root in SearchRoots)
            {
                if (System.IO.Directory.Exists(Root))
                {
                    string[] Found = System.IO.Directory.GetFiles(Root, BuildFileName, System.IO.SearchOption.AllDirectories);
                    if (Found.Length > 0)
                    {
                        bModuleAvailable = true;
                        break;
                    }
                }
            }
        }
        catch
        {
            bModuleAvailable = false;
        }

        if (bModuleAvailable)
        {
            PrivateDependencyModuleNames.Add(ModuleName);
            PublicDefinitions.Add(DefineName + "=1");
        }
        else
        {
            PublicDefinitions.Add(DefineName + "=0");
        }
    }
}

