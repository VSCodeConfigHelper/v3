#include <iostream>

#include "logger.h"
#include "native.h"

int main() {
    Log::init(true);
    LOG_INF(L"程序启动");
    LOG_WRN(L"Here is 中国");

    auto v{Native::getRegistry(HKEY_CLASSES_ROOT, L"vscode\\shell\\open\\command", L"")};
    if (v)
        std::wcout << v.value() << std::endl;
    Native::setCurrentUserEnv(L"TTTT", L"test");

    auto f{Native::browseFolder(L"")};
    if (f)
        std::wcout << f.value() << std::endl;

    auto r = Native::createLink(
        L"C:\\Users\\Guyutongxue\\Desktop\\Visual Studio Code.lnk",
        L"C:\\Users\\Guyutongxue\\AppData\\Local\\Programs\\Microsoft VS Code\\Code.exe",
        L"Visual Studio Code", L"\"E:\\DIY\\Code\\CSharp\\VSCodeConfigHelper3\\bin\\a\"");

    std::wcout << r << std::endl;
}