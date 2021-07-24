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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Cache;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using CommandLine;
using Serilog;

namespace VSCodeConfigHelper3 {

    class ConfigOptions {
        public enum LanguageType {
            Cpp,
            C
        };

        [Option("vscode-path", HelpText = "Path to VS Code")]
        public string? VscodePath { get; set; }
        [Option("mingw-path")]
        public string? MingwPath { get; set; }
        [Option("workspace-path", HelpText = "Path to workspace")]
        public string? WorkspacePath { get; set; }

        [Option("language")]
        public LanguageType Language { get; set; } = LanguageType.Cpp;
        [Option("language-standard")]
        public string? LanguageStandard { get; set; }
        [Value(0, HelpText = "-- 之后的所有命令行参数都将作为编译参数传给 GCC。")]
        public IEnumerable<string> CompileArgs { get; set; } = new string[] { };

        [Option("no-set-env")]
        public bool NoSetEnv { get; set; }
        [Option("external-terminal")]
        public bool UseExternalTerminal { get; set; } = false;
        [Option("apply-nonascii-check")]
        public bool ApplyNonAsciiCheck { get; set; } = false;

        [Option("install-chinese")]
        public bool ShouldInstallL11n { get; set; } = false;
        [Option("uninstall-extensions")]
        public bool ShouldUninstallExtensions { get; set; } = false;

        [Option("generate-test")]
        public bool? GenerateTestFile { get; set; } = null;
        [Option("generate-shortcut")]
        public bool GenerateDesktopShortcut { get; set; }
        [Option("open-vscode")]
        public bool OpenVscodeAfterConfig { get; set; }

        [Option("no-send-analytics")]
        public bool NoSendAnalytics { get; set; }
    };

    class ExtensionManager {
        string path;
        HashSet<string> installedExtensions;
        static readonly HashSet<string> shouldUninstallExtensions = new HashSet<string>{
            "formulahendry.code-runner",
            "austin.code-gnu-global",
            "danielpinto8zz6.c-cpp-compile-run",
            "mitaki28.vscode-clang",
            "jaycetyle.vscode-gnu-global",
            "franneck94.c-cpp-runner",
            "ajshort.include-autocomplete",
            "xaver.clang-format",
            "jbenden.c-cpp-flylint"
        };

        public ExtensionManager(string vscodePath) {
            this.path = Path.Join(Path.GetDirectoryName(vscodePath), "bin\\code.cmd");
            this.installedExtensions = List();
        }

        string run(string args) {
            var proc = new Process {
                StartInfo = new ProcessStartInfo {
                    FileName = path,
                    Arguments = args,
                    RedirectStandardOutput = true,
                    CreateNoWindow = true
                }
            };
            proc.Start();
            string output = proc.StandardOutput.ReadToEnd();
            proc.WaitForExit();
            Log.Debug($"Run command `{path} {args}` get output:");
            Log.Debug(output);
            return output;
        }

        public HashSet<string> List() {
            try {
                Log.Information("获取当前安装已安装的扩展...");
                string output = run("--list-extensions");
                return output.Split(new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries).Select(s => s.Trim().ToLower()).ToHashSet();
            } catch (Exception e) {
                Log.Error(e, "获取当前已安装的扩展时发生错误。");
                throw;
            }
        }

        public void Install(string id) {
            id = id.ToLower();
            if (installedExtensions.Contains(id)) {
                Log.Information($"扩展 {id} 已安装。");
                return;
            }
            try {
                Log.Information($"尝试安装 {id}...");
                run("--install-extension " + id);
                installedExtensions.Add(id);
                Log.Information($"安装完成。");
            } catch (Exception e) {
                Log.Error(e, $"安装扩展 {id} 时发生错误。");
                throw;
            }
        }
        public void Uninstall(string id) {
            id = id.ToLower();
            if (!installedExtensions.Contains(id)) {
                Log.Information($"扩展 {id} 未安装。");
                return;
            }
            try {
                Log.Information($"尝试卸载 {id}...");
                run("--uninstall-extension " + id);
                installedExtensions.Remove(id);
                Log.Information("卸载完成。");
            } catch (Exception e) {
                Log.Error(e, "卸载扩展 {id} 时发生错误。");
                throw;
            }
        }

