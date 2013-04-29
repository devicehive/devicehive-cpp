/** @file
@brief The logging unit test.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#include <hive/log.hpp>
#include <examples/logger_cfg.hpp>

#define HIVELOG_DISABLE_DEBUG
#include <hive/log.hpp>

#undef HIVELOG_DISABLE_DEBUG
#undef HIVELOG_DISABLE_TRACE
#include <hive/log.hpp>

namespace
{

/// @brief Checks for log macroses.
/**
Writes the log messages to `std::cout`
and to the log file with a few backups.
*/
void test_log_0()
{
    using namespace hive::log;

    std::cout << "check log macroses...\n";
    Logger logger("/test/hive/log");

    Target::SharedPtr test_target = Target::Stream::createStdout();
    test_target->setFormat(Format::create("%L:\t%N\t%M\n"));
    test_target->setMinimumLevel(LEVEL_DEBUG);

    Target::File::SharedPtr file_target = Target::File::create("test.log");
    file_target->setMaxFileSize(128).setNumberOfBackups(5);
    file_target->setFormat(Format::create("%L: %M\n"));

    logger.setTarget(Target::Tie::create(test_target, file_target));
    logger.setLevel(LEVEL_TRACE);

    HIVELOG_FATAL(logger, "fatal (2*2=" << (2*2) << ")");
    HIVELOG_FATAL_STR(logger, "fatal string");

    HIVELOG_ERROR(logger, "error (2*2=" << (2*2) << ")");
    HIVELOG_ERROR_STR(logger, "error string");

    HIVELOG_WARN(logger, "warning (2*2=" << (2*2) << ")");
    HIVELOG_WARN_STR(logger, "warning string");

    HIVELOG_INFO(logger, "info (2*2=" << (2*2) << ")");
    {
        HIVELOG_INFO_BLOCK(logger, "info block");
        HIVELOG_INFO_STR(logger, "info string");
    }

    HIVELOG_DEBUG(logger, "debug (2*2=" << (2*2) << ")");
    {
        HIVELOG_DEBUG_BLOCK(logger, "debug block");
        HIVELOG_DEBUG_STR(logger, "debug string");
    }

    HIVELOG_TRACE(logger, "trace (2*2=" << (2*2) << ")");
    {
        HIVELOG_TRACE_BLOCK(logger, "trace block");
        HIVELOG_TRACE_STR(logger, "trace string");
    }

    std::cout << "done\n";
}


/// @brief Checks for log configuration.
/**
Reads the log configuration from JSON.
*/
void test_log_1()
{
    hive::json::Value jcfg;
    jcfg["targets"]["myfile"]["type"] = "file";
    jcfg["targets"]["myfile"]["fileName"] = "test.log";
    jcfg["loggers"]["/"]["level"] = "TRACE";
    jcfg["loggers"]["/"]["targets"].append("console");
    jcfg["loggers"]["/"]["targets"].append("myfile");
    jcfg["loggers"]["/API"]["level"] = "TRACE";
    jcfg["loggers"]["/API"]["targets"].append("myfile");

    std::cout << toStrH(jcfg) << "\n";
    logger_cfg::apply(jcfg);
}

} // local namespace
