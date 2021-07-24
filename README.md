# VS Code Config Helper (v3)

**Under construction**

Track [VSCodeConfigHelper#15](https://github.com/Guyutongxue/VSCodeConfigHelper/issues/15) to get the latest updates on our work.

## Build instruction

- Install .NET 5 SDK.
- Update NuGet source. (Very important, or publish will fail) `https://api.nuget.org/v3/index.json`
- Publish with following command:
```
dotnet publish -r win-x64 --self-contained true /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true 
```
