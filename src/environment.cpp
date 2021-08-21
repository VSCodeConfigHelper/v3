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

#include "environment.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <sstream>

#include "log.h"
#include "native.h"

namespace bp = boost::process;
namespace fs = boost::filesystem;

CompilerInfo::CompilerInfo(const std::string& path, const std::string& versionText)
    : Path(path), VersionText(versionText) {
    auto firstSpace{versionText.find_first_of(' ')};
    auto lastSpace{versionText.find_last_of(' ')};
    VersionNumber = versionText.substr(lastSpace + 1);
    PackageString = versionText.substr(firstSpace + 2, lastSpace - firstSpace - 3);
}

Environment::Environment() {
    LOG_INF("解析环境中...");
    vscodePath = getVscodePath();
#ifdef WINDOWS
    for (auto&& path : getPaths()) {
        auto versionText{testCompiler(path)};
        if (versionText) {
            compilers.emplace_back(path, *versionText);
        }
    }
#else
    LOG_INF("查找 C++ 编译器...");
    auto fullPath{bp::search_path(Native::cppCompiler).string()};
    if (!fullPath.empty()) {
        auto versionText{testCompiler(fullPath)};
        if (versionText) {
            CompilerInfo info(fullPath, *versionText);
            info.type = LanguageType::Cpp;
            LOG_DBG("C++ compiler info: ", info.Path, ": ", info.VersionText);
            compilers.push_back(info);
        }
    }
    LOG_INF("查找 C 编译器...");
    fullPath = bp::search_path(Native::cCompiler).string();
    if (!fullPath.empty()) {
        auto versionText{testCompiler(fullPath)};
        if (versionText) {
            CompilerInfo info(fullPath, *versionText);
            info.type = LanguageType::C;
            LOG_DBG("C compiler info: ", info.Path, ": ", info.VersionText);
            compilers.push_back(info);
        }
    }
#endif
    LOG_INF("解析环境完成。");
    LOG_DBG("Resolved vscode path: ", vscodePath ? *vscodePath : "null");
    LOG_DBG("Resolved compilers: ");
    for (auto&& compiler : compilers) {
        LOG_DBG("  ", compiler.Path, ": ", compiler.VersionText);
    }
}

std::optional<std::string> Environment::getVscodePath() {
#ifdef WINDOWS
    LOG_INF("从注册表中尝试获取 VS Code 路径...");
    auto result{Native::getRegistry(HKEY_CLASSES_ROOT, "vscode\\shell\\open\\command", "")};
    if (!result) return std::nullopt;
    // The value should be like:
    // "C:\Program Files\Microsoft VS Code\Code.exe" --open-url -- "%1"
    // and we just use the string inside the first quotation marks
    std::vector<std::string> parts;
    boost::split(parts, *result, boost::is_any_of("\""));
    if (parts.size() < 2) return std::nullopt;
    const std::string& vscodePath = parts.at(1);
    if (fs::exists(vscodePath)) {
        LOG_DBG("Found VS Code path: ", vscodePath);
        return vscodePath;
    }
#else
    LOG_INF("检测 VS Code 是否安装...");
    auto path{bp::search_path("code").string()};
    LOG_DBG("VS Code Path: ", path);
    if (!path.empty()) return path;
#endif
    return std::nullopt;
}

#ifdef WINDOWS
std::unordered_set<std::string> Environment::getPaths() {
    LOG_INF("获取环境变量 Path 中的值...");
    std::ostringstream allPath;
    auto usrPath{Native::getCurrentUserEnv("Path")};
    auto sysPath{Native::getLocalMachineEnv("Path")};
    LOG_DBG("User Path: ", usrPath ? *usrPath : "");
    LOG_DBG("System Path: ", sysPath ? *sysPath : "");

    if (usrPath) allPath << *usrPath;
    allPath << ';';
    if (sysPath) allPath << *sysPath;

    // https://stackoverflow.com/a/47923278
    boost::regex pathSplitter(";(?=(?:[^\"]*\"[^\"]*\")*(?![^\"]*\"))");
    std::unordered_set<std::string> result;
    boost::split_regex(result, allPath.str(), pathSplitter);
    return result;
}
#endif

std::optional<std::string> Environment::testCompiler(const boost::filesystem::path& path) {
    bp::ipstream is;
#ifdef WINDOWS
    auto compilerPath{path / "g++.exe"};
    if (!fs::exists(compilerPath)) {
        return std::nullopt;
    }
#else
    const auto& compilerPath{path};
#endif
    try {
        bp::child proc(compilerPath, "--version", bp::std_out > is);
        std::string versionText;
        if (std::getline(is, versionText) && !versionText.empty()) {
            proc.wait();
            return boost::trim(versionText), versionText;
        } else {
            return std::nullopt;
        }
    } catch (bp::process_error& err) {
        LOG_WRN("测试编译器 ", compilerPath.native(), " 时失败：", err.what());
        return std::nullopt;
    }
}