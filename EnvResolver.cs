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
using System.Text.RegularExpressions;
using System.Collections.Generic;
using Microsoft.Win32;

namespace VSCodeConfigHelper3 {

    class CompilerInfo {
        public string Path;
        public string? VersionText = null;
        public string? VersionNumber = null;
        public string? PackageString = null;

        public CompilerInfo(string path, string? versionText) {
            Path = path;
            VersionText = versionText;
            if (VersionText is not null) {
                // "g++.exe (Rev6, Built by MSYS2 project) 10.2.0"
                var splitted = VersionText.Split(' ');
                VersionNumber = splitted.Last();
                // remove "g++.exe" and "10.2.0", join other, remove "(" and ")"
                PackageString = string.Join(' ', splitted.Skip(1).SkipLast(1))[1..^1];
            }
        }
    }

    class EnvResolver {

        public string? VscodePath { get; }
        public List<CompilerInfo> Compilers { get; } = new List<CompilerInfo>();

        public EnvResolver() {
            VscodePath = GetVscodePath();
            foreach (var path in GetPaths()) {
                var versionText = TestCompiler(path);
                if (versionText is null) continue;
                Compilers.Add(new CompilerInfo(path,versionText));
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
                string? output = proc.StandardOutput.ReadLine();
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