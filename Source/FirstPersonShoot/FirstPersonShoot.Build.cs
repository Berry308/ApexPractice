// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FirstPersonShoot : ModuleRules
{
	public FirstPersonShoot(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"NavigationSystem",
			"UMG",
			"MassEntity",
			// Mass 常用碎片 (FTransformFragment 等)
			"MassCommon", 
			// Mass 生成系统 (Spawner)
			"MassSpawner",
			// Mass 视觉表现 (LOD/StaticMesh 渲染)
			"MassRepresentation",
            "MassLOD",
			"UnLua"
        });
	}
}
