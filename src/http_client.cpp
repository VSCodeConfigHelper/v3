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

// Functions which will download file from the Internet

#include <httplib.h>

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <chrono>

#include "config.h"
#include "generator.h"
#include "log.h"
#include "native.h"

using namespace std::literals;
using namespace boost::assign;

namespace {
std::optional<std::string> download(const char* host, const char* path, const char* savePath = nullptr) {
#ifdef WINDOWS
    httplib::Client client(host);
    client.set_connection_timeout(5000ms);
    auto res{client.Get(path)};
    if (res && res->status == 200) {
        return res->body;
    } else {
        return std::nullopt;
    }
    if (savePath) {
        boost::filesystem::path p(savePath);
        boost::filesystem::save_string_file(p, res->body);
    }
#else
    namespace bp = boost::process;
    auto curlPath{bp::search_path("curl")};
    if (curlPath.empty()) {
        LOG_ERR("需要 Curl 才能下载文件。当前系统未找到 Curl 安装。");
        return std::nullopt;
    }
    bp::ipstream is;
    std::vector args{std::string(host) + path, "--connect-timeout"s, "5"s};
    if (savePath) {
        args += "--output"s, savePath;
    }
    bp::child curl(curlPath, args, bp::std_out > is, bp::std_err > bp::null);
    std::stringstream ss;
    std::string line;
    while (curl.running() && std::getline(is, line) && !line.empty()) {
        ss << line << '\n';
    }
    curl.wait();
    if (curl.exit_code() != 0) {
        return std::nullopt;
    } else {
        return ss.str();
    }
#endif
}

}  // namespace

namespace Cli {
    
void checkUpdate() {
    auto data{download("https://api.github.com",
                       "/repos/Guyutongxue/VSCodeConfigHelper3/releases/latest")};
    if (!data) {
        LOG_ERR("无法获取最新版本信息。");
        return;
    }
    try {
        const auto json{nlohmann::json::parse(data.value())};
        const auto tag{json["tag_name"]};
        const auto latestVersion{tag.get<std::string>().substr(1)};
        std::cout << "最新版本 v" << latestVersion << "，当前版本 v" << PROJECT_VERSION << "：";
        std::vector<std::string> latestVersionNumber, currentVersionNumber;
        boost::split(latestVersionNumber, latestVersion, boost::is_any_of("."));
        boost::split(currentVersionNumber, PROJECT_VERSION, boost::is_any_of("."));
        if (std::lexicographical_compare(currentVersionNumber.begin(), currentVersionNumber.end(),
                                         latestVersionNumber.begin(), latestVersionNumber.end())) {
            std::cout << "有更新可用。请前往 https://vscch3.vercel.app 获取。" << std::endl;
        } else {
            std::cout << "已是最新版本。" << std::endl;
        }
        LOG_DBG("Latest: ", latestVersion, " Current: ", PROJECT_VERSION);
    } catch (...) {
        LOG_ERR("检查更新时发生错误。");
    }
}

}  // namespace Cli

void ExtensionManager::installOffline(const std::string& id, const char* host, const char* path) {
    if (installedExtensions.find(id) != installedExtensions.end()) {
        LOG_INF("扩展 ", id, " 已安装。");
        return;
    }
    LOG_INF("从 ", host, path, " 下载扩展 ", id, "...");
    LOG_WRN("由于启用了离线扩展安装，工具需要一段时间下载扩展包。请耐心等待...");
    auto vsixPath{Native::getTempFilePath("cpptools.vsix")};
    auto data{download(host, path, vsixPath.c_str())};
    if (data) {
        LOG_INF("下载完成。");
    } else {
        LOG_WRN("离线扩展包下载失败。将使用在线安装。");
        install(id);
        return;
    }
    try {
        LOG_INF("尝试安装...");
        LOG_DBG(runScript({"--install-extension", vsixPath.string()}));
        installedExtensions.insert(id);
        LOG_INF("安装完成。");
    } catch (...) {
        LOG_ERR("从 ", vsixPath, " 安装扩展时发生错误。");
        throw;
    }
}

void Generator::sendAnalytics() {
    auto data{download("https://api.countapi.xyz",
                       "/hit/guyutongxue.github.io/b54f2252-e54a-4bd0-b4c2-33b47db6aa98")};
    if (data) {
        LOG_INF("统计数据发送成功。");
    } else {
        LOG_WRN("统计数据发送失败。");
    }
}
