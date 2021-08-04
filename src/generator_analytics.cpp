

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