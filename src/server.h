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

    int Port() const {
        return port;
    }
    void startListen();

    static void runGui(const Environment& env);
};