#pragma once

#include <httplib.h>

class Server {
    using HttpServer = httplib::Server;
    using Request = httplib::Request;
    using Response = httplib::Response;

    HttpServer server;
    int port;

public:
    int getPort() const { return port; }
    int prepare() {
        server.Get("/.*", [](const Request& req, Response& res) {
            res.set_content("Hello from cpp-httplib", "text/plain");
        });
        server.Post("/hello", [](const Request& req, Response& res) {
            std::string name = req.body;
            res.set_content("Hello, " + name + "!", "text/plain");
        });
        server.Post("/done", [&](const Request& req, Response& res) { server.stop(); });
        return port = server.bind_to_any_port("localhost");
    }
    void start() {
        server.listen_after_bind();
    }
};