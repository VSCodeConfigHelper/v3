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

#include "server.h"

#include <httplib.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <iostream>
#include <iterator>
#include <sstream>

#include "cli.h"
#include "generator.h"
#include "log.h"
#include "native.h"

#ifdef WINDOWS

namespace fs = boost::filesystem;
using namespace std::literals;

Server::Server(const Environment& env) : env(env) {
    setHandlers();
    bind();
}

constexpr const char PROFILE_JSON_PATH[]{"profile.json"};
bool shouldShutdownServer{false};

void Server::setHandlers() {
    // clang-format off
    handlers.insert({"/getEnv", [&](const std::string& _) {
        nlohmann::json j(env.get());
        return j.dump();
    }});
    handlers.insert({"/getFolder", [](const std::string& initDir) {
        auto result{Native::browseFolder(initDir)};
        return result ? *result : "";
    }});
    handlers.insert({"/verifyVscode", [](const std::string& p) {
        fs::path path(p);
        if (boost::algorithm::to_lower_copy(path.filename().string()) == "bin") {
            path = path.parent_path();
        }
        if (fs::exists(path / "Code.exe")) {
            return "valid";
        } else {
            return "invalid";
        }
    }});
    handlers.insert({"/verifyCompiler", [](const std::string& p) {
        if (std::find(p.begin(), p.end(), ';') != p.end()) {
            return R"({ "valid": false, "reason": "semicolon" })"s;
        } 
        fs::path path(p);
        if (boost::algorithm::to_lower_copy(path.filename().string()) != "bin") {
            path = path / "bin";
        }
        auto versionText{Environment::testCompiler(path)};
        if (versionText) {
            CompilerInfo info(path.string(), *versionText);
            return nlohmann::json({
                { "valid", true },
                { "info", info }
            }).dump();
        } else {
            return R"({ "valid": false, "reason": "not_found" })"s;
        }
    }});
    handlers.insert({"/verifyWorkspace", [](const std::string& p) {
        return fs::exists(fs::path(p) / ".vscode") ? "warn" : "ok";
    }});
    handlers.insert({"/saveProfile", [](const std::string& profile) {
        fs::save_string_file(PROFILE_JSON_PATH, profile);
        return "ok";
    }});
    handlers.insert({"/loadProfile", [](const std::string& _){
        if (!fs::exists(PROFILE_JSON_PATH)) return "null"s;
        try {
            std::string profile;
            fs::load_string_file(PROFILE_JSON_PATH, profile);
            return profile;
        } catch (...) {
            return "null"s;
        }
    }});
    handlers.insert({"/done", [](const std::string& body) {
        // brace-init causes bug, see https://github.com/nlohmann/json/issues/2311
        auto j(nlohmann::json::parse(body));
        if (j.at("success").get<bool>()) {
            auto config(j.at("config"));
            auto options{config.get<WindowsOptions>()};
            Generator g(options);
            g.generate();
        }
        return "ok"s;
    }});
    // clang-format on
    for (auto&& [path, handler] : handlers) {
        server.Post(path.c_str(), [&](const Request& req, Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            try {
                LOG_DBG("Path:", path);
                LOG_DBG("Req : ", req.body);
                auto resBody{handler(req.body)};
                LOG_DBG("Res : ", resBody);
                res.set_content(resBody, "text/plain");
            } catch (std::exception& e) {
                LOG_ERR(e.what());
                res.set_content("\"Internal error: "s + e.what() + "\"", "text/plain");
            }
            if (path == "/done") shouldShutdownServer = true;
        });
    }

    // server.Post("/hello", [](const Request& req, Response& res) {
    //     std::string name = req.body;
    //     res.set_content("Hello, " + name + "!", "text/plain");
    // });

    server.Get("/.*", [](const Request& req, Response& res) {
        res.set_content("Hello from cpp-httplib", "text/plain");
    });
}

int Server::bind() {
    if (server.bind_to_port("localhost", port = 8000)) {
        return port;
    } else {
        return port = server.bind_to_any_port("localhost");
    }
}

void Server::startListen() {
    std::thread t([&]() { server.listen_after_bind(); });
    while (!shouldShutdownServer) {
        std::this_thread::sleep_for(200ms);
    }
    std::this_thread::sleep_for(200ms);
    server.stop();
    t.join();
}

void Server::runGui(const Environment& env) {
    Server s(env);
    LOG_INF("本地服务器已启动，即将开始监听 ", s.port, " 端口...");
    auto openAddress{Cli::options.GuiAddress + "?port=" + std::to_string(s.port)};
    if (!Cli::options.NoOpenBrowser) {
        LOG_WRN("已打开网页 ", openAddress, "，请在浏览器中继续操作。请不要关闭此窗口。");
        std::system(("START " + openAddress).c_str());
    } else {
        LOG_WRN("--no-open-browser 选项禁用了浏览器启动。请与 localhost:", s.port,
                " 端口通信完成配置。");
    }
    s.startListen();
    return;
    LOG_ERR("仅 Windows 支持 GUI。程序将退出。");
    std::exit(1);
}

#endif