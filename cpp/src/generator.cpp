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

std::string ExtensionManager::runScript(const std::string& args) {
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
        auto output{runScript("--list-extensions")};
        boost::trim(output), boost::to_lower(output);
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
        LOG_INF("扩展 ", id, " 已安装");
        return;
    }
    try {
        LOG_INF("尝试安装 ", id, "...");
        runScript("--installed-extension " + id);
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
            runScript("--uninstall-extension " + id);
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

Generator::Generator(const ConfigOptions& options) : options{options} {}

const char* Generator::compilerExe() {
    return options.Language == ConfigOptions::LanguageType::Cpp ? "g++.exe" : "gcc.exe";
}
const char* Generator::fileExt() {
    return options.Language == ConfigOptions::LanguageType::Cpp ? ".cpp" : ".c";
}

void Generator::saveFile(const fs::path& path, const char* content) {
    LOG_INF("写入脚本 ", path.string(), " 中...");
    if (fs::exists(path)) {
        LOG_INF("脚本 ", path.string(), " 已存在，无需写入。");
    } else {
        fs::save_string_file(path, content);
        LOG_INF("写入完成。");
    }
}

void Generator::addKeybinding(const std::string& key, const std::string& command,
                              const std::string& args) {
    using json = nlohmann::json;
    auto filepath(fs::path(Native::getAppdata()) / "Code\\User\\keybindings.json");
    LOG_INF("将快捷键 ", key, " (", command, " ", args, ") 添加到 ", filepath.string(), " 中...");
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
    LOG_INF("将 ", newPath, " 添加到环境变量 Path 中...");
    auto origin{Native::getCurrentUserEnv("Path")};
    auto originalPath{origin ? *origin : ""};
    std::list<std::string> paths;
    boost::regex pathSplitter(";(?=(?:[^\"]*\"[^\"]*\")*(?![^\"]*\"))");
    boost::split_regex(paths, originalPath, pathSplitter);
    if (auto it{std::find(paths.begin(), paths.end(), newPath)}; it != paths.end()) {
        LOG_INF("环境变量 Path 已经包含 ", newPath, "。将其移动到最前。");
        paths.erase(it);
        paths.push_front(newPath);
    } else {
        LOG_INF("环境变量 Path 中未包含 ", newPath, "。将其添加。");
        paths.push_front(newPath);
    }
    auto result{boost::join(paths, ";")};
    LOG_DBG("Final Path: ", result);
    Native::setCurrentUserEnv("Path", result);
}

void Generator::generateTasksJson(const fs::path& path) {
    using json = nlohmann::json;
    LOG_INF("生成 ", path.string(), " ...");
    auto args{options.CompileArgs};
    args += "-g", "${file}", "-o", "${fileDirname}\\${fileBasenameNoExtension}.exe";
    // clang-format off
    auto sfbTask(json::object({
        {"type", "cppbuild"},
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
    LOG_INF("生成 ", path.string(), " ...");
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
    LOG_INF("生成 ", path.string(), " ...");
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

std::string Generator::generateTestFile() {}
void Generator::openVscode(const std::optional<std::string>& filepath) {}
void Generator::generateShortcut() {}
void Generator::sendAnalytics() {}

void Generator::generate() {
    try {
        fs::path mingwPath(options.MingwPath);
        fs::path dotVscode(fs::path(options.WorkspacePath) / ".vscode");

        ExtensionManager extensions(options.VscodePath);
        if (options.ShouldUninstallExtensions) {
            extensions.uninstallAll();
        }
        extensions.install("ms-vscode.cpptools");
        if (options.ShouldInstallL11n) {
            extensions.install("ms-ceintl.vscode-language-pack-zh-hans");
        }

        if (options.UseExternalTerminal) {
            saveFile(mingwPath / "pause-console.ps1", Embed::PAUSE_CONSOLE_PS1);
            addKeybinding("f6", "workbench.action.tasks.runTask", "run and pause");
        }
        if (options.ApplyNonAsciiCheck) {
            saveFile(mingwPath / "check-ascii.ps1", Embed::CHECK_ASCII_PS1);
        }

        if (fs::exists(dotVscode)) {
            fs::remove_all(dotVscode);
            LOG_INF("移除了已存在的 .vscode 文件夹。");
        }
        fs::create_directories(dotVscode);
        generateTasksJson(dotVscode / "tasks.json");
        generateLaunchJson(dotVscode / "launch.json");
        generatePropertiesJson(dotVscode / "c_cpp_properties.json");

        if (!options.NoSetEnv) {
            addToPath(mingwPath);
        }
    } catch (std::exception& e) {
        LOG_ERR(e.what());
        throw;
    }
}
