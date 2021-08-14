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

#include "generator.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/assign.hpp>
#include <boost/process.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "config.h"
#include "log.h"
#include "native.h"

namespace bp = boost::process;
namespace fs = boost::filesystem;
using namespace boost::assign;
using namespace std::literals;

std::string ExtensionManager::runScript(const std::initializer_list<std::string>& args) {
    bp::ipstream is;
    bp::child proc(scriptPath, args, bp::std_out > is);
    proc.wait();
    std::ostringstream oss;
    std::copy(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>(),
              std::ostreambuf_iterator<char>(oss));
    return oss.str();
}

ExtensionManager::ExtensionManager(const boost::filesystem::path& vscodePath)
    : scriptPath{vscodePath.parent_path() / "bin\\code.cmd"} {
    installedExtensions = list();
}

std::unordered_set<std::string> ExtensionManager::list() {
    try {
        auto output{runScript({"--list-extensions"})};
        boost::trim(output), boost::to_lower(output);
        LOG_DBG(output);
        std::unordered_set<std::string> result;
        boost::split(result, output, boost::is_any_of("\r\n"));
        return result;
    } catch (...) {
        LOG_ERR("获取当前已安装的扩展时发生错误。");
        throw;
    }
}
void ExtensionManager::install(const std::string& id_orig) {
    auto id{boost::to_lower_copy(id_orig)};
    if (installedExtensions.find(id) != installedExtensions.end()) {
        LOG_INF("扩展 ", id, " 已安装。");
        return;
    }
    try {
        LOG_INF("尝试安装 ", id, "...");
        LOG_DBG(runScript({"--install-extension", id}));
        installedExtensions.insert(id);
        LOG_INF("安装完成。");
    } catch (...) {
        LOG_ERR("安装扩展 ", id, " 时发生错误。");
        throw;
    }
}

void ExtensionManager::uninstall(const std::string& id_orig) {
    auto id{boost::to_lower_copy(id_orig)};
    if (auto pos{installedExtensions.find(id)}; pos != installedExtensions.end()) {
        try {
            LOG_INF("尝试卸载 ", id, "...");
            LOG_DBG(runScript({"--uninstall-extension", id}));
            installedExtensions.erase(pos);
            LOG_INF("卸载完成。");
        } catch (...) {
            LOG_ERR("卸载扩展 ", id, " 时发生错误。");
            throw;
        }
    } else {
        LOG_INF("扩展 ", id, " 未安装。");
    }
}

void ExtensionManager::uninstallAll() {
    for (auto&& id : shouldUninstallExtensions) {
        uninstall(id);
    }
}

namespace {

const char offlineHost[]{"https://guyutongxue.oss-cn-beijing.aliyuncs.com"};
const char offlinePath[]{"/vscode-cpptools/cpptools-win32_v1.5.1.vsix"};
const char C_CPP_EXT_ID[]{"ms-vscode.cpptools"};

boost::regex splitPathRegex(";(?=(?:[^\"]*\"[^\"]*\")*(?![^\"]*\"))");

template <template <typename> typename ContainerT>
ContainerT<std::string> splitPath(std::optional<std::string> (*getter)(const std::string&)) {
    auto origin{getter("Path")};
    auto originalPath{origin ? *origin : ""};
    ContainerT<std::string> container;
    boost::split_regex(container, originalPath, splitPathRegex);
    return container;
}

}  // namespace

Generator::Generator(const ConfigOptions& options) : options{options} {}

const char* Generator::compilerExe() {
    return options.Language == ConfigOptions::LanguageType::Cpp ? "g++.exe" : "gcc.exe";
}
const char* Generator::fileExt() {
    return options.Language == ConfigOptions::LanguageType::Cpp ? ".cpp" : ".c";
}

void Generator::saveFile(const fs::path& path, const char* content) {
    LOG_INF("写入脚本 ", path, " 中...");
    if (fs::exists(path)) {
        LOG_INF("脚本 ", path, " 已存在，无需写入。");
    } else {
        fs::save_string_file(path, content);
        LOG_INF("写入完成。");
    }
}

void Generator::addKeybinding(const std::string& key, const std::string& command,
                              const std::string& args) {
    using json = nlohmann::json;
    auto filepath(fs::path(Native::getAppdata()) / "Code\\User\\keybindings.json");
    LOG_INF("将快捷键 ", key, " (", command, " ", args, ") 添加到 ", filepath, " 中...");
    auto result(json::array());
    if (fs::exists(filepath)) {
        LOG_INF("快捷键设置已经存在，尝试合并...");
        try {
            std::string content;
            fs::load_string_file(filepath, content);
            auto exists(nlohmann::json::parse(content));
            for (auto&& i : exists) {
                if (i["key"].get<std::string>() == key) {
                    LOG_WRN("目标快捷键 ", key, " 已有设置。其将被覆盖。");
                    continue;
                }
                result.push_back(i);
            }
        } catch (...) {
            LOG_WRN("合并快捷键设置时失败。将覆盖原有设置。");
        }
    }
    result += json::object({{"key", key}, {"command", command}, {"args", args}});

    auto resultStr{result.dump(2)};
    LOG_DBG(resultStr);
    fs::save_string_file(filepath, resultStr);
}

