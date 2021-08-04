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
    for (auto&& path : getPaths()) {
        auto versionText{testCompiler(path)};
        if (versionText) {
            compilers.push_back(CompilerInfo(path, *versionText));
        }
    }
    LOG_INF("解析环境完成。");
    LOG_DBG("Resolved vscode path: ", vscodePath ? *vscodePath : "null");
    LOG_DBG("Resolved compilers: ");
    for (auto&& compiler : compilers) {
        LOG_DBG("  ", compiler.Path, ": ", compiler.VersionText);
    }
}

std::optional<std::string> Environment::getVscodePath() {
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
    return std::nullopt;
}

std::unordered_set<std::string> Environment::getPaths() {
    LOG_INF("获取环境变量 Path 中的值...");

    auto usrPath{Native::getCurrentUserEnv("Path")};
    auto sysPath{Native::getLocalMachineEnv("Path")};
    LOG_DBG("User Path: ", usrPath ? *usrPath : "");
    LOG_DBG("System Path: ", sysPath ? *sysPath : "");

    std::ostringstream allPath;
    if (usrPath) allPath << *usrPath;
    allPath << ';';
    if (sysPath) allPath << *sysPath;

    // https://stackoverflow.com/a/47923278
    boost::regex pathSplitter(";(?=(?:[^\"]*\"[^\"]*\")*(?![^\"]*\"))");
    std::unordered_set<std::string> result;
    boost::split_regex(result, allPath.str(), pathSplitter);
    return result;
}

const std::optional<std::string>& Environment::VscodePath() const {
    return vscodePath;
}

const std::vector<CompilerInfo>& Environment::Compilers() const {
    return compilers;
}

std::optional<std::string> Environment::testCompiler(const boost::filesystem::path& path) {
    bp::ipstream is;
    auto compilerPath{path / "g++.exe"};
    if (!fs::exists(compilerPath)) {
        return std::nullopt;
    }
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
