#include "server.h"

#include <httplib.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <iterator>
#include <sstream>

#include "generator.h"
#include "native.h"

namespace fs = boost::filesystem;
using namespace std::literals;

Server::Server(const Environment& env) : env(env) {
    setHandlers();
    bind();
}

constexpr const char PROFILE_JSON_PATH[]{"profile.json"};

void Server::setHandlers() {
    // clang-format off
    handlers.insert({"/getEnv", [&](const std::string& _) {
        nlohmann::json j(env.get());
        return j.dump();
    }});
    handlers.insert({"/getFolder", [](const std::string& initDir) {
        auto result{Native::browseFolder(initDir)};
        return result ? result.value() : "";
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
        fs::path path(p);
        if (boost::algorithm::to_lower_copy(path.filename().string()) != "bin") {
            path = path / "bin";
        }
        auto versionText{Environment::testCompiler(path)};
        if (versionText.has_value()) {
            CompilerInfo info(path.string(), versionText.value());
            return nlohmann::json({
                { "valid", true },
                { "info", info }
            }).dump();
        } else {
            return R"({ "valid": false })"s;
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
        try {
            if (j.at("success").get<bool>()) {
                auto config(j.at("config"));
                auto options{config.get<ConfigOptions>()};
                Generator g(options);
                g.generate();
            }
            return "ok"s;
        } catch (...) {
            return "error"s;
        }
    }});
    // clang-format on

    for (auto&& [path, handler] : handlers) {
        server.Post(path.c_str(), [&](const Request& req, Response& res) {
            auto resBody{handler(req.body)};
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(resBody, "text/plain");
            if (path == "/done") server.stop();
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
    server.bind_to_port("localhost", port = 8000);
    return port;
    // return port = server.bind_to_any_port("localhost");
}

int Server::Port() const {
    return port;
}

void Server::startListen() {
    server.listen_after_bind();
}