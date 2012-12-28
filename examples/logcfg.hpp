/** @file
@brief The log configuration example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#ifndef __EXAMPLES_LOGCFG_HPP_
#define __EXAMPLES_LOGCFG_HPP_

#include <hive/json.hpp>
#include <hive/log.hpp>

#if !defined(HIVE_PCH)
#   include <boost/algorithm/string.hpp>
#endif // HIVE_PCH


/// @brief The log configuration.
namespace logcfg
{

/// @brief The log configuration.
/**
The main configuration should be an JSON object with the following members:

~~~{.js}
configuration: {
    targets: {
        target1: { ... }
        target2: { ... }
        ...
    }

    loggers: {
        logger1: { ... }
        logger2: { ... }
        ...
    }
}
~~~

Each *target* description shoud be a valid JSON object with the mandatory
`type` member which could be one of the following:
  - `file` for file targets
  - `stdout` or `console` for `std::cout` target
  - `stderr` for `std::cerr` target
  - `null` for **NULL** target

Each target supports the following common parameters:
  - `minimumLevel` is the minimum logging level for that target,
     see below for possible values
  - `format` is the custom format string,
     see hive::log::Format for more details

The `file` target also supports the following parameters:
  - `fileName` is mandatory log file name
  - `autoFlushLevel` is the *auto-flush* level, see below for possible values
  - `maxFileSize` is the maximum log file size in bytes,
     `K`, `M` and `G` suffixes supported
  - `numOfBackups` is the maximum number of backup log files.

The *logger* supports the following parameters:
  - `level` is the logging level, see below for possible values
  - `targets` is the array of targets for this logger

The logging level can be one of:

| string identifier     | logging level                 |
|-----------------------|-------------------------------|
| `"ALL"`               | hive::log::LEVEL_ALL          |
| `"TRACE"`             | hive::log::LEVEL_TRACE        |
| `"DEBUG"`             | hive::log::LEVEL_DEBUG        |
| `"INFO"`              | hive::log::LEVEL_INFO         |
| `"WARN"`              | hive::log::LEVEL_WARN         |
| `"ERROR"`             | hive::log::LEVEL_ERROR        |
| `"FATAL"`             | hive::log::LEVEL_FATAL        |
| `"OFF"` \n `"NO"`     | hive::log::LEVEL_OFF          |
| `"ASPARENT"` \n `"-"` | hive::log::LEVEL_AS_PARENT    |

Example:

~~~{.js}
{
  targets: {
    console: {
      type: "console"
      minimumLevel: "INFO"
      format: "%L %N %M\n"
    }
    myfile: {
      type: "file"
      fileName: "test.log"
      maxFileSize: "1M"
      numOfBackups: 1
    }
  }

  loggers: {
    "/": {
      level: "TRACE"
      targets: ["myfile", "console"]
    }
    "/API": {
      level: "DEBUG"
      targets: ["myfile"]
    }
  }
}
~~~
*/
class LogConfig
{
public:
    typedef hive::json::Value JsonValue; ///< @brief The JSON value type.
    typedef hive::log::Target::SharedPtr TargetPtr; ///< @brief The target pointer type.

public:

    /// @brief The main constructor.
    /**
    Parses the log configuration in JSON format.
    @param[in] jcfg The log configuration in JSON format.
    @throw std::runtime_error in case of error.
    */
    LogConfig(JsonValue const& jcfg)
    {
        // default targets...
        m_targets["null"] = hive::log::Target::create();
        m_targets["stderr"] = hive::log::Target::Stream::createStderr();
        m_targets["stdout"] = hive::log::Target::Stream::createStdout();
        m_targets["console"] = m_targets["stdout"];

        parseTargets(jcfg["targets"]);
        parseLoggers(jcfg["loggers"]);
    }


    /// @brief Apply parsed configuration.
    void apply() const
    {
        typedef std::map<hive::String, LoggerInfo>::const_iterator Iterator;
        Iterator i = m_loggers.begin();
        Iterator e = m_loggers.end();
        for (; i != e; ++i)
        {
            hive::String const& name = i->first;
            LoggerInfo const& info = i->second;
            using namespace hive::log;

            Logger logger(name);
            if (1 < info.targets.size())
            {
                Target::Tie::SharedPtr tie = Target::Tie::create();
                for (size_t i = 0; i < info.targets.size(); ++i)
                    tie->add(info.targets[i]); // default level!
                logger.setTarget(tie);
            }
            else if (!info.targets.empty())
                logger.setTarget(info.targets.front());

            if (info.level != -1)
                logger.setLevel(Level(info.level));
        }
    }

private:

    /// @brief Parse the log targets.
    /**
    @param[in] jtargets The targets configuration.
    */
    void parseTargets(JsonValue const& jtargets)
    {
        if (!jtargets.isNull() && !jtargets.isObject())
            throw std::runtime_error("invalid targets configuration");

        JsonValue::MemberIterator i = jtargets.membersBegin();
        JsonValue::MemberIterator e = jtargets.membersEnd();
        for (; i != e; ++i)
        {
            hive::String const& name = i->first;
            JsonValue const& jtarget = i->second;

            m_targets[name] = makeTarget(jtarget);
        }
    }


    /// @brief Make the log target.
    /**
    @param[in] jtarget The target configuration.
    @return The new log target.
    @throw std::runtime_error in case of error.
    */
    static TargetPtr makeTarget(JsonValue const& jtarget)
    {
        using namespace hive::log;

        try
        {
            hive::String ttype;
            TargetPtr target;

            // get target type
            if (jtarget.isNull())
                ttype = "null";
            else if (jtarget.isString())
                ttype = jtarget.asString();
            else if (jtarget.isObject())
                ttype = jtarget["type"].asString();
            else
                throw std::runtime_error("invalid target configuration");

            if (boost::iequals(ttype, "file"))
            {
                if (!jtarget.isObject())
                    throw std::runtime_error("invalid file target configuration");

                const hive::String fileName = jtarget["fileName"].asString();
                if (fileName.empty())
                    throw std::runtime_error("not file name provided");
                Target::File::SharedPtr file = Target::File::create(fileName);

                // auto-flush level
                const hive::String autoFlushLevel = jtarget["autoFlushLevel"].asString();
                if (!autoFlushLevel.empty())
                    file->setAutoFlushLevel(parseLevel(autoFlushLevel));

                // maximum file size
                const hive::String maxFileSize = jtarget["maxFileSize"].asString();
                if (!maxFileSize.empty())
                    file->setMaxFileSize(parseFileSize(maxFileSize));

                // number of backups
                file->setNumberOfBackups(jtarget["numOfBackups"].asUInt16());

                target = file;
            }
            else if (boost::iequals(ttype, "stdout")
                || boost::iequals(ttype, "console"))
            {
                target = Target::Stream::createStdout();
            }
            else if (boost::iequals(ttype, "stderr"))
            {
                target = Target::Stream::createStderr();
            }
            else if (boost::iequals(ttype, "null"))
            {
                target = Target::create(); // NULL
            }

            // common properties
            if (target && jtarget.isObject())
            {
                // minimum logging level
                const hive::String minimumLevel = jtarget["minimumLevel"].asString();
                if (!minimumLevel.empty())
                    target->setMinimumLevel(parseLevel(minimumLevel));

                // custom format
                const hive::String format = jtarget["format"].asString();
                if (!format.empty())
                    target->setFormat(Format::create(format));
            }

            // unknown target
            if (!target)
            {
                hive::OStringStream ess;
                ess << "\"" << ttype << "\" is unknown log target type";
                throw std::runtime_error(ess.str().c_str());
            }

            return target;
        }
        catch (std::exception const& ex)
        {
            hive::OStringStream ess;
            ess << "cannot create target: "
                << ex.what();
            throw std::runtime_error(ess.str().c_str());
        }
    }


    /// @brief Get the target configuration.
    TargetPtr getTarget(hive::String const& name) const
    {
        std::map<hive::String, TargetPtr>::const_iterator t = m_targets.find(name);
        if (t != m_targets.end())
            return t->second;
        else
        {
            hive::OStringStream ess;
            ess << "\"" << name << "\" is unknown log target name";
            throw std::runtime_error(ess.str().c_str());
        }
    }

private:

    /// @brief Parse the loggers.
    /**
    @param[in] jloggers The loggers configuration.
    */
    void parseLoggers(JsonValue const& jloggers)
    {
        if (!jloggers.isNull() && !jloggers.isObject())
            throw std::runtime_error("invalid loggers configuration");

        JsonValue::MemberIterator i = jloggers.membersBegin();
        JsonValue::MemberIterator e = jloggers.membersEnd();

        for (; i != e; ++i)
        {
            hive::String const& name = i->first;
            JsonValue const& jlogger = i->second;
            LoggerInfo &logger = m_loggers[name];

            if (jlogger.isString())
                logger.targets.push_back(getTarget(jlogger.asString()));
            else if (jlogger.isObject())
            {
                JsonValue const& jtargets = jlogger["targets"];

                // get the target list
                if (jtargets.isArray())
                {
                    JsonValue::ElementIterator i = jtargets.elementsBegin();
                    JsonValue::ElementIterator e = jtargets.elementsEnd();

                    for (; i != e; ++i)
                    {
                        const hive::String tname = i->asString();
                        logger.targets.push_back(getTarget(tname));
                    }
                }
                else
                    throw std::runtime_error("invalid targets list");
            }
            else
                throw std::runtime_error("invalid logger configuration");

            const hive::String level = jlogger["level"].asString();
            if (!level.empty())
                logger.level = parseLevel(level);
        }
    }

private:

