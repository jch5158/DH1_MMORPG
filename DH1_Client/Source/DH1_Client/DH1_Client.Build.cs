// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class DH1_Client : ModuleRules
{
	public DH1_Client(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
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
			"Slate",
			"HTTP",
			"Json",
			"JsonUtilities"
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
        string VcpkgPath = Path.Combine(SharedPath, "vcpkg/vcpkg_installed/x64-windows-static-md");

        PublicIncludePaths.Add(Path.Combine(VcpkgPath, "include"));
        PublicIncludePaths.Add(Path.Combine(VcpkgPath, "include/google/protobuf"));
        PublicIncludePaths.Add(Path.Combine(SharedPath, "Protocol"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../../DH1_Engine/CppNetEngine"));

        string ProtobufLibPath = string.Empty;
        string CppNetEngineLibPath = string.Empty;
        if (Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
	        ProtobufLibPath = Path.Combine(VcpkgPath, "debug", "lib", "libprotobufd.lib");
	        CppNetEngineLibPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Shared/Libraries/CppNetEngine/Debug/CppNetEngine.lib"));
        }
        else
        {
	        ProtobufLibPath = Path.Combine(VcpkgPath, "lib", "libprotobuf.lib");
	        CppNetEngineLibPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Shared/Libraries/CppNetEngine/Release/CppNetEngine.lib"));
        }

        PublicAdditionalLibraries.Add(Path.Combine(VcpkgPath, "lib", "mimalloc.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(VcpkgPath, "lib", "fmt.lib"));
        PublicAdditionalLibraries.Add(ProtobufLibPath);
        PublicAdditionalLibraries.Add(CppNetEngineLibPath);

        bEnableExceptions = true;
        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off; // (최신 5.6+ 문법 적용)

        // Protobuf 관련 정의되지 않은 매크로 에러를 방지하기 위해 0으로 명시적 정의
        PublicDefinitions.Add("PROTOBUF_ENABLE_DEBUG_LOGGING_MAY_LEAK_PII=0");

        PublicDefinitions.Add("_UNREAL_=1");
    }
}