        public void UninstallAll() {
            foreach (var id in shouldUninstallExtensions) {
                Uninstall(id);
            }
        }
    }

    class ConfGenerator {

        ConfigOptions options;
        string CompilerExe {
            get {
                return options.Language == ConfigOptions.LanguageType.Cpp ? "g++.exe" : "gcc.exe";
            }
        }
        string FileExt {
            get {
                return options.Language == ConfigOptions.LanguageType.Cpp ? ".cpp" : ".c";
            }
        }

        public ConfGenerator(ConfigOptions options) {
            this.options = options;
        }

        public void Generate() {
            if (options.VscodePath is null || options.MingwPath is null || options.WorkspacePath is null) {
                throw new Exception("Some necessary paths are null.");
            }
            var extensions = new ExtensionManager(options.VscodePath);
            if (options.ShouldUninstallExtensions) {
                extensions.UninstallAll();
            }
            extensions.Install("ms-vscode.cpptools");
            if (options.ShouldInstallL11n) {
                extensions.Install("ms-ceintl.vscode-language-pack-zh-hans");
            }
            if (options.UseExternalTerminal) {
                AddScript("pause-console.ps1", options.MingwPath);
                AddKeybinding("f6", "workbench.action.tasks.runTask", "run and pause");
            }
            if (options.ApplyNonAsciiCheck) {
                AddScript("check-ascii.ps1", options.MingwPath);
            }
            var dotVscode = Path.Join(options.WorkspacePath, ".vscode");
            if (Directory.Exists(dotVscode)) {
                Directory.Delete(dotVscode, true);
                Log.Information("移除了旧的配置文件夹。");
            }
            Directory.CreateDirectory(dotVscode);
            GenerateTasksJson(Path.Join(dotVscode, "tasks.json"));
            GenerateLaunchJson(Path.Join(dotVscode, "launch.json"));
            GeneratePropertiesJson(Path.Join(dotVscode, "c_cpp_properties.json"));
            if (!options.NoSetEnv) {
                AddToPath(options.MingwPath);
            }
            if (options.GenerateTestFile is null) {
                options.GenerateTestFile = !File.Exists(Path.Join(options.WorkspacePath, "helloworld" + FileExt));
            }
            string? testFilename = null;
            if (options.GenerateTestFile is true) {
                testFilename = GenerateTestFile();
            }
            if (options.OpenVscodeAfterConfig) {
                OpenVscode(testFilename);
            }
            if (options.GenerateDesktopShortcut) {
                GenerateShortcut();
            }
            if (!options.NoSendAnalytics) {
                Analytics.Hit();
            }
        }

        void SaveStream(Stream stream, string path) {
            using (var fs = File.Create(path)) {
                stream.Seek(0, SeekOrigin.Begin);
                stream.CopyTo(fs);
                stream.Seek(0, SeekOrigin.Begin);
                Log.Debug(new StreamReader(stream).ReadToEnd());
            }
            Log.Information($"{path} 写入完成。");
        }

