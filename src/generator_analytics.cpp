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

#include "generator.h"
#include "log.h"

using namespace std::literals;

void Generator::sendAnalytics() {
    httplib::Client client("https://api.countapi.xyz");
    client.set_connection_timeout(5000ms);
    auto res{client.Get("/hit/guyutongxue.github.io/b54f2252-e54a-4bd0-b4c2-33b47db6aa98")};
    if (res && res->status == 200) {
        LOG_INF("统计数据发送成功。");
    } else {
        LOG_WRN("统计数据发送失败。");
    }
}