void Generator::addToPath(const fs::path& path) {
    auto newPath{path.string()};

    auto sysPaths{splitPath<std::unordered_set>(Native::getLocalMachineEnv)};
    if (sysPaths.find(newPath) != sysPaths.end()) {
        LOG_WRN("系统环境变量 Path 已包含 ", newPath, "。不会将其添加到用户环境变量 Path 中。");
        return;
    }

    LOG_INF("将 ", newPath, " 添加到用户环境变量 Path 中...");
    auto paths{splitPath<std::list>(Native::getCurrentUserEnv)};
    if (auto it{std::find(paths.begin(), paths.end(), newPath)}; it != paths.end()) {
        LOG_INF("用户环境变量 Path 已经包含 ", newPath, "。将其移动到最前。");
        paths.erase(it);
        paths.push_front(newPath);
    } else {
        LOG_INF("用户环境变量 Path 中未包含 ", newPath, "。将其添加。");
        paths.push_front(newPath);
    }
    auto result{boost::join(paths, ";")};
    LOG_DBG("Final Path: ", result);
    Native::setCurrentUserEnv("Path", result);
}

void Generator::generateTasksJson(const fs::path& path) {
    using json = nlohmann::json;
    LOG_INF("生成 ", path, " ...");
    auto args{options.CompileArgs};
    args += "-g", "${file}", "-o", "${fileDirname}\\${fileBasenameNoExtension}.exe";
    // clang-format off
    auto sfbTask(json::object({
        {"type", "process"}, // "cppbuild" won't apply options
        {"label", "gcc single file build"},
        {"command", (fs::path(options.MingwPath) / compilerExe()).string()},
        {"args", args},
        {"group", json::object({
            {"kind", "build"},
            {"isDefault", true}
        })},
        {"presentation", json::object({
            {"reveal", "silent"},
            {"focus", false},
            {"echo", false},
            {"showReuseMessage", false},
            {"panel", "shared"},
            {"clear", true}
        })},
        {"problemMatcher", "$gcc"}
    }));
    auto pauseTask(json::object({
        {"type", "shell"},
        {"label", "run and pause"},
        {"command", "START"},
        {"dependsOn", "gcc single file build"},
        {"args", json::array({
            "C:\\Windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe",
            "-ExecutionPolicy",
            "ByPass",
            "-NoProfile",
            "-File",
            (fs::path(options.MingwPath) / "pause-console.ps1").string(),
            "${fileDirname}\\${fileBasenameNoExtension}.exe"
        })},
        {"presentation", json::object({
            {"reveal", "never"},
            {"focus", false},
            {"echo", false},
            {"showReuseMessage", false},
            {"panel", "shared"},
            {"clear", true}
        })},
        {"problemMatcher", json::array()}
    }));
    auto asciiTask(json::object({
        {"type", "process"},
        {"label", "check ascii"},
        {"command", "C:\\Windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe"},
        {"dependsOn", "gcc single file build"},
        {"args", json::array({
            "-ExecutionPolicy",
            "ByPass",
            "-NoProfile",
            "-File",
            (fs::path(options.MingwPath) / "check-ascii.ps1").string(),
            "${fileDirname}\\${fileBasenameNoExtension}.exe"
        })},
        {"presentation", json::object({
            {"reveal", "never"},
            {"focus", false},
            {"echo", false},
            {"showReuseMessage", false},
            {"panel", "shared"},
            {"clear", true}
        })},
        {"problemMatcher", json::array()}
    }));
    auto allTasks(json::array({sfbTask}));
    if (options.UseExternalTerminal)
        allTasks += pauseTask;
    if (options.ApplyNonAsciiCheck)
        allTasks += asciiTask;
    auto result(json::object({
        {"version", "2.0.0"},
        {"tasks", allTasks},
        {"options", json::object({
            {"shell", json::object({
                {"executable", "C:\\Windows\\System32\\cmd.exe"},
                {"args", json::array({
                    "/C"
                })}
            })},
            {"env", json::object({
                {"Path", options.MingwPath + ";${env:Path}"}
            })}
        })}
    }));
    // clang-format on
    auto resultStr{result.dump(2)};
    LOG_DBG(resultStr);
    fs::save_string_file(path, resultStr);
}