        void GenerateTasksJson(string filepath) {
            Log.Information($"生成 {filepath}...");
            using var stream = new MemoryStream();
            using var w = new Utf8JsonWriter(stream, new JsonWriterOptions {
                Indented = true,
                Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
            });
            w.WriteStartObject();
            {
                w.WriteString("version", "2.0.0");
                w.WritePropertyName("tasks"); w.WriteStartArray();
                {
                    w.WriteStartObject();
                    {
                        w.WriteString("type", "cppbuild");
                        w.WriteString("label", "gcc single file build");
                        w.WriteString("command", Path.Join(options.MingwPath, CompilerExe));
                        w.WritePropertyName("args"); w.WriteStartArray();
                        {
                            foreach (var arg in options.CompileArgs) {
                                w.WriteStringValue(arg);
                            }
                            w.WriteStringValue("-g");
                            w.WriteStringValue("${file}");
                            w.WriteStringValue("-o");
                            w.WriteStringValue("${fileDirname}\\${fileBasenameNoExtension}.exe");
                        }
                        w.WriteEndArray();
                        w.WritePropertyName("group"); w.WriteStartObject();
                        {
                            w.WriteString("kind", "build");
                            w.WriteBoolean("isDefault", true);
                        }
                        w.WriteEndObject();
                        w.WritePropertyName("presentation"); w.WriteStartObject();
                        {
                            w.WriteString("reveal", "silent");
                            w.WriteBoolean("focus", false);
                            w.WriteBoolean("echo", false);
                            w.WriteBoolean("showReuseMessage", false);
                            w.WriteString("panel", "shared");
                            w.WriteBoolean("clear", true);
                        }
                        w.WriteEndObject();
                        w.WriteString("problemMatcher", "$gcc");
                    }
                    w.WriteEndObject();
                    if (options.UseExternalTerminal) {
                        w.WriteStartObject();
                        {
                            w.WriteString("label", "run and pause");
                            w.WriteString("type", "shell");
                            w.WriteString("command", "START");
                            w.WriteString("dependsOn", "gcc single file build");
                            w.WritePropertyName("args"); w.WriteStartArray();
                            {
                                w.WriteStringValue("C:\\Windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe");
                                w.WriteStringValue("-ExecutionPolicy");
                                w.WriteStringValue("ByPass");
                                w.WriteStringValue("-NoProfile");
                                w.WriteStringValue("-File");
                                w.WriteStringValue(Path.Join(options.MingwPath, "pause-console.ps1"));
                                w.WriteStringValue("${fileDirname}\\${fileBasenameNoExtension}.exe");
                            }
                            w.WriteEndArray();
                            w.WritePropertyName("presentation"); w.WriteStartObject();
                            {
                                w.WriteString("reveal", "never");
                                w.WriteBoolean("focus", false);
                                w.WriteBoolean("echo", false);
                                w.WriteBoolean("showReuseMessage", false);
                                w.WriteString("panel", "shared");
                                w.WriteBoolean("clear", true);
                            }
                            w.WriteEndObject();
                            w.WritePropertyName("problemMatcher"); w.WriteStartArray();
                            w.WriteEndArray();
                        }
                        w.WriteEndObject();
                    }
                    if (options.ApplyNonAsciiCheck) {
                        w.WriteStartObject();
                        {
                            w.WriteString("label", "check ascii");
                            w.WriteString("type", "process");
                            w.WriteString("command", "C:\\Windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe");
                            w.WriteString("dependsOn", "gcc single file build");
                            w.WritePropertyName("args"); w.WriteStartArray();
                            {
                                w.WriteStringValue("-ExecutionPolicy");
                                w.WriteStringValue("ByPass");
                                w.WriteStringValue("-NoProfile");
                                w.WriteStringValue("-File");
                                w.WriteStringValue(Path.Join(options.MingwPath, "check-ascii.ps1"));
                                w.WriteStringValue("${fileDirname}\\${fileBasenameNoExtension}.exe");
                            }
                            w.WriteEndArray();
                            w.WritePropertyName("presentation"); w.WriteStartObject();
                            {
                                w.WriteString("reveal", "never");
                                w.WriteBoolean("focus", false);
                                w.WriteBoolean("echo", false);
                                w.WriteBoolean("showReuseMessage", false);
                                w.WriteString("panel", "shared");
                                w.WriteBoolean("clear", true);
                            }
                            w.WriteEndObject();
                            w.WritePropertyName("problemMatcher"); w.WriteStartArray();
                            w.WriteEndArray();
                        }
                        w.WriteEndObject();
                    }
                }
                w.WriteEndArray();
                w.WritePropertyName("options"); w.WriteStartObject();
                {
                    w.WritePropertyName("shell"); w.WriteStartObject();
                    {
                        w.WriteString("executable", "C:\\Windows\\System32\\cmd.exe");
                        w.WritePropertyName("args"); w.WriteStartArray();
                        {
                            w.WriteStringValue("/C");
                        }
                        w.WriteEndArray();
                    }
                    w.WriteEndObject();
                    w.WritePropertyName("env"); w.WriteStartObject();
                    {
                        w.WriteString("Path", options.MingwPath + ";${env:Path}");
                    }
                    w.WriteEndObject();
                }
                w.WriteEndObject();
            }
            w.WriteEndObject();
            w.Flush();
            SaveStream(stream, filepath);
        }

