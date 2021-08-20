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

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
#include <boost/nowide/args.hpp>
#include <boost/nowide/filesystem.hpp>

#include "cli.h"
#include "environment.h"
#include "log.h"
#include "native.h"
#include "server.h"

int main(int argc, char** argv) {
    boost::nowide::nowide_filesystem();
    boost::nowide::args _(argc, argv);
    Cli::init(argc, argv);
    // LOG_WRN("你好");
    Environment env;
#ifdef WINDOWS
    if (Cli::options.Mode == Cli::GuiMode)
        Server::runGui(env);
    else
#endif
        Cli::runCli(env);
}