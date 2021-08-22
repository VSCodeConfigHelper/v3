// Copyright (C) 2021 Guyutongxue
// 
// This file is part of VS Code Config Helper.
// 
// VS Code Config Helper is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VS Code Config Helper is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VS Code Config Helper.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "generator.h"
#include "environment.h"

namespace Cli {

struct ProgramOptions : CurrentOptions {
    bool Verbose;
    bool AssumeYes;

    bool RemoveScripts;
    bool NoOpenBrowser;
    std::string GuiAddress;

    bool Help;
    bool Version;
};

extern ProgramOptions options;

void init(int argc, char** argv);

void runCli(const Environment& env);
}