// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class DH1_Client : ModuleRules
{
	public DH1_Client(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange([
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
			"JsonUtilities",
			"ImageDownload"
		]);

		PrivateDependencyModuleNames.AddRange([]);

		PublicIncludePaths.AddRange([
			"DH1_Client",
			"DH1_Client/Variant_Strategy",
			"DH1_Client/Variant_Strategy/UI",
			"DH1_Client/Variant_TwinStick",
			"DH1_Client/Variant_TwinStick/AI",
			"DH1_Client/Variant_TwinStick/Gameplay",
			"DH1_Client/Variant_TwinStick/UI"
		]);

        var SharedPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Shared"));
        var VcpkgPath = Path.Combine(SharedPath, "vcpkg/vcpkg_installed/x64-windows-static-md");

        PublicIncludePaths.Add(Path.Combine(VcpkgPath, "include"));
        PublicIncludePaths.Add(Path.Combine(VcpkgPath, "include/google/protobuf"));
        PublicIncludePaths.Add(Path.Combine(SharedPath, "Protocol"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Network/Protocol"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../../../DH1_Engine/CppNetEngine"));

        string VcpkgLibPath;
        string ProtobufLibPath;
        string CppNetEngineLibPath;
        if (Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
	        VcpkgLibPath = Path.Combine(VcpkgPath, "debug", "lib");
            ProtobufLibPath = Path.Combine(VcpkgLibPath, "libprotobufd.lib");
	        CppNetEngineLibPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Shared/Libraries/CppNetEngine/Debug/CppNetEngine.lib"));
        }
        else
        {
	        VcpkgLibPath = Path.Combine(VcpkgPath, "lib");
            ProtobufLibPath = Path.Combine(VcpkgLibPath, "libprotobuf.lib");
	        CppNetEngineLibPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Shared/Libraries/CppNetEngine/Release/CppNetEngine.lib"));
        }

        PublicAdditionalLibraries.Add(Path.Combine(VcpkgLibPath, "mimalloc.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(VcpkgLibPath, "fmt.lib"));
        PublicAdditionalLibraries.Add(ProtobufLibPath);
        PublicAdditionalLibraries.Add(Path.Combine(VcpkgLibPath, "utf8_range.lib"));
        PublicAdditionalLibraries.Add(CppNetEngineLibPath);


        // [추가] Crashpad 라이브러리 링크 (vcpkg에 있다면 VcpkgLibPath 경로 사용)
        PublicAdditionalLibraries.Add(Path.Combine(VcpkgLibPath, "vcpkg_crashpad_client.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(VcpkgLibPath, "vcpkg_crashpad_util.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(VcpkgLibPath, "vcpkg_crashpad_base.lib"));

        // DH1_Client.Build.cs 내부
        PublicAdditionalLibraries.Add(ProtobufLibPath); // 기존 코드

        var AbslLibDir = Path.Combine(VcpkgPath, "lib");
        if (Directory.Exists(AbslLibDir))
        {
	        var AbslLibs = Directory.GetFiles(AbslLibDir, "absl_*.lib");
	        foreach (var Lib in AbslLibs)
	        {
		        PublicAdditionalLibraries.Add(Lib);
	        }
        }

        bEnableExceptions = true;
        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off; // (최신 5.6+ 문법 적용)

        // Protobuf 관련 정의되지 않은 매크로 에러를 방지하기 위해 0으로 명시적 정의
        PublicDefinitions.Add("PROTOBUF_ENABLE_DEBUG_LOGGING_MAY_LEAK_PII=0");

        PublicDefinitions.Add("_UNREAL_=1");
    }
}