    /// @brief Parse the log level.
    /**
    @param[in] level The logging level name.
    @return The logging level.
    @throw std::runtime_error in case of error.
    */
    static hive::log::Level parseLevel(hive::String const& level)
    {
        using namespace hive;

        if (boost::iequals(level, "ALL"))
            return log::LEVEL_ALL;
        else if (boost::iequals(level, "TRACE"))
            return log::LEVEL_TRACE;
        else if (boost::iequals(level, "DEBUG"))
            return log::LEVEL_DEBUG;
        else if (boost::iequals(level, "INFO"))
            return log::LEVEL_INFO;
        else if (boost::iequals(level, "WARN"))
            return log::LEVEL_WARN;
        else if (boost::iequals(level, "ERROR"))
            return log::LEVEL_ERROR;
        else if (boost::iequals(level, "FATAL"))
            return log::LEVEL_FATAL;
        else if (boost::iequals(level, "OFF") || boost::iequals(level, "NO"))
            return log::LEVEL_OFF;
        else if (boost::iequals(level, "ASPARENT") || boost::iequals(level, "-"))
            return log::LEVEL_AS_PARENT;
        else // unknown level
        {
            hive::OStringStream ess;
            ess << "\"" << level << "\" is unknown logging level";
            throw std::runtime_error(ess.str().c_str());
        }
    }


    /// @brief Parse the file size.
    /**
    @param[in] fileSize The file size string.
        May be siffuxed with `K`, `M` or `G`.
    @return The file size in bytes.
    @throw std::runtime_error in case of error.
    */
    static size_t parseFileSize(hive::String const& fileSize)
    {
        hive::IStringStream iss(fileSize);

        double size = 0;
        if (!(iss >> size))
            throw std::runtime_error("cannot parse size");

        hive::String suffix; // optional
        if ((iss >> suffix) && !suffix.empty())
        {
            if (boost::iequals(suffix, "K"))
                size *= 1024;
            else if (boost::iequals(suffix, "M"))
                size *= 1024*1024;
            else if (boost::iequals(suffix, "G"))
                size *= 1024*1024*1024;
            else // unknown suffix
            {
                hive::OStringStream ess;
                ess << "\"" << suffix << "\" is unknown file size suffix";
                throw std::runtime_error(ess.str().c_str());
            }
        }

        if (!(iss >> std::ws).eof())
            throw std::runtime_error("invalid file size");

        if (size < std::numeric_limits<size_t>::min()
            || std::numeric_limits<size_t>::max() < size)
        {
            throw std::runtime_error("invalid file size - out of range");
        }

        return size_t(size);
    }

private:

    /// @brief The list of known targets.
    std::map<hive::String, TargetPtr> m_targets;

    /// @brief The logger information.
    struct LoggerInfo
    {
        std::vector<TargetPtr> targets; ///< @brief The list of targets.
        int level;         ///< @brief The logging level.

        /// @brief The default constructor.
        LoggerInfo()
            : level(-1) // don't change
        {}
    };

    /// @brief The list of known loggers.
    std::map<hive::String, LoggerInfo> m_loggers;
};


/// @brief Apply a log configuration.
/**
This function parses and applies the log configuration.

@param[in] jcfg The log configuration in JSON format.
@throw std::runtime_error in case of error.
@see LogConfig
*/
inline void apply(hive::json::Value const& jcfg)
{
    LogConfig cfg(jcfg);
    cfg.apply();
}


/// @brief Load and apply a log configuration from file.
/**
@param[in] fileName The log configuration file name.
@throw std::runtime_error in case of error.
*/
inline void apply(hive::String const& fileName)
{
    try
    {
        std::ifstream i_file(fileName.c_str());
        if (!i_file.is_open())
            throw std::runtime_error("file not found");

        hive::json::Value jcfg;
        i_file >> jcfg;

        apply(jcfg);
    }
    catch (std::exception const& ex)
    {
        hive::OStringStream ess;
        ess << "cannot apply log configuration: "
            << ex.what();
        throw std::runtime_error(ess.str().c_str());
    }
}

} // logcfg namespace

#endif //__EXAMPLES_LOGCFG_HPP_
