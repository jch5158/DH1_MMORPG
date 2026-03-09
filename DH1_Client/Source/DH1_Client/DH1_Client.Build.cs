// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class DH1_Client : ModuleRules
{
	public DH1_Client(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"DH1_Client",
			"DH1_Client/Variant_Strategy",
			"DH1_Client/Variant_Strategy/UI",
			"DH1_Client/Variant_TwinStick",
			"DH1_Client/Variant_TwinStick/AI",
			"DH1_Client/Variant_TwinStick/Gameplay",
			"DH1_Client/Variant_TwinStick/UI"
		});

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true


        string SharedPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Shared"));
        string VcpkgPath = Path.Combine(SharedPath, "vcpkg_installed/x64-windows-static-md");

        PublicIncludePaths.Add(Path.Combine(VcpkgPath, "include"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Network/Protocol"));

        PublicAdditionalLibraries.Add(Path.Combine(VcpkgPath, "lib/libprotobuf.lib"));

        bEnableExceptions = true;
        bEnableUndefinedIdentifierWarnings = false;
    }
}