void Generator::generateLaunchJson(const fs::path& path) {
    using json = nlohmann::json;
    LOG_INF("生成 ", path, " ...");
    // clang-format off
    auto result(json::object({
        {"version", "0.2.0"},
        {"configurations", json::array({
            json::object({
                {"name", "gcc single file debug"},
                {"type", "cppdbg"},
                {"request", "launch"},
                {"program", "${fileDirname}\\${fileBasenameNoExtension}.exe"},
                {"args", json::array({})},
                {"stopAtEntry", false},
                {"cwd", "${fileDirname}"},
                {"environment", json::array({})},
                {"externalConsole", options.UseExternalTerminal},
                {"MIMode", "gdb"},
                {"miDebuggerPath", (fs::path(options.MingwPath) / "gdb.exe").string()},
                {"setupCommands", json::array({
                    json::object({
                        {"text", "-enable-pretty-printing"},
                        {"ignoreFailures", true}
                    })
                })},
                {"preLaunchTask", options.ApplyNonAsciiCheck ? "check ascii" : "gcc single file build"},
                {"internalConsoleOptions", "neverOpen"}
            })
        })}
    }));
    // clang-format on
    auto resultStr{result.dump(2)};
    LOG_DBG(resultStr);
    fs::save_string_file(path, resultStr);
}
void Generator::generatePropertiesJson(const fs::path& path) {
    using json = nlohmann::json;
    LOG_INF("生成 ", path, " ...");
    // clang-format off
    auto result(json::object({
        {"version", 4},
        {"configurations", json::array({
            json::object({
                {"name", "gcc"},
                {"includePath", json::array({
                    "${workspaceFolder}/**"
                })},
                {"compilerPath", (fs::path(options.MingwPath) / compilerExe()).string()},
                {options.Language == ConfigOptions::LanguageType::Cpp ? 
                    "cppStandard" : 
                    "cStandard",
                options.LanguageStandard == "c++23" ?
                    "c++20" :
                    options.LanguageStandard},
                {"intelliSenseMode", "windows-gcc-x64"}
            })
        })}
    }));
    // clang-format on
    auto resultStr{result.dump(2)};
    LOG_DBG(resultStr);
    fs::save_string_file(path, resultStr);
}

std::string Generator::generateTestFile() {
    auto filepath{fs::path(options.WorkspacePath) / ("helloworld"s + fileExt())};
    for (int i{1}; fs::exists(filepath); i++) {
        filepath =
            fs::path(options.WorkspacePath) / ("helloworld(" + std::to_string(i) + ")" + fileExt());
    }
    LOG_INF("正在生成测试文件 ", filepath, "...");
    const std::string compileHotkeyComment{
        "按下 "s + (options.UseExternalTerminal ? "F6" : "Ctrl + F5") + " 编译运行。"};
    const std::string compileResultComment{"按下 "s +
                                           (options.UseExternalTerminal
                                                ? "F6 后，您将在弹出的"
                                                : "F5 后，您将在下方弹出的终端（Terminal）") +
                                           "窗口中看到这一行字。"};
    bool isCpp{options.Language == ConfigOptions::LanguageType::Cpp};
    std::string (*c)(const std::string&){nullptr};
    if (isCpp)  // I'm wonder why MSVC IntelliSense doesn't support Lambda in ?:.
        c = [](const std::string& s) { return "// " + s; };
    else
        c = [](const std::string& s) { return "/* " + s + " */"; };
    std::ostringstream oss;
    oss << c("VS Code C/C++ 测试代码 \"Hello World\"") << '\n';
    oss << c("由 VSCodeConfigHelper v" PROJECT_VERSION " 生成") << '\n';
    oss << '\n';
    oss << c("您可以在当前的文件夹（工作文件夹）下编写代码。") << '\n';
    oss << '\n';
    oss << c(compileHotkeyComment) << '\n';
    oss << c("按下 F5 编译调试。") << '\n';
    oss << c("按下 Ctrl + Shift + B 编译，但不运行。") << '\n';
    if (isCpp) {
        oss << R"(
#include <iostream>

/**
 * 程序执行的入口点。
 */
int main() {
    // 在标准输出中打印 "Hello, world!"
    std::cout << "Hello, world!" << std::endl;
}
)";
    } else {
        oss << R"(
#include <stdio.h>
#include <stdlib.h>

/**
 * 程序执行的入口点。
 */
int main(void) {
    /* 在标准输出中打印 "Hello, world!" */
    printf("Hello, world!");
    return EXIT_SUCCESS;
}
)";
    }
    oss << '\n';
    oss << c("此文件编译运行将输出 \"Hello, world!\"。") << '\n';
    oss << c(compileResultComment) << '\n';
    oss << c("** 重要提示：您以后编写其它代码时，请务必确保文件名不包含中文或特殊字符，切记！**")
        << '\n';
    oss << c("如果遇到了问题，请您浏览") << '\n';
    oss << c("https://github.com/Guyutongxue/VSCodeConfigHelper/blob/v2.x/TroubleShooting.md")
        << '\n';
    oss << c("获取帮助。如果问题未能得到解决，请联系开发者 guyutongxue@163.com。") << '\n';

    fs::save_string_file(filepath, oss.str());
    LOG_INF("测试文件写入完成。");
    return filepath.string();
}

