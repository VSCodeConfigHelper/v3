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

#include "log.h"

#include <fcntl.h>
#include <io.h>

#include <boost/locale/generator.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup.hpp>
#include <iostream>

namespace Log {

namespace logging = boost::log;
namespace trivial = boost::log::trivial;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

namespace {

constexpr const char* const levelText[]{"TRC", "DBG", "INF", "WRN", "ERR", "FTL"};

void consoleFormatter(const logging::record_view& rec, logging::formatting_ostream& strm) {
    auto level{rec[trivial::severity]};
    auto date_time_formatter{
        expr::stream << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S")};

    strm << "[";
    date_time_formatter(rec, strm);
    if (level) {
        strm << " " << levelText[level.get()];
    }
    strm << "] ";
    strm << rec[expr::smessage];
}

void fileFormatter(const logging::record_view& rec, logging::formatting_ostream& strm) {
    auto level{rec[trivial::severity]};
    auto date_time_formatter{expr::stream << expr::format_date_time<boost::posix_time::ptime>(
                                 "TimeStamp", "%Y-%m-%d %H:%M:%S.%f")};
    date_time_formatter(rec, strm);
    strm << " [" << levelText[level.get()] << "] " << rec[expr::smessage];
}

}  // namespace

src::severity_logger<trivial::severity_level> logger{};

void init(bool verbose) {
    logging::add_common_attributes();

    auto fileSink{boost::log::add_file_log("vscch.log")};
    // std::locale loc = boost::locale::generator()("en_US.UTF-8");
    // fileSink->imbue(loc);
    fileSink->set_formatter(&fileFormatter);

    boost::shared_ptr<sinks::synchronous_sink<sinks::text_ostream_backend>> consoleSink{
        logging::add_console_log(std::cout)};

    consoleSink->set_formatter(&consoleFormatter);
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    if (verbose) {
        consoleSink->set_filter(trivial::severity >= trivial::info);
    } else {
        consoleSink->set_filter(trivial::severity >= trivial::warning);
    }
}

}  // namespace Log