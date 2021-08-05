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

#include <windows.h>

#include <boost/log/sources/logger.hpp>
#include <boost/log/trivial.hpp>

namespace Log {

extern boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;
void init(bool verbose);

template <typename... Ts>
void log(boost::log::trivial::severity_level level, const Ts&... content) {
    using namespace boost::log;
    WORD color{0x0F};
    switch (level) {
        case trivial::trace: color = 0x08; break;
        case trivial::debug: color = 0x07; break;
        case trivial::info: color = 0x0F; break;
        case trivial::warning: color = 0x0E; break;
        case trivial::error: color = 0x0C; break;
        case trivial::fatal: color = 0x04; break;
    }
    auto hstdout{GetStdHandle(STD_OUTPUT_HANDLE)};
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hstdout, &csbi);
    SetConsoleTextAttribute(hstdout, color);
    record rec = logger.open_record(keywords::severity = level);
    if (rec) {
        record_ostream strm(rec);
        (strm << ... << content);
        strm.flush();
        logger.push_record(std::move(rec));
    }
    SetConsoleTextAttribute(hstdout, csbi.wAttributes);
}

}  // namespace Log

// Should use __VA_OPT__, but VS intellisense doesn't like it
#define LOG_DBG(...) Log::log(boost::log::trivial::debug, ##__VA_ARGS__)
#define LOG_INF(...) Log::log(boost::log::trivial::info, ##__VA_ARGS__)
#define LOG_WRN(...) Log::log(boost::log::trivial::warning, ##__VA_ARGS__)
#define LOG_ERR(...) Log::log(boost::log::trivial::error, ##__VA_ARGS__)