void Generator::openVscode(const std::optional<std::string>& filename) {
    const auto& folderPath{fs::absolute(options.WorkspacePath).string()};

    std::vector args{folderPath};
    if (filename) {
        args += "--goto", *filename;
    }
    LOG_INF("启动 VS Code...");
    LOG_DBG(options.VscodePath, " ", boost::join(args, " "));
    try {
        bp::spawn(options.VscodePath, bp::args = args, bp::std_out > bp::null);
    } catch (std::exception& e) {
        LOG_WRN("启动 VS Code 失败：", e.what());
    }
}
void Generator::generateShortcut() {
    fs::path shortcutPath{fs::path(Native::getDesktop()) / "Visual Studio Code.lnk"};
    if (fs::exists(shortcutPath)) {
        LOG_WRN("快捷方式 ", shortcutPath, " 已存在，将被覆盖。");
        fs::remove(shortcutPath);
    }
    auto targetPath{fs::absolute(options.WorkspacePath).string()};
    auto result{Native::createLink(shortcutPath.string(), options.VscodePath,
                                   "Open VS Code at " + targetPath, "\"" + targetPath + "\"")};
    if (result) {
        LOG_INF("快捷方式 ", shortcutPath, " 已生成。");
    } else {
        LOG_WRN("快捷方式 ", shortcutPath, " 生成失败。");
    }
}

void Generator::generate() {
    try {
        fs::path dotVscode(fs::path(options.WorkspacePath) / ".vscode");

        ExtensionManager extensions(options.VscodePath);
        if (options.ShouldUninstallExtensions) {
            extensions.uninstallAll();
        }
        if (options.OfflineInstallCCpp) {
            extensions.installOffline(C_CPP_EXT_ID, offlineHost, offlinePath);
        } else {
            extensions.install(C_CPP_EXT_ID);
        }
        if (options.ShouldInstallL11n) {
            extensions.install("ms-ceintl.vscode-language-pack-zh-hans");
        }
        if constexpr (Native::isWindows) {
            fs::path mingwPath(options.MingwPath);
            if (options.UseExternalTerminal) {
                saveFile(mingwPath / "pause-console.ps1", Embed::PAUSE_CONSOLE_PS1);
                addKeybinding("f6", "workbench.action.tasks.runTask", "run and pause");
            }
            if (options.ApplyNonAsciiCheck) {
                saveFile(mingwPath / "check-ascii.ps1", Embed::CHECK_ASCII_PS1);
            }
            if (!options.NoSetEnv) {
                addToPath(mingwPath);
            }
        } else {
            // *nix specified
            // TODO
        }

        if (fs::exists(dotVscode)) {
            fs::remove_all(dotVscode);
            LOG_INF("移除了已存在的 .vscode 文件夹。");
        }
        fs::create_directories(dotVscode);
        generateTasksJson(dotVscode / "tasks.json");
        generateLaunchJson(dotVscode / "launch.json");
        generatePropertiesJson(dotVscode / "c_cpp_properties.json");
        if (options.GenerateTestFile == ConfigOptions::GenTestType::Auto) {
            if (fs::exists(fs::path(options.WorkspacePath) / "helloworld.cpp")) {
                options.GenerateTestFile = ConfigOptions::GenTestType::Never;
            } else {
                options.GenerateTestFile = ConfigOptions::GenTestType::Always;
            }
        }
        std::optional<std::string> testFilename;
        if (options.GenerateTestFile == ConfigOptions::GenTestType::Always) {
            testFilename = generateTestFile();
        }
        if (options.OpenVscodeAfterConfig) {
            openVscode(testFilename);
        }
        if (options.GenerateDesktopShortcut) {
            generateShortcut();
        }
        if (!options.NoSendAnalytics) {
            sendAnalytics();
        }
        LOG_INF("配置完成。");
    } catch (std::exception& e) {
        LOG_ERR(e.what());
        throw;
    }
}
