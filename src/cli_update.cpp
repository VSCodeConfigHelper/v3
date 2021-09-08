

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <nlohmann/json.hpp>
#include <sstream>

#include "config.h"
#include "log.h"

using namespace std::literals;

namespace Cli {

// cpp-httplib cannot get github API. I don't know why.
void checkUpdate() {
    namespace bp = boost::process;
    auto curlPath{bp::search_path("curl")};
    if (curlPath.empty()) {
        LOG_ERR("由于 GitHub 限制，需要 Curl 才能检查更新。当前系统未找到 Curl 安装。");
        return;
    }
    bp::ipstream is;
    bp::child curl(curlPath,
                   "https://api.github.com/repos/Guyutongxue/VSCodeConfigHelper3/releases/latest",
                   bp::std_out > is, bp::std_err > bp::null);
    std::stringstream ss;
    std::string line;
    while (curl.running() && std::getline(is, line) && !line.empty()) {
        ss << line << '\n';
    }
    try {
        const auto json{nlohmann::json::parse(ss.str())};
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