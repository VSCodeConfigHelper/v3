// Copyright (C) 2021 Guyutongxue
// 
// This file is part of VSCodeConfigHelper.
// 
// VSCodeConfigHelper is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VSCodeConfigHelper is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VSCodeConfigHelper.  If not, see <http://www.gnu.org/licenses/>.

using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Text.Json;
using CommandLine;
using Serilog;

[assembly: SupportedOSPlatform("windows")]
namespace VSCodeConfigHelper3 {

    delegate string RequestHandler(string requestBody);

    class FolderGetter {

        [DllImport("kernel32.dll", ExactSpelling = true)]
        private static extern IntPtr GetConsoleWindow();

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetForegroundWindow(IntPtr hWnd);

        // bring console to front before launching FolderBrowserDialog
        // https://stackoverflow.com/a/12066376
        private void BringConsoleToFront() {
            SetForegroundWindow(GetConsoleWindow());
        }


        private string scriptLocation;
        private string powershellLocation;

        private void GenerateScript() {
            Assembly current = typeof(Program).GetTypeInfo().Assembly;
            using (var reader = current.GetManifestResourceStream(@"VSCodeConfigHelper3.scripts.get-folder.ps1")) {
                if (reader is null) {
                    throw new FileNotFoundException("Cannot read PowerShell script from assembly.");
                }
                if (File.Exists(scriptLocation)) {
                    File.Delete(scriptLocation);
                }
                using (var writer = File.Create(scriptLocation)) {
                    reader.Seek(0, SeekOrigin.Begin);
                    reader.CopyTo(writer);
                }
            }
        }

        public FolderGetter() {
            scriptLocation = Path.Join(Path.GetTempPath(), "get-folder.ps1");
            powershellLocation = Environment.ExpandEnvironmentVariables(@"%SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe");
            GenerateScript();
        }

        public string? Get(string initDir) {
            if (!File.Exists(scriptLocation)) {
                GenerateScript();
            }
            BringConsoleToFront();
            try {
                var proc = new Process {
                    StartInfo = new ProcessStartInfo {
                        FileName = powershellLocation,
                        ArgumentList = {
                            "-Sta",
                            "-ExecutionPolicy",
                            "Bypass",
                            "-NoProfile",
                            "-File",
                            scriptLocation,
                            initDir
                        },
                        RedirectStandardOutput = true,
                        CreateNoWindow = true,
                    }
                };
                proc.Start();
                string output = proc.StandardOutput.ReadToEnd();
                proc.WaitForExit();
                if (string.IsNullOrWhiteSpace(output)) {
                    return null;
                }
                return output.Trim();
            } catch (Exception ex) {
                Debug.WriteLine(ex.ToString());
                return null;
            }

        }
    }

    class ProgramOptions : ConfigOptions {
        [Option('V', "verbose", Default = false)]
        public bool Verbose { get; set; }
        [Option('g', "use-gui", Default = false)]
        public bool UseGui { get; set; }
        [Option('y', "assume-yes", Default = false)]
        public bool AssumeYes { get; set; }

        [Option("remove-scripts")]
        public bool RemoveScripts { get; set; }

        [Option("help")]
        public bool Help { get; set; }
        [Option("version")]
        public bool Version { get; set; }
    }

    static class Program {

        static EnvResolver? env = null;
        static Server? server = null;
        static ConfGenerator? config = null;

        static FolderGetter folderGetter = new();

        #region Handlers definitions

        static string HandleGetEnv(string _) {
            var json = JsonSerializer.Serialize(env);
            return json;
        }

        static string HandleGetFolder(string initDir) {
            return folderGetter.Get(initDir) ?? "";
        }

        static string HandleVerifyVscode(string path) {
            if (Path.GetFileName(path).ToLower() == "bin") {
                path = Path.GetDirectoryName(path) ?? path;
            }
            if (Directory.Exists(path) && File.Exists(Path.Join(path, "Code.exe"))) {
                return "valid";
            } else {
                return "invalid";
            }
        }

        static string HandleVerifyCompiler(string path) {
            if (Path.GetFileName(path).ToLower() != "bin") {
                path = Path.Join(path, "bin");
            }
            var versionText = EnvResolver.TestCompiler(path);
            if (versionText is null) return JsonSerializer.Serialize(new {
                valid = false
            });
            var info = new CompilerInfo(path, versionText);
            return JsonSerializer.Serialize(new {
                valid = true,
                info = info
            });
        }

        static string HandleVerifyWorkspace(string path) {
            if (Directory.Exists(Path.Join(path, ".vscode"))) {
                return "warn";
            } else {
                return "ok";
            }
        }

