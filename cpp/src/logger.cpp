
#include "logger.h"

#include <fcntl.h>
#include <io.h>

#include <boost/locale/generator.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup.hpp>

namespace Log {

namespace logging = boost::log;
namespace trivial = boost::log::trivial;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

namespace {


const char* const levelText[]{"TRC", "DBG", "INF", "WRN", "ERR", "FTL"};

void consoleFormatter(const logging::record_view& rec, logging::wformatting_ostream& strm) {
    auto level{rec[trivial::severity]};
    auto date_time_formatter{
        expr::stream << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", L"%H:%M:%S")};

    strm << "[";
    date_time_formatter(rec, strm);
    if (level) {
        strm << " " << levelText[level.get()];
    }
    strm << "] ";
    strm << rec[expr::wmessage];
}

void fileFormatter(const logging::record_view& rec, logging::formatting_ostream& strm) {
    auto level{rec[trivial::severity]};
    auto date_time_formatter{expr::stream << expr::format_date_time<boost::posix_time::ptime>(
                                 "TimeStamp", "%Y-%m-%d %H:%M:%S.%f")};
    strm << "[";
    date_time_formatter(rec, strm);
    strm << " " << levelText[level.get()] << "] " << rec[expr::wmessage];
}

}  // namespace

src::wseverity_logger<trivial::severity_level> logger{};

void init(bool verbose) {
    logging::add_common_attributes();

    auto fileSink{boost::log::add_file_log("vscch.log")};
    std::locale loc = boost::locale::generator()("en_US.UTF-8");
    fileSink->imbue(loc);
    fileSink->set_formatter(&fileFormatter);

    boost::shared_ptr<sinks::synchronous_sink<sinks::wtext_ostream_backend>> consoleSink{
        logging::add_console_log(std::wcout)};

    consoleSink->set_formatter(&consoleFormatter);
    _setmode(_fileno(stdout), _O_WTEXT);

    if (verbose) {
        consoleSink->set_filter(trivial::severity >= trivial::info);
    } else {
        consoleSink->set_filter(trivial::severity >= trivial::warning);
    }
}

}  // namespace Log