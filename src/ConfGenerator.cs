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

namespace VSCodeConfigHelper3 {

    class ConfigOptions {
        public enum LanguageType {
            Cpp,
            C
        };
        public string? VscodePath { get; set; }
        public string? MingwPath { get; set; }
        public string? WorkspacePath { get; set; }

        public LanguageType Language { get; set; }
        public string? LanguageStandard { get; set; }
        public List<string> CompilerArgs { get; set; } = new();


        public bool ShouldSetEnv { get; set; } = true;
        public bool UseExternalTerminal { get; set; } = false;
        public bool ApplyNonAsciiCheck { get; set; } = false;

        public bool ShouldInstallL11n { get; set; } = false;
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

        public bool? GenerateTestFile { get; set; } = null;
        public bool GenerateDesktopShortcut { get; set; } = false;
        public bool OpenVscodeAfterConfig { get; set; } = true;

        public bool SendAnalytics { get; set; } = true;
    };


    class ConfGenerator {

        ConfigOptions options;

        public ConfGenerator(ConfigOptions options) {
            this.options = options;
        }

        public void Generate() {
            if (options.VscodePath is null || options.MingwPath is null || options.WorkspacePath is null) {
                throw new Exception("Some necessary paths are null.");
            }
            
        }
    };
}