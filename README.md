# VS Code Config Helper (v3)

**Under construction**

Track [VSCodeConfigHelper#15](https://github.com/Guyutongxue/VSCodeConfigHelper/issues/15) to get the latest updates on our work.


## **IMPORTANT** About this branch

I'm now working on migrating the current code to C++. The main reason is that the .NET 5 single-file-executable is too large. It bundles almost all .NET Runtime, which brings a very poor user experience.

The C++ version will be developed under MinGW and released under MSVC. It uses CMake as the build tool and uses lots of libraries (Boost etc.) to implements all the features.

The work is a little bit hard. If the released executable (with static linkage to MSVCRT) is still too large, I may switch back to .NET then.


## Build instruction

- Install .NET 5 SDK.
- Update NuGet source. (Very important, or publish will fail) `https://api.nuget.org/v3/index.json`
- Publish with following command:
```
dotnet publish -r win-x64 --self-contained true /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true 
```
