using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using Microsoft.Win32;

namespace VSCodeConfigHelper3 {

    struct CompilerInfo {
        public string Path;
        public string Version;
    }

    class EnvResolver {

        public string? VscodePath { get; }
        public List<CompilerInfo> Compilers { get; } = new List<CompilerInfo>();

        public EnvResolver() {
            VscodePath = GetVscodePath();
            foreach (var path in GetPaths()) {
                var version = TestCompiler(path);
                if (version is null) continue;
                Compilers.Add(new CompilerInfo {
                    Path = path,
                    Version = version
                });
            }
        }

        private static string? TestCompiler(string path) {
            string compiler = Path.Join(path, "g++.exe");
            if (!File.Exists(compiler)) {
                return null;
            }
            try {
                var proc = new Process {
                    StartInfo = new ProcessStartInfo {
                        FileName = compiler,
                        ArgumentList = {
                            "--version"
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
                return output;
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.ToString());
                return null;
            }
        }

        private static string? GetVscodePath() {
            var rk = Registry.ClassesRoot.OpenSubKey(@"vscode\shell\open\command");
            if (rk is null) return null;
            // The value should be like:
            // "C:\Program Files\Microsoft VS Code\Code.exe" --open-url -- "%1"
            // and we just use the string inside the first quotation marks
            string? vscodePath = rk.GetValue("") is string s ? s.Split('"')[1] : null;
            if (!File.Exists(vscodePath)) return null;
            return vscodePath;
        }

        private static List<string> GetPaths() {
            string path = Environment.GetEnvironmentVariable("Path") ?? "";
            // https://stackoverflow.com/questions/21261314
            var paths = Regex.Split(path, ";(?=(?:[^\"]*\"[^\"]*\")*(?![^\"]*\"))");
            return paths.Select(str => Environment.ExpandEnvironmentVariables(str)).ToList();
        }

    }
}