        static string HandleSaveProfile(string profile) {
            using (var writer = File.CreateText("profile.json")) {
                writer.Write(profile);
            }
            return "ok";
        }

        static string HandleLoadProfile(string profile) {
            if (!File.Exists("profile.json")) {
                return "null";
            }
            using (var reader = File.OpenText("profile.json")) {
                profile = reader.ReadToEnd();
            }
            return profile;
        }

        static string HandleDone(string req) {
            using var document = JsonDocument.Parse(req);
            var root = document.RootElement;
            var success = root.GetProperty("success");
            if (success.ValueKind == JsonValueKind.False) {
                return "ok";
            }
            var configText = root.GetProperty("config").GetRawText();
            var options = JsonSerializer.Deserialize<ConfigOptions>(configText);
            if (options is not null) {
                config = new ConfGenerator(options);
            }
            return "ok";
        }

        #endregion

        static bool isGui = true;

        static void RunWithOptions(ProgramOptions options) {
            // Create logger
            if (File.Exists("VSCH.log")) {
                File.Delete("VSCH.log");
            }
            if (options.Verbose) {
                Log.Logger = new LoggerConfiguration()
                    .MinimumLevel.Debug()
                    .WriteTo.Console(Serilog.Events.LogEventLevel.Information)
                    .WriteTo.File("VSCH.log")
                    .CreateLogger();
            } else {
                Log.Logger = new LoggerConfiguration()
                    .MinimumLevel.Debug()
                    .WriteTo.Console(Serilog.Events.LogEventLevel.Warning)
                    .WriteTo.File("VSCH.log")
                    .CreateLogger();
            }
            if (options.Version) {
                PrintVersion();
                Environment.Exit(0);
            }
            if (options.Help) {
                PrintHelp();
                Environment.Exit(0);
            }
            if (options.UseGui) {
                isGui = true;
            }
            if (Environment.OSVersion.Version.Major < 10) {
                Log.Error("此程序要求 Windows 10 或更高版本的操作系统。");
                if (isGui) {
                    Console.WriteLine("按下任意键退出...");
                    Console.ReadKey();
                }
                Environment.Exit(1);
            }
            Log.Information($"VSCodeConfigHelper 版本v{ProgramVersion()} 谷雨同学制作 guyutongxue@163.com");
            env = new EnvResolver();
            if (isGui) {
                // Launch GUI
                Log.Information("使用 GUI。");
                server = new Server(out string serveUrl);
                server.AddHandler("/getEnv", HandleGetEnv);
                server.AddHandler("/getFolder", HandleGetFolder);
                server.AddHandler("/verifyVscode", HandleVerifyVscode);
                server.AddHandler("/verifyCompiler", HandleVerifyCompiler);
                server.AddHandler("/verifyWorkspace", HandleVerifyWorkspace);
                server.AddHandler("/saveProfile", HandleSaveProfile);
                server.AddHandler("/loadProfile", HandleLoadProfile);
                server.AddHandler("/done", HandleDone, true);
                Log.Warning("即将启动浏览器，请在浏览器中继续操作。请不要最小化此窗口。");
                Server.OpenBrowser(serveUrl);
                server.Run();
                if (config is null) {
                    Log.Error("配置被用户中止。程序将退出。");
                    Environment.Exit(1);
                }
            } else {
                // Launch CLI
                CompilerInfo mingwInfo;
                if (options.MingwPath is null) {
                    Log.Information($"未从命令行传入 MinGW 路径，将使用自动检测到的路径。");
                    int chosen = 0;
                    if (env.Compilers.Count == 0) {
                        Log.Error("MinGW 路径未找到：命令行参数未传入且未自动检测到。程序将退出。");
                        Environment.Exit(1);
                    } else if (env.Compilers.Count == 1) {
                        Log.Information($"选择自动检测到的 MinGW：{env.Compilers[0].Path} - {env.Compilers[0].VersionNumber} ({env.Compilers[0].PackageString})");
                        chosen = 0;
                    } else {
                        if (options.AssumeYes) {
                            Log.Information($"由于启用了 -y，选择第一个 MinGW。");
                            chosen = 0;
                        } else {
                            Console.WriteLine("自动检测到多个 MinGW。请在其中选择一个：");
                            foreach (var (x, i) in env.Compilers.Select((x, i) => (x, i))) {
                                Console.WriteLine($"[{i}] {x.Path} - {x.VersionNumber} ({x.PackageString})");
                            }
                            Console.Write("输入你想要选择的 MinGW 编号，并按下回车：");
                            while (!int.TryParse(Console.ReadLine(), out chosen) || chosen < 0 || chosen >= env.Compilers.Count) {
                                Console.Write("不是合法的值。请重试：");
                            }
                        }
                    }
                    mingwInfo = env.Compilers[chosen];
                    Log.Information($"在多个 MinGW 中选中 {mingwInfo.Path} - {mingwInfo.VersionNumber} ({mingwInfo.PackageString})。");
                } else {
                    Log.Information($"从命令行传入了 MinGW 路径 {options.MingwPath}，将使用此路径。");
                    if (Path.GetFileName(options.MingwPath).ToLower() != "bin") {
                        options.MingwPath = Path.Join(options.MingwPath, "bin");
                    }
                    string? versionText = EnvResolver.TestCompiler(options.MingwPath);
                    if (versionText is null) {
                        Log.Error($"验证 MinGW 失败：无法取得其版本信息（路径：{options.MingwPath}）。程序将退出。");
                        Environment.Exit(1);
                    } else {
                        Log.Information($"MinGW 路径 {options.MingwPath} 可用，版本信息：{versionText}");
                    }
                    mingwInfo = new CompilerInfo(options.MingwPath, versionText);
                }
                options.MingwPath = mingwInfo.Path;

                if (options.RemoveScripts) {
                    Log.Information("启用了开关 --remove-script，程序将删除所有脚本。");
                    string[] filenames = new[] {
                        "check-ascii.ps1",
                        "pause-console.ps1"
                    };
                    foreach (var i in filenames) {
                        var path = Path.Join(options.MingwPath, i);
                        if (File.Exists(path)) {
                            File.Delete(path);
                            Log.Information($"删除了脚本 {path}。");
                        }
                    }
                    Log.Information("脚本删除操作完成。程序将退出。");
                    Environment.Exit(0);
                }

                if (options.VscodePath is null) {
                    Log.Information($"未从命令行传入 VS Code 路径，将使用自动检测到的路径：{env.VscodePath}");
                    if (env.VscodePath is null) {
                        Log.Error("VS Code 路径未找到：命令行参数未传入且未自动检测到。程序将退出。");
                        Environment.Exit(1);
                    }
                    options.VscodePath = env.VscodePath;
                } else {
                    Log.Information($"从命令行传入 VS Code 路径 {options.VscodePath}，将使用此路径。");
                    if (!File.Exists(options.VscodePath)) {
                        Log.Error($"验证 VS Code 路径失败：{options.VscodePath} 文件不存在。程序将退出。");
                        Environment.Exit(1);
                    }
                }

                (string, string) getLatestSupportStandardFromCompiler(string? version) {
                    if (version is null) {
                        return ("c++14", "c11");
                    }
                    int majorVersion = int.Parse(version.Split('.')[0]);
                    if (majorVersion < 5) {
                        int minorVersion = int.Parse(version.Split('.')[1]);
                        if (majorVersion == 4 && minorVersion > 7) {
                            return ("c++11", "c11");
                        } else {
                            return ("c++98", "c99");
                        }
                    } else {
                        switch (majorVersion) {
                            case 5: return ("c++14", "c11");
                            case 6: return ("c++14", "c11");
                            case 7: return ("c++14", "c11");
                            case 8: return ("c++17", "c18");
                            case 9: return ("c++17", "c18");
                            case 10: return ("c++20", "c18");
                            case 11: return ("c++23", "c18");

                            default: return ("c++14", "c11");
                        }
                    }
                }
                if (options.LanguageStandard is null) {
                    Log.Information("未从命令行传入语言标准，将根据编译器选择语言标准。");
                    var std = getLatestSupportStandardFromCompiler(mingwInfo.VersionNumber);
                    options.LanguageStandard = options.Language == ConfigOptions.LanguageType.Cpp ? std.Item1 : std.Item2;
                    Log.Information($"将使用语言标准：{options.LanguageStandard}。");
                } else {
                    Log.Information($"从命令行传入了语言标准 {options.LanguageStandard}，将使用此语言标准。");
                    string[] cppStandards = new[] { "c++98", "c++11", "c++14", "c++17", "c++20", "c++23" };
                    string[] cStandards = new[] { "c99", "c11", "c18" };
                    if (options.Language == ConfigOptions.LanguageType.Cpp && !cppStandards.Contains(options.LanguageStandard)) {
                        Log.Error($"{options.LanguageStandard} 不是合法的 C++ 语言标准。程序将退出。");
                        Log.Information($"合法的 C++ 语言标准有：{string.Join(',', cppStandards)}");
                        Environment.Exit(1);
                    }
                    if (options.Language == ConfigOptions.LanguageType.C && !cStandards.Contains(options.LanguageStandard)) {
                        Log.Error($"{options.LanguageStandard} 不是合法的 C 语言标准。程序将退出。");
                        Log.Information($"合法的 C 语言标准有：{string.Join(',', cStandards)}");
                        Environment.Exit(1);
                    }
                }
                var args = options.CompileArgs.ToList();
                if (args.Find(x => x.StartsWith("-std=")) is null) {
                    Log.Information($"将添加 '-std={options.LanguageStandard}' 到编译参数中。");
                    args.Add("-std=" + options.LanguageStandard);
                    options.CompileArgs = args;
                }
                if (options.WorkspacePath is null) {
                    Log.Error("未从命令行传入工作区文件夹路径。程序将退出。");
                    Environment.Exit(1);
                } else {
                    if (Directory.Exists(Path.Join(options.WorkspacePath, ".vscode"))) {
                        Log.Warning($"工作区文件夹 {options.WorkspacePath} 下已存在配置。若继续则原有配置将被覆盖。");
                        if (!options.AssumeYes) {
                            Console.Write("是否继续？[Y/n] ");
                            var ki = Console.ReadKey();
                            if (ki.Key != ConsoleKey.Enter && ki.Key != ConsoleKey.Y) {
                                Log.Error("配置被用户中止。程序将退出。");
                                Environment.Exit(1);
                            }
                            Console.WriteLine();
                        }
                    }
                }
                config = new ConfGenerator(options);
            }
            config.Generate();
            Environment.Exit(0);
        }

