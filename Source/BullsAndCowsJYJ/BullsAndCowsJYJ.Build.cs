//Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BullsAndCowsJYJ : ModuleRules
{
	public BullsAndCowsJYJ(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] 
		
	
		{
			//Initial Dependencies
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
		
			//UI
			"UMG", "Slate", "SlateCore",
			
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		PublicIncludePaths.AddRange(new string[] { "BullsAndCowsJYJ" });

}
}
