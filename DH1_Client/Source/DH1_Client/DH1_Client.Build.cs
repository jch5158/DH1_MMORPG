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
	        "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "UMG", "Slate", "SlateCore", "HTTP", "Json", "JsonUtilities", "ImageDownload",
            "NavigationSystem", "Navmesh", "ImageWrapper"
        ]);

        PrivateDependencyModuleNames.AddRange([]);

        PublicIncludePaths.AddRange([
	        "DH1_Client",
            Path.Combine(ModuleDirectory, "Network/Protocol")
        ]);

        var SharedPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Shared"));
        var EnginePath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../DH1_Engine"));
        var VcpkgPath = Path.Combine(SharedPath, "vcpkg/vcpkg_installed/x64-windows-static-md");

        PublicIncludePaths.AddRange([
	        Path.Combine(VcpkgPath, "include"),
            Path.Combine(VcpkgPath, "include/google/protobuf"),
            Path.Combine(SharedPath, "Protocol"),
            Path.Combine(EnginePath, "CppNetEngine")
        ]);

        var bIsDebug = Target.Configuration is UnrealTargetConfiguration.Debug or UnrealTargetConfiguration.DebugGame;

        var VcpkgLibPath = Path.Combine(VcpkgPath, bIsDebug ? "debug/lib" : "lib");
        var ConfigName = bIsDebug ? "Debug" : "Release";
        var ProtobufLibName = bIsDebug ? "libprotobufd.lib" : "libprotobuf.lib";

        var HiredisLibName = bIsDebug ? "hiredisd.lib" : "hiredis.lib";

        PublicAdditionalLibraries.AddRange([
	        Path.Combine(VcpkgLibPath, "vcpkg_crashpad_client.lib"),
            Path.Combine(VcpkgLibPath, "vcpkg_crashpad_util.lib"),
            Path.Combine(VcpkgLibPath, "vcpkg_crashpad_base.lib"),
            // utf8_validity/utf8_range are protobuf 4.x+ only (removed for 3.21.x)
            Path.Combine(VcpkgLibPath, "mimalloc.lib"),
            Path.Combine(VcpkgLibPath, "fmt.lib"),
            Path.Combine(VcpkgLibPath, ProtobufLibName),
            Path.Combine(VcpkgLibPath, "redis++_static.lib"),
            Path.Combine(VcpkgLibPath, HiredisLibName),
            Path.GetFullPath(Path.Combine(SharedPath, $"Libraries/CppNetEngine/{ConfigName}/CppNetEngine.lib")),
            Path.GetFullPath(Path.Combine(SharedPath, $"Libraries/ProtoBridge/{ConfigName}/ProtoBridge.lib"))
        ]);

        // protobuf 3.21.x has no abseil dependency - remove absl linking if upgrading to 4.x+
        // if (Directory.Exists(VcpkgLibPath))
        // {
        //     var AbslLibs = Directory.GetFiles(VcpkgLibPath, "absl_*.lib");
        //     PublicAdditionalLibraries.AddRange(AbslLibs);
        // }

        bEnableExceptions = true;
        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off; // 최신 5.6+ 문법 적용

        PublicDefinitions.Add("PROTOBUF_ENABLE_DEBUG_LOGGING_MAY_LEAK_PII=0");
        PublicDefinitions.Add("GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE=0");
        PublicDefinitions.Add("_UNREAL_=1");
    }
}