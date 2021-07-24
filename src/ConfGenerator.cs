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
using System.Reflection;
using System.Text.Json;
using System.Text.RegularExpressions;
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
        static readonly List<string> uninstallExtensions = new List<string>{
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

        [Option("generate-test")]
        public bool? GenerateTestFile { get; set; } = null;
        [Option("generate-shortcut")]
        public bool GenerateDesktopShortcut { get; set; }
        [Option("open-vscode")]
        public bool OpenVscodeAfterConfig { get; set; }

        [Option("no-send-analytics")]
        public bool NoSendAnalytics { get; set; }
    };


    class ConfGenerator {

        ConfigOptions options;
        string CompilerExe {
            get {
                return options.Language == ConfigOptions.LanguageType.Cpp ? "g++.exe" : "gcc.exe";
            }
        }

        public ConfGenerator(ConfigOptions options) {
            this.options = options;
        }

        public void Generate() {
            if (options.VscodePath is null || options.MingwPath is null || options.WorkspacePath is null) {
                throw new Exception("Some necessary paths are null.");
            }
            if (options.UseExternalTerminal) {
                CompileVendor("ConsolePauser.cpp", Path.Join(options.MingwPath, "ConsolePauser.exe"));
            }
            var dotVscode = Path.Join(options.WorkspacePath, ".vscode");
            if (Directory.Exists(dotVscode)) {
                Directory.Delete(dotVscode, true);
            }
            Directory.CreateDirectory(dotVscode);
            GenerateTasksJson(Path.Join(dotVscode, "tasks.json"));

            if (!options.NoSetEnv) {
                AddToPath(options.MingwPath);
            }
        }

        void GenerateTasksJson(string filepath) {
            using var stream = File.Create(filepath);
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
                        w.WriteString("type", "shell");
                        w.WriteString("label", "g++ single file build");
                        w.WriteString("command", Path.Join(options.MingwPath, CompilerExe));
                        w.WritePropertyName("args"); w.WriteStartArray();
                        {
                            foreach (var arg in options.CompileArgs) {
                                w.WriteStringValue(arg);
                            }
                            w.WriteStringValue("-g");
                            w.WriteStringValue("\"${file}\"");
                            w.WriteStringValue("-o");
                            w.WriteStringValue("\"${fileDirname}\\${fileBasenameNoExtension}.exe\"");
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
                    w.WriteStartObject();
                    {
                        w.WriteString("label", "run with pauser");
                        w.WriteString("type", "shell");
                        w.WriteString("command", "TODO");
                    }
                    w.WriteEndObject();
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
        }

        void CompileVendor(string filename, string destination) {
            if (File.Exists(destination)) {
                Log.Information($"{destination} 已经存在，无需编译。");
                return;
            }
            Log.Information($"尝试编译 {filename}...");
            // Load source to temp file
            Assembly current = typeof(Program).GetTypeInfo().Assembly;
            string fileLocation = Path.Join(Path.GetTempPath(), filename);
            using (var reader = current.GetManifestResourceStream($"VSCodeConfigHelper3.vendor.{filename}")) {
                if (reader is null) {
                    throw new FileNotFoundException($"Cannot read {filename} from assembly.");
                }
                if (File.Exists(fileLocation)) {
                    File.Delete(fileLocation);
                }
                using (var writer = File.Create(fileLocation)) {
                    reader.Seek(0, SeekOrigin.Begin);
                    reader.CopyTo(writer);
                }
            }
            // compile file to destination
            Debug.Assert(options.MingwPath is not null);
            var compiler = Path.Combine(options.MingwPath, "g++.exe");
            var proc = new Process {
                StartInfo = new ProcessStartInfo {
                    FileName = compiler,
                    ArgumentList = {
                            fileLocation,
                            "-o",
                            destination
                        },
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true,
                }
            };
            Log.Debug($"Start g++ for compiling {filename}, stderr:");
            proc.Start();
            Log.Debug(proc.StandardError.ReadToEnd());
            proc.WaitForExit();
            Log.Debug($"Compiler exit with code {proc.ExitCode}.");
            if (proc.ExitCode != 0) {
                throw new Exception($"Compiling {filename} failed.");
            }
            Log.Information($"{destination} 编译完成。");
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

    }

    public static class ListExtensions {
        public static void MoveItemAtIndexToFront<T>(this List<T> list, int index) {
            T item = list[index];
            for (int i = index; i > 0; i--)
                list[i] = list[i - 1];
            list[0] = item;
        }
    }
}