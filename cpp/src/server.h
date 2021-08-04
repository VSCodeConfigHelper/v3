#pragma once

#include <httplib.h>

#include <functional>
#include <unordered_map>

#include "environment.h"

class Server {
    using HttpServer = httplib::Server;
    using Request = httplib::Request;
    using Response = httplib::Response;

    HttpServer server;
    int port;

    std::reference_wrapper<const Environment> env;

    std::unordered_map<std::string, std::function<std::string(const std::string&)>> handlers;
    void setHandlers();
    int bind();

public:
    Server(const Environment& env);

    int Port() const;
    void startListen();

    static void runGui(const Environment& env);
};