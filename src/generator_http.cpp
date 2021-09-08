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


// Separate Generator::sendAnalytics from generator.cpp because it needs <httplib.h>. <httplib.h>
// introduces too many symbols while MinGW assembler cannot handle them all.

#include <httplib.h>

#include <chrono>
#include <boost/filesystem.hpp>

#include "generator.h"
#include "log.h"
#include "native.h"

using namespace std::literals;

void ExtensionManager::installOffline(const std::string& id, const char* host, const char* path) {
    if (installedExtensions.find(id) != installedExtensions.end()) {
        LOG_INF("扩展 ", id, " 已安装。");
        return;
    }
    LOG_INF("从 ", host, path, " 下载扩展 ", id, "...");
    LOG_WRN("由于启用了离线扩展安装，工具需要一段时间下载扩展包。请耐心等待...");
    httplib::Client client(host);
    auto res{client.Get(path)};
    if (res && res->status == 200) {
        LOG_INF("下载完成。");
    } else {
        LOG_WRN("离线扩展包下载失败。将使用在线安装。");
        install(id);
        return;
    }
    auto vsixPath{Native::getTempFilePath("cpptools.vsix")};
    boost::filesystem::save_string_file(vsixPath, res->body);
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
    httplib::Client client("https://api.countapi.xyz");
    client.set_connection_timeout(5000ms);
    auto res{client.Get("/hit/guyutongxue.github.io/b54f2252-e54a-4bd0-b4c2-33b47db6aa98")};
    if (res && res->status == 200) {
        LOG_INF("统计数据发送成功。");
    } else {
        LOG_WRN("统计数据发送失败。");
        LOG_DBG(res.error(), " ", res ? res->status : -1);
    }
}