        void GenerateLaunchJson(string filepath) {
            Log.Information($"生成 {filepath}...");
            using var stream = new MemoryStream();
            using var w = new Utf8JsonWriter(stream, new JsonWriterOptions {
                Indented = true,
                Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
            });
            w.WriteStartObject();
            {
                w.WriteString("version", "0.2.0");
                w.WritePropertyName("configurations"); w.WriteStartArray();
                {
                    w.WriteStartObject();
                    {
                        w.WriteString("name", "gcc single file debug");
                        w.WriteString("type", "cppdbg");
                        w.WriteString("request", "launch");
                        w.WriteString("program", "${fileDirname}\\${fileBasenameNoExtension}.exe");
                        w.WritePropertyName("args"); w.WriteStartArray();
                        w.WriteEndArray();
                        w.WriteBoolean("stopAtEntry", false);
                        w.WriteString("cwd", "${fileDirname}");
                        w.WritePropertyName("environment"); w.WriteStartArray();
                        w.WriteEndArray();
                        w.WriteBoolean("externalConsole", options.UseExternalTerminal);
                        w.WriteString("MIMode", "gdb");
                        w.WriteString("miDebuggerPath", Path.Join(options.MingwPath, "gdb.exe"));
                        w.WritePropertyName("setupCommands"); w.WriteStartArray();
                        {
                            w.WriteStartObject();
                            {
                                w.WriteString("text", "-enable-pretty-printing");
                                w.WriteBoolean("ignoreFailures", true);
                            }
                            w.WriteEndObject();
                        }
                        w.WriteEndArray();
                        w.WriteString("preLaunchTask", options.ApplyNonAsciiCheck ? "check ascii" : "gcc single file build");
                        w.WriteString("internalConsoleOptions", "neverOpen");
                    }
                    w.WriteEndObject();
                }
                w.WriteEndArray();
            }
            w.WriteEndObject();
            w.Flush();
            SaveStream(stream, filepath);
        }

        void GeneratePropertiesJson(string filepath) {
            Log.Information($"生成 {filepath}...");
            using var stream = new MemoryStream();
            using var w = new Utf8JsonWriter(stream, new JsonWriterOptions {
                Indented = true,
                Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
            });
            w.WriteStartObject();
            {
                w.WriteNumber("version", 4);
                w.WritePropertyName("configurations"); w.WriteStartArray();
                {
                    w.WriteStartObject();
                    {
                        w.WriteString("name", "gcc");
                        w.WritePropertyName("includePath"); w.WriteStartArray();
                        {
                            w.WriteStringValue("${workspaceFolder}/**");
                        }
                        w.WriteEndArray();
                        w.WriteString("compilerPath", Path.Join(options.MingwPath, CompilerExe));
                        if (options.Language == ConfigOptions.LanguageType.Cpp) {
                            // https://github.com/microsoft/vscode-cpptools/issues/7506
                            if (options.LanguageStandard == "c++23")
                                w.WriteString("cppStandard", "c++20");
                            else
                                w.WriteString("cppStandard", options.LanguageStandard);
                        } else {
                            w.WriteString("cStandard", options.LanguageStandard);
                        }
                        w.WriteString("intelliSenseMode", "windows-gcc-x64");
                    }
                    w.WriteEndObject();
                }
                w.WriteEndArray();
            }
            w.WriteEndObject();
            w.Flush();
            SaveStream(stream, filepath);
        }

