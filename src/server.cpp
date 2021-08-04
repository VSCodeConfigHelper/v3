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
        try { 
            fs::save_string_file(PROFILE_JSON_PATH, profile);
        } catch (...) {}
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
        try {
            if (j.at("success").get<bool>()) {
                auto config(j.at("config"));
                auto options{config.get<ConfigOptions>()};
                Generator g(options);
                g.generate();
            }
            return "ok"s;
        } catch (std::exception& e) {
            LOG_ERR(e.what());
            return "error"s;
        }
    }});
    // clang-format on
    for (auto&& [path, handler] : handlers) {
        server.Post(path.c_str(), [&](const Request& req, Response& res) {
            LOG_DBG("Req body: ", req.body);
            auto resBody{handler(req.body)};
            LOG_DBG("Res body: ", resBody);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(resBody, "text/plain");
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

int Server::Port() const {
    return port;
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
        std::system(("START " + openAddress).c_str());
    }
    s.startListen();
}