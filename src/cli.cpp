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

#include "cli.h"

#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <iostream>

#include "config.h"
#include "log.h"
#include "native.h"

#ifndef WINDOWS
#include <boost/process.hpp>
#endif

namespace Cli {

using namespace std::literals;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

ProgramOptions options;

namespace {

void preprocessOptions(const po::options_description& desc) {
    Log::init(options.Verbose);
    if (options.Help) {
        desc.print(std::cout, 30);
        std::exit(0);
    }
    if (options.Version) {
        std::cout << "VSCodeConfigHelper v" PROJECT_VERSION " (c) Guyutongxue" << std::endl;
        std::exit(0);
    }
}

template <typename T>
void addOption(po::options_description& desc, const char* name, T& target, const char* info) {
    if constexpr (std::is_same_v<T, bool>) {
        desc.add_options()(name, po::bool_switch(&target), info);
    } else {
        desc.add_options()(name, po::value<T>(&target), info);
    }
}

std::pair<const char*, const char*> getLatestSupportStandardFromCompiler(
    const std::string& version) {
    try {
        std::vector<std::string> versionParts;
        boost::split(versionParts, version, boost::is_any_of("."));
        int majorVersion{std::stoi(versionParts.at(0))};
        if (majorVersion < 5) {
            int minorVersion{std::stoi(versionParts.at(1))};
            if (majorVersion == 4 && minorVersion > 7) {
                return {"c++11", "c11"};
            } else {
                return {"c++98", "c99"};
            }
        } else {
            switch (majorVersion) {
                case 5: return {"c++14", "c11"};
                case 6: return {"c++14", "c11"};
                case 7: return {"c++14", "c11"};
                case 8: return {"c++17", "c18"};
                case 9: return {"c++17", "c18"};
                case 10: return {"c++20", "c18"};
                case 11: return {"c++23", "c18"};

                default: return {"c++14", "c11"};
            }
        }
    } catch (...) {
        return {"c++14", "c11"};
    }
}
const std::array cppStandards{"c++98"s, "c++11"s, "c++14"s, "c++17"s, "c++20"s, "c++23"s};
const std::array cStandards{"c99"s, "c11"s, "c18"s};

template <typename T, typename U>
bool contains(const T& c, const U& v) {
    return std::find(std::begin(c), std::end(c), v) != std::end(c);
}

constexpr const char DEFAULT_GUI_ADDRESS[]{"https://vscch3.vercel.app/config.html"};

}  // namespace

void init(int argc, char** argv) {
    po::options_description generalOpt("General Options", 79);
    po::options_description configOpt("Configuration Options", 79);
    po::options_description advancedOpt("Advanced Options", 79);
#define ADD_OPTION_G(name, property, info) addOption(generalOpt, name, options.property, info)
#define ADD_OPTION_C(name, property, info) addOption(configOpt, name, options.property, info)
#define ADD_OPTION_A(name, property, info) addOption(advancedOpt, name, options.property, info)

    // IMPORTANT NOTE:
    // The length of the ASCII string must be a multiple of 3, otherwise the help message may fail
    // to print
    ADD_OPTION_G("verbose,V", Verbose, "显示详细的输出信息");
    ADD_OPTION_G("assume-yes,y", AssumeYes, "关闭命令行交互操作，总是假设选择“是”");
    ADD_OPTION_G("help,h", Help, "显示此帮助信息并退出");
    ADD_OPTION_G("version,v", Version, "显示程序版本信息并退出");
#ifdef WINDOWS
    ADD_OPTION_C("use-gui,g", UseGui,
                 "使用 GUI  进行配置。当不提供任何命令行参数时，此选项将被默认使用");
    ADD_OPTION_C("vscode-path", VscodePath,
                 "指定 VS Code 安装路径。若不提供，则工具自动从注册表获取");
    ADD_OPTION_C("mingw-path", MingwPath,
                 "指定  MinGW  的安装路径。若不提供，则工具自动从环境变量获取");
#else
    ADD_OPTION_C("compiler", Compiler, "指定编译器");
#endif
    ADD_OPTION_C("workspace-path", WorkspacePath, "指定工作区文件夹路径。若使用 CLI  则必须提供");
    // ADD_OPTION_C("language", Language, "");
    decltype(options.LanguageStandard) a;
    ADD_OPTION_C("language-standard", LanguageStandard,
                 "指定语言标准。若不提供，则工具根据 MinGW 编译器版本选取");
    ADD_OPTION_C("external-terminal", UseExternalTerminal, "使用外部终端进行运行和调试");
    ADD_OPTION_C("apply-nonascii-check", ApplyNonAsciiCheck,
                 "在调试前进行文件名中非 ASCII   字符的检查");
    ADD_OPTION_C("install-chinese", ShouldInstallL11n, "为 VS Code 安装中文语言包");
    ADD_OPTION_C("offline-cpptools", OfflineInstallCCpp, "离线安装  C/C++  扩展");
    ADD_OPTION_C("uninstall-extensions", ShouldUninstallExtensions, "卸载多余的 VS Code 扩展");
    // ADD_OPTION_C("generate-test", GenerateTestFile, "");
#ifdef WINDOWS
    ADD_OPTION_C("no-set-env", NoSetEnv, "不设置用户环境变量");
    ADD_OPTION_C("generate-shortcut", GenerateDesktopShortcut,
                 "生成指向工作区文件夹的桌面快捷方式");
#endif
    ADD_OPTION_C("open-vscode", OpenVscodeAfterConfig, "在配置完成后自动打开 VS Code");
    ADD_OPTION_C("no-send-analytics", NoSendAnalytics, "不发送统计信息");
    ADD_OPTION_A("remove-scripts", RemoveScripts, "删除此程序注入的所有脚本并退出");
#ifdef WINDOWS
    ADD_OPTION_A("no-open-browser", NoOpenBrowser, "使用 GUI  时不自动打开浏览器");
    ADD_OPTION_A("gui-address", GuiAddress, "指定使用 GUI  时自动打开的网页");
#endif

    // other options that cannot be parsed directly
    std::string languageText;
    std::string modeText;
    // clang-format off
    configOpt.add_options()
        ("language", po::value<std::string>(&languageText)->default_value("c++"), "指定配置目标语言。可为 c++  或 c")
        ("generate-test", "强制生成测试文件")
        ("no-generate-test", "不生成测试文件")
    ;
    // clang-format on

    po::options_description allOpts;
    allOpts.add(generalOpt).add(configOpt).add(advancedOpt);
    po::variables_map vm;
    std::optional<std::string> parseError;
    try {
        po::store(po::parse_command_line(argc, argv, allOpts), vm);
        po::notify(vm);
    } catch (const std::exception& e) {
        parseError = e.what();
    }
#ifdef WINDOWS
    if (argc == 1) {
        options.UseGui = true;
    }
#endif
    preprocessOptions(allOpts);

    if (parseError) {
        LOG_ERR("命令行参数存在错误：", *parseError);
    }
    Native::checkSystemVersion();

    // parse other options
    boost::to_lower(languageText);
    if (languageText == "c++") {
        options.Language = BaseOptions::LanguageType::Cpp;
    } else if (languageText == "c") {
        options.Language = BaseOptions::LanguageType::C;
    } else {
        LOG_ERR(languageText, "是不支持的目标语言。程序将退出");
        std::exit(1);
    }
    if (options.GuiAddress.empty()) {
        options.GuiAddress = DEFAULT_GUI_ADDRESS;
    }
    if (vm.count("generate-test")) {
        options.GenerateTestFile = BaseOptions::GenTestType::Always;
    } else if (vm.count("no-generate-test")) {
        options.GenerateTestFile = BaseOptions::GenTestType::Never;
    } else {
        options.GenerateTestFile = BaseOptions::GenTestType::Auto;
    }

#undef ADD_OPTION_G
#undef ADD_OPTION_C
#undef ADD_OPTION_A
}

boost::filesystem::path scriptDirectory() {
#ifdef WINDOWS
    return fs::path(options.MingwPath);
#else
    return fs::path(Native::getAppdata()) / ".local/bin";
#endif
}

void runCli(const Environment& env) {
    std::unique_ptr<const CompilerInfo> pInfo;
    const auto& compilers{env.Compilers()};
#ifdef WINDOWS
    if (options.MingwPath.empty()) {
        LOG_INF("未从命令行传入 MinGW 路径，将使用自动检测到的路径。");
        int chosen{0};
        if (compilers.size() == 0) {
            LOG_ERR("MinGW 路径未找到：命令行参数未传入且未自动检测到。程序将退出。");
            std::exit(1);
        } else if (compilers.size() == 1) {
            LOG_INF("选择自动检测到的 MinGW：", compilers.at(0).Path, " - ",
                    compilers.at(0).VersionNumber, " (", compilers.at(0).PackageString, ")");
            chosen = 0;
        } else {
            if (options.AssumeYes) {
                LOG_INF("由于启用了 -y，选择第一个 MinGW。");
                chosen = 0;
            } else {
                std::cout << "自动检测到多个 MinGW。请在其中选择一个：";
                for (int i{0}; i < compilers.size(); i++) {
                    std::cout << " [" << i << "] " << compilers.at(i).Path << " - "
                              << compilers.at(i).VersionNumber << " ("
                              << compilers.at(i).PackageString << ")" << std::endl;
                }
                std::cout << "输入你想要选择的 MinGW 编号，并按下回车：";
                std::string input;
                while (true) {
                    std::getline(std::cin, input);
                    try {
                        chosen = std::stoi(input);
                        if (chosen < 0 || chosen >= compilers.size()) {
                            throw std::out_of_range("");
                        }
                        break;
                    } catch (std::exception& e) {
                        std::cout << "不是合法的值。请重试：";
                    }
                }
            }
        }
        pInfo = std::make_unique<const CompilerInfo>(compilers.at(chosen));
        LOG_INF("在多个 MinGW 中选中 ", pInfo->Path, " - ", pInfo->VersionNumber, " (",
                pInfo->PackageString, ")");
    } else {
        LOG_INF("从命令行传入了 MinGW 路径 ", options.MingwPath, "，将使用此路径。");
        fs::path mingwPath{options.MingwPath};
        if (boost::to_lower_copy(mingwPath.filename().string()) != "bin") {
            mingwPath = mingwPath / "bin";
        }
        auto versionText{Environment::testCompiler(mingwPath)};
        if (!versionText) {
            LOG_ERR("验证 MinGW 失败：无法取得其版本信息（路径：", options.MingwPath,
                    "）。程序将退出。");
            std::exit(1);
        }
        LOG_INF("MinGW 路径 ", options.MingwPath, " 可用，版本信息：", *versionText);
        pInfo = std::make_unique<const CompilerInfo>(mingwPath.string(), *versionText);
    }
    options.MingwPath = pInfo->Path;
#else
    if (options.Compiler.empty()) {
        if (compilers.size() == 1) {
            pInfo = std::make_unique<const CompilerInfo>(compilers.at(0));
        } else {
            LOG_ERR("NO COMPILER");
            std::exit(1);
        }
    } else {
        auto fullPath{options.Compiler.find('/') == std::string::npos
                          ? boost::process::search_path(options.Compiler).string()
                          : options.Compiler};
        auto versionText{Environment::testCompiler(fullPath)};
        if (!versionText) {
            LOG_ERR("验证编译器 ", fullPath, " 失败：无法取得其版本信息。程序将退出。");
            std::exit(1);
        }
        LOG_INF("编译器 ", fullPath, " 可用，版本信息：", *versionText);
        pInfo = std::make_unique<const CompilerInfo>(fullPath, *versionText);
    }
#endif
    if (options.RemoveScripts) {
        LOG_INF("启用了开关 --remove-script，程序将删除所有脚本。");
        const char* filenames[]{"check-ascii.ps1", "pause-console" SCRIPT_EXT};
        for (const auto& filename : filenames) {
            auto scriptPath(scriptDirectory() / filename);
            if (fs::exists(scriptPath)) {
                fs::remove(scriptPath);
                LOG_INF("删除了脚本 ", scriptPath, "。");
            }
        }
        LOG_INF("脚本删除操作完成。程序将退出。");
        std::exit(0);
    }
#ifdef WINDOWS
    if (options.VscodePath.empty()) {
        LOG_INF("未从命令行传入 VS Code 路径，将使用自动检测到的路径。");
#endif
        const auto& vscodePath{env.VscodePath()};
        if (vscodePath) {
            LOG_INF("自动检测到的 VS Code 路径：", *vscodePath);
            options.VscodePath = *vscodePath;
        } else {
            LOG_ERR("VS Code 路径未找到：命令行参数未传入且未自动检测到。程序将退出。");
            std::exit(1);
        }
#ifdef WINDOWS
    } else {
        LOG_INF("从命令行传入了 VS Code 路径 ", options.VscodePath, "，将使用此路径。");
        if (!fs::exists(options.VscodePath)) {
            LOG_ERR("VS Code 路径验证失败：", options.VscodePath, "文件不存在。程序将退出。");
            std::exit(1);
        }
    }
#endif
    if (options.LanguageStandard.empty()) {
        LOG_INF("未从命令行传入语言标准，将根据编译器选择语言标准。");
        auto [cppStd, cStd]{getLatestSupportStandardFromCompiler(pInfo->VersionNumber)};
        options.LanguageStandard =
            options.Language == BaseOptions::LanguageType::Cpp ? cppStd : cStd;
        LOG_INF("将使用语言标准：", options.LanguageStandard, "。");
    } else {
        if (options.Language == BaseOptions::LanguageType::Cpp &&
            !contains(cppStandards, options.LanguageStandard)) {
            LOG_ERR(options.LanguageStandard, " 不是合法的 C++ 语言标准。程序将退出。");
            LOG_INF("合法的 C++ 语言标准有：", boost::join(cppStandards, ", "));
            std::exit(1);
        }
        if (options.Language == BaseOptions::LanguageType::C &&
            !contains(cStandards, options.LanguageStandard)) {
            LOG_ERR(options.LanguageStandard, " 不是合法的 C 语言标准。程序将退出。");
            LOG_INF("合法的 C 语言标准有：", boost::join(cStandards, ", "));
            std::exit(1);
        }
    }
    auto& args{options.CompileArgs};
    if (std::none_of(args.begin(), args.end(),
                     [](const std::string& x) { return x.starts_with("-std="); })) {
        LOG_INF("将添加 '-std=", options.LanguageStandard, "' 到编译参数中。");
        args.emplace_back("-std=" + options.LanguageStandard);
    }
    if (options.WorkspacePath.empty()) {
        LOG_ERR("未从命令行传入工作区文件夹路径（--workspace-path）。程序将退出。");
        std::exit(1);
    }
    if (fs::exists(fs::path(options.WorkspacePath) / ".vscode")) {
        LOG_WRN("工作区文件夹 ", options.WorkspacePath,
                " 下已存在配置。若继续则原有配置将被覆盖。");
        if (!options.AssumeYes) {
            std::cout << "是否继续？[Y/n] ";
            char key{Native::getch()};
            if (key != '\r' && key != 'y') {
                LOG_ERR("配置被用户中止。程序将退出。");
                std::exit(1);
            }
            std::cout << std::endl;
        }
    }
    Generator generator(options);
    generator.generate();
}
}  // namespace Cli