        void AddScript(string filename, string path) {
            Log.Information($"将脚本 {filename} 添加到路径 {path} 中...");
            string destination = Path.Join(path, filename);
            if (File.Exists(destination)) {
                Log.Information($"{destination} 已经存在。");
                return;
            }
            Assembly current = typeof(Program).GetTypeInfo().Assembly;
            using (var reader = current.GetManifestResourceStream($"VSCodeConfigHelper3.scripts.{filename}")) {
                if (reader is null) {
                    throw new FileNotFoundException($"Cannot read {filename} from assembly.");
                }
                using (var writer = File.Create(destination)) {
                    reader.Seek(0, SeekOrigin.Begin);
                    reader.CopyTo(writer);
                }
            }
            Log.Information($"{filename} 添加完成。");
        }

        void AddKeybinding(string key, string command, string? args = null) {
            string filepath = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\Code\\User\\keybindings.json";
            Log.Information($"将快捷键 {key} ({command} {args}) 添加到 {filepath} 中...");
            using var stream = new MemoryStream();
            using var w = new Utf8JsonWriter(stream, new JsonWriterOptions {
                Indented = true,
                Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
            });
            w.WriteStartArray();
            {
                w.WriteStartObject();
                {
                    w.WriteString("key", key);
                    w.WriteString("command", command);
                    w.WriteString("args", args);
                }
                w.WriteEndObject();
                if (File.Exists(filepath)) {
                    Log.Information($"快捷键设置已经存在，尝试合并...");
                    try {
                        using (JsonDocument doc = JsonDocument.Parse(File.ReadAllText(filepath))) {
                            JsonElement root = doc.RootElement;
                            foreach (var i in root.EnumerateArray()) {
                                if (i.TryGetProperty("key", out JsonElement thisKey) && thisKey.GetString() == key) {
                                    Log.Warning($"目标快捷键 {key} 已有设置。其将被覆盖。");
                                    continue;
                                }
                                i.WriteTo(w);
                            }
                        }
                        Log.Information("合并成功完成。");
                    } catch (Exception e) {
                        Log.Warning(e, "合并失败。原有快捷键将丢失。");
                    }
                }
            }
            w.WriteEndArray();
            w.Flush();
            SaveStream(stream, filepath);
        }

        static void AddToPath(string path) {
            Log.Information($"添加 {path} 到 Path 中...");
            string origin = Environment.GetEnvironmentVariable("Path", EnvironmentVariableTarget.User) ?? "";
            var paths = Regex.Split(origin, ";(?=(?:[^\"]*\"[^\"]*\")*(?![^\"]*\"))");
            var pathsList = paths.Select(str => str.Replace("\"", "")).ToList();
            // Find if there already exists a path. If so, move it to the front.
            var replaceIndex = pathsList.FindIndex(p => Environment.ExpandEnvironmentVariables(p) == path);
            if (replaceIndex == -1) {
                pathsList.Add(path);
            } else {
                pathsList.MoveItemAtIndexToFront(replaceIndex);
            }
            Environment.SetEnvironmentVariable("Path", string.Join(';', pathsList), EnvironmentVariableTarget.User);
        }

