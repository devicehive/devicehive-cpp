/** @file
@brief The logging unit test.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#include <hive/log.hpp>
#include <hive/json.hpp>
#include <stdexcept>
#include <iostream>
#include <assert.h>

#define HIVELOG_DISABLE_DEBUG
#include <hive/log.hpp>

#undef HIVELOG_DISABLE_DEBUG
#undef HIVELOG_DISABLE_TRACE
#include <hive/log.hpp>

namespace
{
    using namespace hive;


// test target
class TestTarget:
    public log::Target
{
public:

    // factory method
    static SharedPtr create()
    {
        return SharedPtr(new TestTarget());
    }

public:

    // save log message
    virtual void send(log::Message const& msg) const
    {
        if (log::Format::SharedPtr fmt = getFormat())
            fmt->format(std::cout, msg);
        else
            log::Format::defaultFormat(std::cout, msg);
    }
};


// test format
class TestFormat:
    public log::Format
{
public:

    // factory method
    static SharedPtr create()
    {
        return SharedPtr(new TestFormat());
    }

public:

    // format the log message.
    virtual void format(OStream &os, log::Message const& msg)
    {
        //if (msg.loggerName)
        //    os << msg.loggerName << ' ';
        os << std::setw(5) << getLevelName(msg.level) << ": ";
        if (msg.prefix)
            os << msg.prefix;
        if (msg.message)
            os << msg.message;
        os << '\n';
    }
};


/*
Checks for log functions.
*/
void test_log0()
{
    std::cout << "check log macroses...\n";
    log::Logger logger("test/hive/log");
    log::Target::SharedPtr test_target = TestTarget::create();
    log::Target::File::SharedPtr file_target = log::Target::File::create("test.log");
    file_target->setMaxFileSize(128).setNumberOfBackups(5);
    logger.setTarget(log::Target::Tie::create(test_target, file_target));
    test_target->setFormat(TestFormat::create());
    logger.setLevel(log::LEVEL_TRACE);
    //log::Logger::root().setLevel(log::LEVEL_TRACE);

    HIVELOG_FATAL(logger, "fatal 2*2=" << (2*2));
    HIVELOG_FATAL_STR(logger, "fatal");

    HIVELOG_ERROR(logger, "error 2*2=" << (2*2));
    HIVELOG_ERROR_STR(logger, "error");

    HIVELOG_WARN(logger, "warning 2*2=" << (2*2));
    HIVELOG_WARN_STR(logger, "warning");

    HIVELOG_INFO(logger, "info 2*2=" << (2*2));
    {
        HIVELOG_INFO_BLOCK(logger, "info block");
        HIVELOG_INFO_STR(logger, "info string");
    }

    HIVELOG_DEBUG(logger, "debug 2*2=" << (2*2));
    {
        HIVELOG_DEBUG_BLOCK(logger, "debug block");
        HIVELOG_DEBUG_STR(logger, "debug string");
    }

    HIVELOG_TRACE(logger, "trace 2*2=" << (2*2));
    {
        HIVELOG_TRACE_BLOCK(logger, "trace block");
        HIVELOG_TRACE_STR(logger, "trace string");
    }

    std::cout << "done\n";
}


// log configuration tool
class LogConfig
{
public:

    // apply configuration
    static void apply(json::Value const& jval)
    {
        std::map<String, log::Target::SharedPtr> my_targets;
        my_targets["null"] = log::Target::create();
        my_targets["stderr"] = log::Target::Stderr::create();
        my_targets["console"] = my_targets["stderr"];

        { // parse log targets
            json::Value const& jtargets = jval["targets"];
            json::Value::MemberIterator i = jtargets.membersBegin();
            json::Value::MemberIterator e = jtargets.membersEnd();
            for (; i != e; ++i)
                my_targets[i->first] = makeTarget(i->second);
        }

        { // apply targets
            json::Value const& jloggers = jval["loggers"];
            json::Value::MemberIterator i = jloggers.membersBegin();
            json::Value::MemberIterator e = jloggers.membersEnd();
            for (; i != e; ++i)
            {
                String const& name = i->first;
                json::Value const& jlogger = i->second;

                String level_s = jlogger["level"].asString();
                json::Value const& jtargets = jlogger["targets"];
                std::vector<log::Target::SharedPtr> targets;

                {
                    json::Value::ElementIterator i = jtargets.elementsBegin();
                    json::Value::ElementIterator e = jtargets.elementsEnd();

                    for (; i != e; ++i)
                    {
                        const String tname = i->asString();
                        if (log::Target::SharedPtr t = my_targets[tname])
                            targets.push_back(t);
                        else
                        {
                            OStringStream ess;
                            ess << "\"" << tname << "\" is unknown log target name";
                            throw std::runtime_error(ess.str().c_str());
                        }
                    }
                }

                log::Logger logger(name);
                if (!level_s.empty())
                    logger.setLevel(parseLevel(level_s));
                if (!targets.empty())
                {
                    if (1 < targets.size())
                    {
                        log::Target::Tie::SharedPtr tie = log::Target::Tie::create();
                        for (size_t i = 0; i < targets.size(); ++i)
                            tie->add(targets[i]);
                        logger.setTarget(tie);
                    }
                    else
                        logger.setTarget(targets.front());
                }
            }
        }
    }

private:

    // make log target
    static log::Target::SharedPtr makeTarget(json::Value const& jval)
    {
        try
        {
            const String type = jval["type"].asString();
            if (type == "file")
            {
                const String fileName = jval["fileName"].asString();
                return log::Target::File::create(fileName);
            }

            { // unknown target
                OStringStream ess;
                ess << "\"" << type << "\" is unknown log target type";
                throw std::runtime_error(ess.str().c_str());
            }
        }
        catch (std::exception const& ex)
        {
            OStringStream ess;
            ess << "cannot create target: "
                << ex.what();
            throw std::runtime_error(ess.str().c_str());
        }
    }

    // parse the log level
    static log::Level parseLevel(String const& level)
    {
        if (level == "TRACE")
            return log::LEVEL_TRACE;
        else if (level == "DEBUG")
            return log::LEVEL_DEBUG;
        else if (level == "INFO")
            return log::LEVEL_INFO;
        else if (level == "WARN")
            return log::LEVEL_WARN;
        else if (level == "ERROR")
            return log::LEVEL_ERROR;
        else if (level == "FATAL")
            return log::LEVEL_FATAL;
        else if (level == "OFF")
            return log::LEVEL_OFF;
        else if (level == "ASPARENT")
            return log::LEVEL_AS_PARENT;

        { // unknown level
            OStringStream ess;
            ess << "\"" << level << "\" is unknown log level";
            throw std::runtime_error(ess.str().c_str());
        }
    }
};


/*
Checks for log configuration.
*/
void test_log1()
{
    json::Value jcfg;
    jcfg["targets"]["myfile"]["type"] = "file";
    jcfg["targets"]["myfile"]["fileName"] = "test.log";
    jcfg["loggers"]["/"]["level"] = "TRACE";
    jcfg["loggers"]["/"]["targets"].append("console");
    jcfg["loggers"]["/"]["targets"].append("myfile");
    jcfg["loggers"]["/API"]["level"] = "TRACE";
    jcfg["loggers"]["/API"]["targets"].append("myfile");

    std::cout << jcfg << "\n";
    LogConfig::apply(jcfg);
}

} // local namespace
