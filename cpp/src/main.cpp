#include <iostream>

#include "env_resolver.h"
#include "server.h"
#include "logger.h"
#include "native.h"

#include <boost/nowide/iostream.hpp>
#include <boost/nowide/filesystem.hpp>

int main(int argc, char** argv) {
    boost::nowide::nowide_filesystem();
    Log::init(true);
    // LOG_INF("程序启动");
    // LOG_WRN("Here is 中国");

    // auto v{Native::getRegistry(HKEY_CLASSES_ROOT, "vscode\\shell\\open\\command", "")};
    // if (v)
    //     boost::nowide::cout << v.value() << std::endl;
    // Native::setCurrentUserEnv("TTTT", "hello中国");

    // auto f{Native::browseFolder("")};
    // if (f)
    //     boost::nowide::cout << f.value() << std::endl;

    // auto r = Native::createLink(
    //     "C:\\Users\\Guyutongxue\\Desktop\\Visual Studio Code.lnk",
    //     "C:\\Users\\Guyutongxue\\AppData\\Local\\Programs\\Microsoft VS Code\\Code.exe",
    //     "Visual Studio Code", "\"C:\\Users\\Guyutongxue\\Documents\\MyFiles\\VSCodeConfigHelper3\\bin\\a\"");

    // boost::nowide::cout << r << std::endl;

    // Server s;
    // int port{s.prepare()};
    // boost::nowide::cout << port << std::endl;
    // s.start();

    // auto r{EnvResolver::testCompiler("C:\\mingw64\\bin")};
    // if (r) {
    //     boost::nowide::cout << r.value() << std::endl;
    // } else {
    //     boost::nowide::cout << "nul" << std::endl;
    // }
    EnvResolver env;
}