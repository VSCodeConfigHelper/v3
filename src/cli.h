#pragma once

#include "generator.h"
#include "environment.h"

namespace Cli {
struct ProgramOptions : ConfigOptions {
    bool UseGui;
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