        public static Version? ProgramVersion() {
            return Assembly.GetExecutingAssembly().GetName().Version;
        }

        static void PrintVersion() {
            Console.WriteLine("VSCodeConfigHelper v" + ProgramVersion());
            Console.WriteLine("Copyright (c) 2021 Guyutongxue");
        }

        static void PrintHelp() {
            PrintVersion();
            Console.WriteLine("");
            Console.WriteLine("Usage: VSCodeConfigHelper.exe [<options>...] [-- <compile args>...]");
            Console.WriteLine("");
            // Width indicator (help message should not be wider than 80 chars)
            //567890123456789012345678901234567890123456789012345678901234567890
            Console.WriteLine(@"Options:
-V, --verbose                   显示详细的输出信息
-g, --use-gui                   使用 GUI 进行配置。当不提供任何命令行参数时，此
                                选项将被默认使用
-y, --assume-yes                关闭命令行交互操作，总是假设选择“是”
--vscode-path=<path>            指定 VS Code 安装路径。若不提供，则工具自动从注
                                册表获取
--mingw-path=<path>             指定 MinGW 的安装路径。若不提供，则工具自动从环
                                境变量获取
--workspace-path=<path>         指定工作区文件夹路径。若使用 CLI 则必须提供
--language=<Cpp|C>              指定配置目标语言。默认为 Cpp （大小写敏感）
--language-standard=<standard>  指定语言标准。默认根据 MinGW 编译器版本选取
--no-set-env                    不设置用户环境变量
--external-terminal             使用外部终端进行运行和调试
--apply-non-ascii-check         在调试前进行文件名中非 ASCII 字符的检查
--install-chinese               为 VS Code 安装中文语言包
--uninstall-extensions          卸载多余的 VS Code 扩展
--generate-test=<true|false>    指定是否强制或强制不生成测试文件。若不提供此参数
                                则自动选择
--generate-shortcut             是否生成指向工作区文件夹的桌面快捷方式
--open-vscode                   是否在配置完成后自动打开 VS Code
--no-send-analytics             不发送统计信息

--help                          显示此帮助信息并退出
--version                       显示程序版本并退出
--remove-scripts                从 MinGW 删除此程序注入的所有脚本并退出
");
            Console.WriteLine(@"Compile Args:
-- <any gcc options>...         -- 之后的所有命令行参数将作为编译参数传递给 GCC
");
        }


        static void Main(string[] args) {
            if (args.Length != 0) {
                isGui = false;
            }

            var result = new Parser(with => {
                with.AutoHelp = false;
                with.AutoVersion = false;
                with.EnableDashDash = true;
            })
                .ParseArguments<ProgramOptions>(args)
                .WithParsed(RunWithOptions)
                .WithNotParsed(errors => {
                    Environment.Exit(1);
                });
        }
    }
}
