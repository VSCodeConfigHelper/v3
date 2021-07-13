# VS Code Config Helper (v3)



## Build instruction

- Install .NET 5 SDK.
- Update NuGet source. (Very important, or publish will fail) `https://api.nuget.org/v3/index.json`
- Build.

Publish:
```
dotnet publish -r win-x64 --self-contained true /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true 
```