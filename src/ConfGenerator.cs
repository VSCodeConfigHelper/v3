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
        enum LanguageType {
            Cpp,
            C
        };
        LanguageType Language { get; set; }
        string? LanguageStandard { get; set; }
        List<string> CompilerArgs { get; set; } = new();

        string VscodePath { get; set; }
        string WorkspacePath { get; set; }
        CompilerInfo Compiler { get; set; }

        bool ShouldSetEnv { get; set; } = true;
        bool UseExternalTerminal { get; set; } = false;

        bool ShouldUninstallExtensions { get; set; } = false;
        List<string> UninstallExtensions { get; set; } = new List<string>{
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

        bool ApplyNonAsciiCheck { get; set; } = false;

        bool GenerateDesktopShortcut { get; set; } = false;
        bool? GenerateTestFile { get; set; } = null;
        bool OpenVscodeAfterConfig { get; set; } = true;

        bool SendAnalytics { get; set; } = true;

        ConfigOptions(LanguageType lang, string vscodePath, string workspacePath, CompilerInfo compiler) {
            Language = lang;
            VscodePath = vscodePath;
            WorkspacePath = workspacePath;
            Compiler = compiler;
        }
    };


    class ConfGenerator {

        ConfGenerator() {
            
        }
    };
}