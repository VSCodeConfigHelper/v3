#include "env_resolver.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <sstream>

#include "logger.h"
#include "native.h"

namespace bp = boost::process;
namespace fs = boost::filesystem;

CompilerInfo::CompilerInfo(const std::string& path, const std::string& versionText)
    : path(path), versionText(versionText) {
    std::vector<std::string> parts;
    boost::split(parts, versionText, boost::is_any_of(" "));
    versionNumber = parts.back();
    if (parts.size() > 1) {
        std::string pkgStrRaw = parts.at(parts.size() - 2);
        packageString = pkgStrRaw.substr(1, pkgStrRaw.size() - 2);
    }
}

EnvResolver::EnvResolver() {
    LOG_INF("解析环境中...");
    vscodePath = getVscodePath();
    for (auto&& path : getPaths()) {
        auto versionText{testCompiler(path)};
        if (versionText) {
            compilers.push_back(CompilerInfo(path, versionText.value()));
        }
    }
    LOG_INF("解析环境完成。");
    LOG_DBG("Resolved vscode path: ", vscodePath ? vscodePath.value() : "");
    LOG_DBG("Resolved compilers: ");
    for (auto&& compiler : compilers) {
        LOG_DBG("  ", compiler.path, ": ", compiler.versionText);
    }
}

std::optional<std::string> EnvResolver::getVscodePath() {
    LOG_INF("从注册表中尝试获取 VS Code 路径...");
    auto result{Native::getRegistry(HKEY_CLASSES_ROOT, "vscode\\shell\\open\\command", "")};
    if (result.has_value() && fs::exists(result.value())) {
        LOG_DBG("Found VS Code path: ", result.value());
        return result;
    }
    return std::nullopt;
}

std::unordered_set<std::string> EnvResolver::getPaths() {
    LOG_INF("获取环境变量 Path 中的值...");

    constexpr const char* const USR_ENV_REG{"Environment"};
    constexpr const char* const SYS_ENV_REG{
        "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"};
    auto usrPath{Native::getRegistry(HKEY_CURRENT_USER, USR_ENV_REG, "Path")};
    auto sysPath{Native::getRegistry(HKEY_LOCAL_MACHINE, SYS_ENV_REG, "Path")};
    LOG_DBG("User Path: ", usrPath ? usrPath.value() : "");
    LOG_DBG("System Path: ", sysPath ? sysPath.value() : "");

    std::ostringstream allPath;
    if (usrPath.has_value()) allPath << usrPath.value();
    allPath << ';';
    if (sysPath.has_value()) allPath << sysPath.value();

    boost::regex pathSplitter(";(?=(?:[^\"]*\"[^\"]*\")*(?![^\"]*\"))");
    std::unordered_set<std::string> result;
    boost::split_regex(result, allPath.str(), pathSplitter);
    return result;
}

std::optional<std::string> EnvResolver::testCompiler(const std::string& path) {
    bp::ipstream is;
    auto compilerPath{fs::path(path) / "g++.exe"};
    if (!fs::exists(compilerPath)) {
        return std::nullopt;
    }
    try {
    bp::child proc(compilerPath, "--version", bp::std_out > is);
    std::string versionText;
    if (std::getline(is, versionText) && !versionText.empty()) {
        proc.wait();
        return versionText;
    } else {
        return std::nullopt;
    }
    } catch (bp::process_error& err) {
        LOG_WRN("测试编译器 ", compilerPath.native(), " 时失败：", err.what());
        return std::nullopt;
    }
}