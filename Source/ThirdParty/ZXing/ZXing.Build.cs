using System;
using System.IO;
using UnrealBuildTool;

public class ZXing : ModuleRules {
  public ZXing(ReadOnlyTargetRules Target) : base(Target)
  {
    Type = ModuleType.External;
      
    PublicIncludePaths.Add(Path.Combine(ZXingSourcePath, "core", "src"));
    
    if (Target.Platform == UnrealTargetPlatform.Win64)
    {
      PublicAdditionalLibraries.Add(Path.Combine(ZXignIntermediatePath, "Win64", "core", "Release", "ZXing.lib"));
      RuntimeDependencies.Add(Path.Combine(ZXingBinariesPath, "Win64", "ZXing.dll"));
    }
    else if (Target.Platform == UnrealTargetPlatform.Mac)
    {
      PublicAdditionalLibraries.Add(Path.Combine(ZXignIntermediatePath, "Mac", "core", "libZXing.a"));
      RuntimeDependencies.Add(Path.Combine(ZXingBinariesPath, "Mac", "libZXing.dylib"));
    }
    else if (Target.Platform == UnrealTargetPlatform.Android)
    {
      PublicAdditionalLibraries.Add(Path.Combine(ZXignIntermediatePath, "Android", "core", "Release", "libZXing.a"));
      RuntimeDependencies.Add(Path.Combine(ZXingBinariesPath, "Android", "libZXing.so"));
    }
  } 
  
  private string ZXingSourcePath {
    get { return Path.Combine(ModuleDirectory, "zxing-cpp"); }
  }
  
  private string ZXignIntermediatePath {
    get { return Path.Combine(PluginDirectory, "Intermediate", "ThirdParty", "ZXing"); }
  }
  
  private string ZXingBinariesPath {
    get { return Path.Combine(PluginDirectory, "Binaries"); }
  }
}
