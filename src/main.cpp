#include <windows.h>
#include <shellapi.h>

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
    if (Cli::options.UseGui) {
        Server::runGui(env);
    } else {
        Cli::runCli(env);
    }
}