        string GenerateTestFile() {
            string filepath = Path.Join(options.WorkspacePath, $"helloworld{FileExt}");
            {
                int i = 1;
                while (File.Exists(filepath)) {
                    filepath = Path.Join(options.WorkspacePath, $"helloworld({i++}){FileExt}");
                }
            }
            Log.Information($"正在生成测试文件 {filepath}...");
            string compileHotkeyComment = "按下 " + (options.UseExternalTerminal ? "F6" : "Ctrl + F5") + " 编译运行。";
            string compileResultComment = "按下 " + (options.UseExternalTerminal ? "F5 后，您将在下方弹出的终端（Terminal）" : "F6 后，您将在弹出的") + "窗口中看到这一行字。";
            bool isCpp = options.Language == ConfigOptions.LanguageType.Cpp;
            Func<string, string> C = isCpp ? (s => "// " + s) : (s => "/* " + s + " */");

            StringBuilder sb = new StringBuilder();
            sb.AppendLine(C("VS Code C/C++ 测试代码 \"Hello World\""));
            sb.AppendLine(C("由 VSCodeConfigHelper 生成"));
            sb.AppendLine();
            sb.AppendLine(C("您可以在当前的文件夹（工作文件夹）下编写代码。"));
            sb.AppendLine();
            sb.AppendLine(C(compileHotkeyComment));
            sb.AppendLine(C("按下 F5 编译调试。"));
            sb.AppendLine(C("按下 Ctrl + Shift + B 编译，但不运行。"));
            if (isCpp) {
                sb.AppendLine(@"
#include <iostream>

/**
 * 程序执行的入口点。
 */
int main() {
    // 在标准输出中打印 ""Hello, world!""
    std::cout << ""Hello, world!"" << std::endl;
}");
            } else {
                sb.AppendLine(@"
#include <stdio.h>
#include <stdlib.h>

/**
 * 程序执行的入口点。
 */
int main(void) {
    /* 在标准输出中打印 ""Hello, world!"" */
    printf(""Hello, world!"");
    return EXIT_SUCCESS;
}");
            }
            sb.AppendLine();
            sb.AppendLine(C("此文件编译运行将输出 \"Hello, world!\"。"));
            sb.AppendLine(C(compileResultComment));
            sb.AppendLine(C("** 重要提示：您以后编写其它代码时，请务必确保文件名不包含中文或特殊字符，切记！**"));
            sb.AppendLine(C("如果遇到了问题，请您浏览"));
            sb.AppendLine(C("https://github.com/Guyutongxue/VSCodeConfigHelper/blob/master/TroubleShooting.md"));
            sb.AppendLine(C("获取帮助。如果问题未能得到解决，请联系开发者 guyutongxue@163.com。"));

            using (var sw = new StreamWriter(filepath)) {
                sw.Write(sb.ToString());
            }
            Log.Information("测试文件写入完成。");
            return filepath;
        }

        void OpenVscode(string? filepath) {
            Debug.Assert(options.WorkspacePath is not null && options.VscodePath is not null);
            string folderpath = options.WorkspacePath;
            Log.Information("启动 VS Code...");
            Process.Start(new ProcessStartInfo {
                FileName = options.VscodePath,
                Arguments = filepath is null ? $"\"{folderpath}\"" : $"\"{folderpath}\" -g \"{filepath}:1\"",
                CreateNoWindow = true
            });
        }

        void GenerateShortcut() {
            Log.Information("正在生成桌面快捷方式...");
            string shortcutPath = Path.Join(Environment.GetFolderPath(Environment.SpecialFolder.Desktop), "Visual Studio Code.lnk");
            if (File.Exists(shortcutPath)) {
                Log.Warning($"快捷方式 {shortcutPath} 已存在，将移除。");
                File.Delete(shortcutPath);
            }
            Debug.Assert(options.VscodePath is not null && options.WorkspacePath is not null);
            string targetPath = Path.GetFullPath(options.WorkspacePath);
            Shortcut.Create(
                shortcutPath,
                options.VscodePath,
                $"\"{targetPath}\"",
                Path.GetDirectoryName(options.VscodePath)!,
                $"Open VS Code at {targetPath}",
                null,
                null
            );
            Log.Information("桌面快捷方式生成完成。");
        }
    }

    public static class ListExtensions {
        public static void MoveItemAtIndexToFront<T>(this List<T> list, int index) {
            T item = list[index];
            for (int i = index; i > 0; i--)
                list[i] = list[i - 1];
            list[0] = item;
        }
    }

    // https://stackoverflow.com/a/14141782
    public class Shortcut {
        private static Type m_type = Type.GetTypeFromProgID("WScript.Shell")!;
        private static object m_shell = Activator.CreateInstance(m_type)!;

        [ComImport, TypeLibType((short)0x1040), Guid("F935DC23-1CF0-11D0-ADB9-00C04FD58A0B")]
        private interface IWshShortcut {
            [DispId(0)]
            string FullName { [return: MarshalAs(UnmanagedType.BStr)] [DispId(0)] get; }
            [DispId(0x3e8)]
            string Arguments { [return: MarshalAs(UnmanagedType.BStr)] [DispId(0x3e8)] get; [param: In, MarshalAs(UnmanagedType.BStr)] [DispId(0x3e8)] set; }
            [DispId(0x3e9)]
            string Description { [return: MarshalAs(UnmanagedType.BStr)] [DispId(0x3e9)] get; [param: In, MarshalAs(UnmanagedType.BStr)] [DispId(0x3e9)] set; }
            [DispId(0x3ea)]
            string Hotkey { [return: MarshalAs(UnmanagedType.BStr)] [DispId(0x3ea)] get; [param: In, MarshalAs(UnmanagedType.BStr)] [DispId(0x3ea)] set; }
            [DispId(0x3eb)]
            string IconLocation { [return: MarshalAs(UnmanagedType.BStr)] [DispId(0x3eb)] get; [param: In, MarshalAs(UnmanagedType.BStr)] [DispId(0x3eb)] set; }
            [DispId(0x3ec)]
            string RelativePath { [param: In, MarshalAs(UnmanagedType.BStr)] [DispId(0x3ec)] set; }
            [DispId(0x3ed)]
            string TargetPath { [return: MarshalAs(UnmanagedType.BStr)] [DispId(0x3ed)] get; [param: In, MarshalAs(UnmanagedType.BStr)] [DispId(0x3ed)] set; }
            [DispId(0x3ee)]
            int WindowStyle { [DispId(0x3ee)] get; [param: In] [DispId(0x3ee)] set; }
            [DispId(0x3ef)]
            string WorkingDirectory { [return: MarshalAs(UnmanagedType.BStr)] [DispId(0x3ef)] get; [param: In, MarshalAs(UnmanagedType.BStr)] [DispId(0x3ef)] set; }
            [TypeLibFunc((short)0x40), DispId(0x7d0)]
            void Load([In, MarshalAs(UnmanagedType.BStr)] string PathLink);
            [DispId(0x7d1)]
            void Save();
        }

        public static void Create(string fileName, string targetPath, string arguments, string workingDirectory, string description, string? hotkey, string? iconPath) {
            IWshShortcut shortcut = (IWshShortcut)m_type.InvokeMember("CreateShortcut", System.Reflection.BindingFlags.InvokeMethod, null, m_shell, new object[] { fileName })!;
            shortcut.Description = description;
            shortcut.TargetPath = targetPath;
            shortcut.WorkingDirectory = workingDirectory;
            shortcut.Arguments = arguments;
            if (!string.IsNullOrEmpty(hotkey))
                shortcut.Hotkey = hotkey;
            if (!string.IsNullOrEmpty(iconPath))
                shortcut.IconLocation = iconPath;
            shortcut.Save();
        }
    }

    public class Analytics {
        // https://countapi.xyz/
        // with namespace 'guyutongxue.github.io', key: b54f2252-e54a-4bd0-b4c2-33b47db6aa98
        public static void Hit() {
            try {
                // Logging.Log("Hit a count.");
                ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12 | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls;
                HttpWebRequest request = WebRequest.CreateHttp("https://api.countapi.xyz/hit/guyutongxue.github.io/b54f2252-e54a-4bd0-b4c2-33b47db6aa98");
                request.UserAgent = $"VSCodeConfigHelper v{Assembly.GetExecutingAssembly().GetName().Version}";
                request.Method = "GET";
                request.Timeout = 5000;
                HttpWebResponse response = (HttpWebResponse)request.GetResponse();
                StreamReader sr = new StreamReader(response.GetResponseStream(), Encoding.UTF8);
                sr.ReadToEnd();
            } catch (Exception ex) {
                Log.Error(ex, "发送统计数据失败。");
            }
        }
    }
}