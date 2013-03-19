/** @file
@brief The logging tools.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_hive_log
*/
#ifndef __HIVE_LOG_HPP_
#define __HIVE_LOG_HPP_

#include "defs.hpp"

#if !defined(HIVE_PCH)
#   include <boost/shared_ptr.hpp>
#   include <boost/weak_ptr.hpp>
#   include <iostream>
#   include <sstream>
#   include <fstream>
#   include <vector>
#endif // HIVE_PCH

#include <boost/date_time/posix_time/posix_time.hpp>


// TODO: logger configuration from file
// TODO: description & examples

namespace hive
{
    /// @brief The logging tools.
    namespace log
    {

/// @brief The logging levels.
/**
There are a few main levels (ordered from the less important
to the most important):
  - hive::log::LEVEL_TRACE is used to log almost everything
  - hive::log::LEVEL_DEBUG is used to log debug information
  - hive::log::LEVEL_INFO is used to log main information
  - hive::log::LEVEL_WARN is used to log potentially dangerous situations
  - hive::log::LEVEL_ERROR is used to log non-critical errors or exceptions
  - hive::log::LEVEL_FATAL is used to log fatal errors

and a few auxiliary levels:
  - hive::log::LEVEL_ALL is used to log everything
  - hive::log::LEVEL_OFF no any logging at all
  - hive::log::LEVEL_AS_PARENT is used in loggers hierarchy to indicate
        that the logging level is the same as its parent logging level
*/
enum Level
{
    LEVEL_ALL,      ///< @brief Full logging.
    LEVEL_TRACE,    ///< @brief The **TRACE** level.
    LEVEL_DEBUG,    ///< @brief The **DEBUG** level.
    LEVEL_INFO,     ///< @brief The **INFO** level.
    LEVEL_WARN,     ///< @brief The **WARN** level.
    LEVEL_ERROR,    ///< @brief The **ERROR** level.
    LEVEL_FATAL,    ///< @brief The **FATAL** level.
    LEVEL_OFF,      ///< @brief No logging.
    LEVEL_AS_PARENT ///< @brief The same as parent's.
};


/// @brief The log message.
/**
This class holds all information about log message.

You have to deal with this class directly only if you develop
your own log target or formatter. See hive::log::Target::send()
and hive::log::Format::format() methods.

@warning This class doesn't make any copies of strings.
Be careful with strings lifetime.
*/
class Message
{
public:
    const char* loggerName; ///< @brief The source logger name.
    Level       level;      ///< @brief The logging level.

    const char* message;  ///< @brief The log message.
    const char* prefix;   ///< @brief The log message prefix (optional).

    const char* file;     ///< @brief The source file name.
    int         line;     ///< @brief The source line number.

    boost::posix_time::ptime timestamp; ///< @brief The log message timestamp.

    // TODO: thread identifier

public:

    /// @brief The main constructor.
    /**
    @param[in] loggerName_ The source logger name.
    @param[in] level_ The logging level.
    @param[in] message_ The log message.
    @param[in] prefix_ The log message prefix.
    @param[in] file_ The source file name.
    @param[in] line_ The source line number.
    */
    Message(const char* loggerName_, Level level_,
            const char* message_, const char *prefix_,
            const char* file_, int line_)
        : loggerName(loggerName_)
        , level(level_)
        , message(message_)
        , prefix(prefix_)
        , file(file_)
        , line(line_)
        , timestamp(boost::posix_time::microsec_clock::universal_time())
    {}
};


/// @brief The log message format.
/**
This is base class for all log message formats.
Derived classes should override the format() virtual method.

The create() factory method should be used
to get a new instance of the Format class.

By default simple format is used:
~~~
timestamp logger level message
~~~
This format is always available through defaultFormat() static method.

Also there is possible to use custom format (with `printf`-like syntax) which
is available through customFormat() static method. For example, default format
is equivalent to "%T %N %L %M\n". The following format options are allowed:

| option | description                    |
|--------|--------------------------------|
| `%%T`  | the timestamp                  |
| `%%N`  | the logger name                |
| `%%L`  | the logging level              |
| `%%M`  | the log message (with prefix)  |

The auxiliary getLevelName() static method might be used to convert
logging level into the text representation.
*/
class Format
{
protected:

    /// @brief The default constructor.
    Format()
    {}


    /// @brief The main constructor.
    /**
    @param[in] format The custom format string.
    */
    explicit Format(String const& format)
        : m_format(format)
    {}

public:

    /// @brief The trivial destructor.
    virtual ~Format()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Format> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] format The custom format string.
        Empty string for the default format.
    @return The new format instance.
    */
    static SharedPtr create(String const& format = String())
    {
        return SharedPtr(new Format(format));
    }

public:

    /// @brief Get the custom format string.
    /**
    @return The custom format string.
    Empty string means default format.
    */
    String const& getCustomFormat() const
    {
        return m_format;
    }

public:

    /// @brief Do a log message formatting.
    /**
    Uses defaultFormat() or customFormat() method as an implementation.

    @param[in,out] os The output stream.
    @param[in] msg The log message to format.
    */
    virtual void format(OStream &os, Message const& msg)
    {
        if (!m_format.empty())
            customFormat(os, msg, m_format);
        else
            defaultFormat(os, msg);
    }


    /// @brief %Format a log message (default format).
    /**
    @param[in,out] os The output stream.
    @param[in] msg The log message to format.
    */
    static void defaultFormat(OStream &os, Message const& msg)
    {
        os << msg.timestamp << ' ';
        if (msg.loggerName)
            os << msg.loggerName << ' ';
        os << getLevelName(msg.level)
            << ' ';
        if (msg.prefix)
            os << msg.prefix;
        if (msg.message)
            os << msg.message;
        os << '\n';
    }


    /// @brief %Format a log message (custom format).
    /**
    @param[in,out] os The output stream.
    @param[in] msg The log message to format.
    @param[in] fmt The custom format string.
    */
    static void customFormat(OStream &os, Message const& msg, String const& fmt)
    {
        const size_t N = fmt.size();
        for (size_t i = 0; i < N; ++i)
        {
            const char ch = fmt[i];

            if (ch == '%' && (i+1) < N)
            {
                const char ext = fmt[++i];
                // TODO: width and fill?

                switch (ext)
                {
                    case '%':       // percent
                        os.put(ext);
                        break;

                    case 'T':       // timestamp
                        // TODO: advanced time formats like %Y/%m/%D %H:%M:%S
                        os << msg.timestamp;
                        break;

                    case 'N':       // logger name
                        if (msg.loggerName)
                            os << msg.loggerName;
                        break;

                    case 'L':       // logging level
                        os << getLevelName(msg.level);
                        break;

                    case 'M':       // message
                        if (msg.prefix)
                            os << msg.prefix;
                        if (msg.message)
                            os << msg.message;
                        break;

                    // TODO: source file name and line number

                    default:        // unknown format
                        os.put(ch);
                        os.put(ext);
                        break;
                }
            }
            else
                os.put(ch);
        }
    }

public:

    /// @brief Get the logging level name.
    /**
    @param[in] level The logging level.
    @return The logging level name.
    */
    static const char* getLevelName(Level level)
    {
        switch (level)
        {
            case LEVEL_TRACE:       return "TRACE";
            case LEVEL_DEBUG:       return "DEBUG";
            case LEVEL_INFO:        return "INFO";
            case LEVEL_WARN:        return "WARN";
            case LEVEL_ERROR:       return "ERROR";
            case LEVEL_FATAL:       return "FATAL";

            case LEVEL_ALL:         return "ALL";
            case LEVEL_OFF:         return "OFF";
            case LEVEL_AS_PARENT:   return "PARENT";

            default:                return "UNKNOWN";
        }
    }

protected:
    String m_format; ///< @brief The custom format string.
};


/// @brief The log target.
/**
This is the base class for all log targets - endpoint for all log messages.
Base class does nothing and works like a **NULL** target.

See the inner classes for standard targets: Stream, File and Tie.

The target uses specified log message format (see setFormat() method)
or hive::log::Format::defaultFormat() if no any format provided.

There is a simple filter feature - you can provide the minimum logging level.
All log messages with the logging level below this limit will be ignored.
This simple message filter is disabled by default, i.e. the minimum logging
level is hive::log::LEVEL_ALL.
*/
class Target
{
protected:

    /// @brief The default constructor.
    Target()
        : m_minimumLevel(LEVEL_ALL)
    {}

public:

    /// @brief The trivial destructor.
    virtual ~Target()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Target> SharedPtr;


    /// @brief The factory method.
    /**
    @return The new **NULL** target instance.
    */
    static SharedPtr create()
    {
        return SharedPtr(new Target());
    }

public:

    /// @brief Send a log message to the target.
    /**
    @param[in] msg The log message.
    */
    virtual void send(Message const& msg) const
    {
        // do nothing by default
    }

public:

    /// @brief Set message format.
    /**
    @param[in] format The message format. May be NULL.
    @return The self reference.
    */
    Target& setFormat(Format::SharedPtr format)
    {
        m_format = format;
        return *this;
    }


    /// @brief Get message format.
    /**
    @return The message format. May be NULL.
    */
    Format::SharedPtr getFormat() const
    {
        return m_format;
    }

public:

    /// @brief Set the minimum level.
    /**
    @param[in] level The minimum logging level.
    */
    Target& setMinimumLevel(Level level)
    {
        m_minimumLevel = level;
        return *this;
    }


    /// @brief Get the minimum level.
    /**
    @return The minimum logging level.
    */
    Level getMinimumLevel() const
    {
        return m_minimumLevel;
    }

public: // common targets
    class Stream;
    class File;
    class Tie;

protected:
    Format::SharedPtr m_format; ///< @brief The message format.
    Level m_minimumLevel; ///< @brief The simple message filter.
};


/// @brief The "Stream" target.
/**
Sends log messages to an external stream such as `std::cerr` or `std::cout`.

@warning The stream object is stored as reference.
         Mind the external stream object lifetime.
*/
class Target::Stream:
    private NonCopyable,
    public Target
{
protected:

    /// @brief The main constructor.
    /**
    @param[in] os The external stream.
    */
    explicit Stream(OStream &os)
        : m_os(os)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Stream> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] os The external stream.
    @return The new "Stream" target instance.
    */
    static SharedPtr create(OStream &os)
    {
        return SharedPtr(new Stream(os));
    }


    /// @brief The factory method (std::cout).
    /**
    Uses `std::cout` stream as a target stream.
    @return The new "Stream" target instance.
    */
    static SharedPtr createStdout()
    {
        return create(std::cout);
    }


    /// @brief The factory method (std::cerr).
    /**
    Uses `std::cerr` stream as a target stream.
    @return The new "Stream" target instance.
    */
    static SharedPtr createStderr()
    {
        return create(std::cerr);
    }

public:

    /// @copydoc Target::send()
    /**
    Writes the log message to the external stream.
    */
    virtual void send(Message const& msg) const
    {
        // apply simple message filter
        if (msg.level < getMinimumLevel())
            return;

        if (Format::SharedPtr fmt = getFormat())
            fmt->format(m_os, msg);
        else
            Format::defaultFormat(m_os, msg);
    }

protected:
    OStream & m_os; ///< @brief The external stream.
};


/// @brief The "File" target.
/**
Sends log messages to the file stream.

All write operations may be automatically flushed to disk but of course it
reduces the overal application performance. The "auto-flush" logging level
is used to control this process. By default it's hive::log::LEVEL_WARN,
so all **WARN**, **ERROR** and **FATAL** message will be automatically
flushed, other messages (**INFO**, **DEBUG**, **TRACE**) will not.

A log file size limit might be specified as long as a maximum number
of log file backups. The file size limit isn't perfect precised and has the
log message boundary. That means that the whole log message will be written
although there is only one byte of free space in the log file. Once log file
is full the "backup" procedure will be started:

|   before   |    after   | comments                   |
|------------|------------|----------------------------|
| `my.log.4` |            | deleted (the oldest)       |
| `my.log.3` | `my.log.4` | rename: `log.3` -> `log.4` |
| `my.log.2` | `my.log.3` | rename: `log.2` -> `log.3` |
| `my.log.1` | `my.log.2` | rename: `log.1` -> `log.2` |
| `my.log.0` | `my.log.1` | rename: `log.0` -> `log.1` |
| `my.log`   | `my.log.0` | rename: `log` -> `log.0`   |
|            | `my.log`   | created (the newest)       |

Note, it's not recommended to provide log file size limit with no backups.

By default there is no any file size limit and therefore no any backups.

Log file is always open in *append* mode.
*/
class Target::File:
    private NonCopyable,
    public Target
{
protected:

    /// @brief The main constructor.
    /**
    @param[in] fileName The log file name.
    @param[in] autoFlushLevel The "auto-flush" logging level.
    */
    File(String const& fileName, Level autoFlushLevel)
        : m_fileName(fileName)
        , m_autoFlushLevel(autoFlushLevel)
        , m_maxFileSize(0)
        , m_numOfBackups(0)
        , m_needNewFile(false)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<File> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] fileName The log file name.
    @param[in] autoFlushLevel The "auto-flush" logging level.
    @return The new "File" target instance.
    */
    static SharedPtr create(String const& fileName, Level autoFlushLevel = LEVEL_WARN)
    {
        return SharedPtr(new File(fileName, autoFlushLevel));
    }

public:

    /// @brief Set the "auto-flush" logging level.
    /**
    @param[in] autoFlushLevel The "auto-flush" logging level.
    @return The self reference.
    */
    File& setAutoFlushLevel(Level autoFlushLevel)
    {
        m_autoFlushLevel = autoFlushLevel;
        return *this;
    }


    /// @brief Get the "auto-flush" logging level.
    /**
    @return The "auto-flush" logging level.
    */
    Level getAutoFlushLevel() const
    {
        return m_autoFlushLevel;
    }

public:

    /// @brief Set the log file size limit.
    /**
    @param[in] maxFileSize The maximum file size in bytes.
    @return The self reference.
    */
    File& setMaxFileSize(size_t maxFileSize)
    {
        m_maxFileSize = maxFileSize;
        return *this;
    }


    /// @brief Get the log file size limit.
    /**
    @return The maximum file size in bytes.
    */
    size_t getMaxFileSize() const
    {
        return size_t(m_maxFileSize);
    }

public:

    /// @brief Set the maximum number of backups.
    /**
    @param[in] N The maximum number of backups.
    @return The self reference.
    */
    File& setNumberOfBackups(size_t N)
    {
        m_numOfBackups = N;
        return *this;
    }


    /// @brief Get the maximum number of backups.
    /**
    @return The maximum number of backups.
    */
    size_t getNumberOfBackups() const
    {
        return m_numOfBackups;
    }

public:

    /// @copydoc Target::send()
    /**
    Writes the log message to the file stream.
    */
    virtual void send(Message const& msg) const
    {
        // apply simple message filter
        if (msg.level < getMinimumLevel())
            return;

        if (!m_file.is_open()) // try to open/reopen
        {
            if (m_needNewFile)
                startNewFile();

            m_file.clear();
            m_file.open(m_fileName.c_str(),
                std::ios::binary|std::ios::app); // append!
        }

        if (m_file.is_open()) // write message
        {
            if (Format::SharedPtr fmt = getFormat())
                fmt->format(m_file, msg);
            else
                Format::defaultFormat(m_file, msg);

            if (m_autoFlushLevel <= msg.level)
                m_file.flush();

            // check the file size...
            const std::streamsize sz = m_file.tellp();
            if (m_maxFileSize && m_maxFileSize <= sz)
            {
                // restart next time!
                m_needNewFile = true;
                m_file.close();
            }
        }
    }

private:

    /// @brief Start new log file.
    /**
    Do backups and start writing to new file.
    */
    void startNewFile() const
    {
        const size_t W = getNumOfDigits();
        const size_t N = m_numOfBackups;

        if (0 < N)
        {
            // remove the latest one
            String next = buildFileName(N-1, W);
            ::remove(next.c_str());

            // level-up the backups
            for (size_t i = N-1; 0 < i; --i)
            {
                String prev = buildFileName(i-1, W);
                ::rename(prev.c_str(), next.c_str());
                next.swap(prev);
            }

            // reset the current log file
            ::rename(m_fileName.c_str(), next.c_str());
        }
        else
        {
            // remove the current log file
            ::remove(m_fileName.c_str());
        }
    }


    /// @brief Build backup file name.
    /**
    @param[in] index The backup index.
    @param[in] width The number of backup digits.
    @return The backup file name.
    */
    String buildFileName(size_t index, size_t width) const
    {
        OStringStream oss;
        oss.fill('0');
        oss << m_fileName << '.'
            << std::setw(width)
            << index;
        return oss.str();
    }


    /// @brief Get the number of backup digits.
    /**
    The output is equal to `ceil(log10(m_numOfBackups))`.
    @return The number of backup digits.
    */
    size_t getNumOfDigits() const
    {
        size_t H = 10;
        size_t n = 1;
        while (H < m_numOfBackups && n < 10)
        {
            H *= 10;
            n += 1;
        }
        return n;
    }

private:
    String m_fileName; ///< @brief The file name.
    Level m_autoFlushLevel; ///< @brief The "auto-flush" level.

    std::streamsize m_maxFileSize; ///< @brief The maximum file size in bytes.
    size_t m_numOfBackups; ///< @brief The maximum number of backups.
    mutable bool m_needNewFile; ///< @brief The need new file flag.

    mutable std::ofstream m_file; ///< @brief The file stream.
};


/// @brief The "Tie" target.
/**
Sends log messages to the several child targets.

You could add many child targets using add() method.
There is no way to remove a child target, it's only
possible to remove all child targets using clear().

The add() method returns self reference so you can
combine mutiple inserts into one line:

~~~{.cpp}
hive::log::Target::Tie::SharedPtr tie = hive::log::Target::Tie::create();
tie->add(t1).add(t2).add(t3).add(t4);
~~~

If the number of child targets is less than five you can use create() method:

~~~{.cpp}
hive::log::Target::Tie::SharedPtr tie = hive::log::Target::Tie::create(t1, t2, t3, t4);
~~~

It is possible to specify the minimum logging level filter for each child target individually.
That means each message will be filtered three times:
  - Tie target filter itself
  - Tie child filter (specified with add() method)
  - child target filter
*/
class Target::Tie:
    public Target
{
protected:

    /// @brief The default constructor.
    /**
    No any child targets.
    */
    Tie()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Tie> SharedPtr;


    /// @brief The factory method.
    /**
    If you need more than four child targets, use add() method instead.

    @param[in] a The first child target. May be NULL.
    @param[in] b The second child target. May be NULL.
    @param[in] c The third child target. May be NULL.
    @param[in] d The fourth child target. May be NULL.
    @return The new "Tie" target instance.
    */
    static SharedPtr create(
        Target::SharedPtr a = Target::SharedPtr(),
        Target::SharedPtr b = Target::SharedPtr(),
        Target::SharedPtr c = Target::SharedPtr(),
        Target::SharedPtr d = Target::SharedPtr())
    {
        SharedPtr tie(new Tie());
        if (a) tie->add(a);
        if (b) tie->add(b);
        if (c) tie->add(c);
        if (d) tie->add(d);
        return tie;
    }

public:

    /// @copydoc Target::send()
    /**
    Sends the log message to the all child targets.
    */
    virtual void send(Message const& msg) const
    {
        // apply simple message filter
        if (msg.level < getMinimumLevel())
            return;

        Container::const_iterator i = m_childs.begin();
        Container::const_iterator const e = m_childs.end();
        for (; i != e; ++i)
        {
            Target::SharedPtr target = i->first;
            Level minimumLevel = i->second;

            // apply simple message filter individually
            if (minimumLevel <= msg.level)
                target->send(msg);
        }
    }

public:

    /// @brief Add child target.
    /**
    @param[in] target The child target.
    @param[in] minimumLevel The minimum logging level.
    @return The self reference.
    */
    Tie& add(Target::SharedPtr target, Level minimumLevel = LEVEL_ALL)
    {
        if (target)
            m_childs.push_back(std::make_pair(target, minimumLevel));
        return *this;
    }


    /// @brief Remove all child targets.
    /**
    @return The self reference.
    */
    Tie& clear()
    {
        m_childs.clear();
        return *this;
    }

protected:
    typedef std::pair<Target::SharedPtr, Level> Child; ///< @brief The child target type.
    typedef std::vector<Child> Container; ///< @brief The list of child targets type.
    Container m_childs; ///< @brief The list of child targets.
};


/// @brief The logger.
/**
This is the main class of the logging tools.

All loggers organized into tree hierarchy. The root logger available
through root() static method. By default root() logger sends messages
to the standard error stream at the **WARN** level minimum.
*/
class Logger
{
private:

    /// @brief Implementation.
    /**
    This class is used as hidden implementation to support
    easy copying of hive::log::Logger instances.
    */
    class Impl
    {
    public:
        Level level; ///< @brief The logging level.
        String name; ///< @brief The logger name.

        Target::SharedPtr target; ///< @brief The current target.
        boost::weak_ptr<Impl> parent; ///< @brief The parent logger.

    public:

        /// @brief The default constructor.
        /**
        @param[in] name_ The logger name.
        @param[in] level_ The logging level.
        */
        Impl(String const& name_, Level level_)
            : level(level_)
            , name(name_)
        {}

    public:

        /// @brief The shared pointer type.
        typedef boost::shared_ptr<Impl> SharedPtr;

        /// @brief The log childs.
        std::vector<SharedPtr> childs;

    public:

        /// @brief Find child by name.
        /**
        @param[in] name The logger name.
        @return The child or NULL.
        */
        SharedPtr find(String const& name) const
        {
            const size_t N = childs.size();
            for (size_t i = 0; i < N; ++i)
            {
                SharedPtr impl = childs[i];
                if (impl->name == name)
                    return impl;
            }

            return SharedPtr(); // not found
        }
    };

public:

    /// @brief The main constructor.
    /**
    @param[in] name The logger name.
    */
    explicit Logger(String const& name)
        : m_impl(getImpl(name))
    {}

private:

    /// @brief The default constructor.
    /**
    Is used for root logger.
    */
    Logger()
        : m_impl(getRootImpl())
    {}

public:

    /// @brief Get the logging level.
    /**
    @return The logging level.
    */
    Level getLevel() const
    {
        return m_impl->level;
    }


    /// @brief Set the logging level.
    /**
    @param[in] level The logging level.
    @return The self reference.
    */
    Logger& setLevel(Level level)
    {
        m_impl->level = level;
        return *this;
    }


    /// @brief Is the logging enabled for the level?
    /**
    @param[in] level The logging level to check.
    @return `true` if logging enabled.
    */
    bool isEnabledFor(Level level) const
    {
        Impl::SharedPtr i = m_impl;

        while (i)
        {
            if (i->level != LEVEL_AS_PARENT)
                return (i->level <= level);
            i = i->parent.lock();
        }

        return false; // disabled by default
    }

public:

    /// @brief Get the logger name.
    /**
    @return The logger name.
    */
    String const& getName() const
    {
        return m_impl->name;
    }

public:

    /// @brief Get the target.
    /**
    @return The target. May be NULL.
    */
    Target::SharedPtr getTarget() const
    {
        return m_impl->target;
    }


    /// @brief Set the target.
    /**
    @param[in] target The new target.
    @return The self reference.
    */
    Logger& setTarget(Target::SharedPtr target)
    {
        m_impl->target = target;
        return *this;
    }

public:

    /// @brief Get the root logger.
    /**
    @return The root logger reference.
    */
    static Logger& root()
    {
        // empty name
        static Logger L;
        return L;
    }

public:

    /// @brief Send log message to the target.
    /**
    If there is no appropriate target found then the log message is ignored.

    @param[in] level The logging level.
    @param[in] message The log message.
    @param[in] prefix The log message prefix (optional).
    @param[in] file The source file name (optional).
    @param[in] line The source line number.
    */
    void send(Level level, const char* message, const char *prefix, const char* file, int line) const
    {
        if (Target::SharedPtr target = getAppropriateTarget())
        {
            Message msg(getName().c_str(), level,
                message, prefix, file, line);
            target->send(msg);
        }
    }

private:

    /// @brief Get the appropriate target.
    /**
    This method returns the first non-NULL target in loggers hierarchy.

    @return The target or NULL.
    */
    Target::SharedPtr getAppropriateTarget() const
    {
        Impl::SharedPtr i = m_impl;

        while (i)
        {
            if (i->target)
                return i->target;
            i = i->parent.lock();
        }

        return Target::SharedPtr(); // not found
    }

private:

    /// @brief Get implementation by log name.
    /**
    This method finds the implementation with provided name or creates new one.

    @param[in] name The log name.
    @return The logger implementation.
    */
    static Impl::SharedPtr getImpl(String const& name)
    {
        const char SEP = '/'; // name separator

        Impl::SharedPtr impl = root().m_impl;
        String new_name; //new_name.reserve(name.size());

        for (size_t t = 0; ;) // tokinizer position
        {
            const size_t e = name.find(SEP, t);
            String sub_name = name.substr(t, e-t);

            if (!sub_name.empty())
            {
                new_name += SEP;
                new_name += sub_name;

                Impl::SharedPtr child = impl->find(new_name);
                if (!child)
                {
                    child.reset(new Impl(new_name, LEVEL_AS_PARENT));
                    impl->childs.push_back(child);
                    child->parent = impl;
                }
                impl = child;
            }

            if (e != String::npos)
                t = e+1;
            else
                break;
        }

        return impl;
    }


    /// @brief Get root implementation.
    /**
    The log level is **WARN** and the target
    is `std::cerr` stream by default.

    @return The root logger implementation.
    */
    static Impl::SharedPtr getRootImpl()
    {
        Impl::SharedPtr impl(new Impl(String(), LEVEL_WARN));
        impl->target = Target::Stream::createStderr();
        return impl;
    }

private:
    Impl::SharedPtr m_impl; ///< @brief The implementation.
};


        namespace impl
        {

/// @brief The code block logging.
/**
Is used to log method calls or other code blocks.
At construction sends *ENTER* log message.
At destruction sends *LEAVE* log message.

For example:

~~~{.cpp}
void f()
{
    HIVELOG_TRACE_BLOCK(logger, "f()");

    // function body
}
~~~

Cannot be used with temporary strings.
*/
class Block
{
public:

    /// @brief The main constructor.
    /**
    Sends the *ENTER* log message.

    @param[in] logger The logger.
    @param[in] level The logging level.
    @param[in] message The custom message.
    @param[in] file The file name.
    @param[in] line The line number.
    */
    Block(Logger const& logger, Level level, const char* message, const char* file, int line)
        : m_logger(logger)
        , m_level(level)
        , m_message(message)
        , m_file(file)
        , m_line(line)
    {
        if (m_logger.isEnabledFor(m_level))
        {
            logger.send(m_level, m_message,
                ">>>ENTER: ", m_file, m_line);
        }
    }


    /// @brief The destructor.
    /**
    Sends the *LEAVE* log message.
    */
    ~Block()
    {
        if (m_logger.isEnabledFor(m_level))
        {
            m_logger.send(m_level, m_message,
                "<<<LEAVE: ", m_file, m_line);
        }
    }

private:
    Logger const& m_logger; ///< @brief The logger.
    Level         m_level;  ///< @brief The logging level.
    const char* m_message;  ///< @brief The log message.
    const char* m_file;     ///< @brief The source file name.
    int         m_line;     ///< @brief The source line number.
};

        } // impl namespace
    } // log namespace
} // hive namespace


/// @name Implementation
/// @{

/// @brief Send complex log message.
/**
This macro formats the log message using hive::OStringStream object.

@param[in] logger The logger.
@param[in] message The log message.
@param[in] level The logging level.
@hideinitializer
*/
#define IMPL_HIVELOG_BODY(logger, message, level)               \
    do {                                                        \
        if ((logger).isEnabledFor(hive::log::level))            \
        {                                                       \
            hive::OStringStream oss;                            \
            oss << message;                                     \
            (logger).send(hive::log::level,                     \
                oss.str().c_str(), 0, __FILE__, __LINE__);      \
        }                                                       \
    } while (0)


/// @brief Send simple log string.
/**
@param[in] logger The logger.
@param[in] message The log message (C-string).
@param[in] level The logging level.
@hideinitializer
*/
#define IMPL_HIVELOG_STR_BODY(logger, message, level)           \
    do {                                                        \
        if ((logger).isEnabledFor(hive::log::level))            \
        {                                                       \
            (logger).send(hive::log::level,                     \
                message, 0, __FILE__, __LINE__);                \
        }                                                       \
    } while (0)


/// @brief Define *ENTER/LEAVE* log block.
/**
@param[in] logger The logger.
@param[in] message The log message (C-string).
@param[in] level The logging level.
@hideinitializer
*/
#define IMPL_HIVELOG_BLOCK_BODY(logger, message, level)         \
    hive::log::impl::Block hive_log_block_##level(logger,       \
        hive::log::level, message, __FILE__, __LINE__)


/// @brief Disable logging.
/**
@param[in] logger The logger.
@param[in] message The log message.
@hideinitializer
*/
#define IMPL_HIVELOG_DISABLED(logger, message)                  \
    do {} while (0)

/// @}

#endif // __HIVE_LOG_HPP_


// we have to put these macroses outside the file guards
// to be able to enable/disable log levels for the same source file
#if 1 // macro magic :)

// disable logging at compile time
#if defined(HIVELOG_DISABLE_FATAL) && !defined(HIVELOG_DISABLE_ERROR)
#   define HIVELOG_DISABLE_ERROR
#endif
#if defined(HIVELOG_DISABLE_ERROR) && !defined(HIVELOG_DISABLE_WARN)
#   define HIVELOG_DISABLE_WARN
#endif
#if defined(HIVELOG_DISABLE_WARN) && !defined(HIVELOG_DISABLE_INFO)
#   define HIVELOG_DISABLE_INFO
#endif
#if defined(HIVELOG_DISABLE_INFO) && !defined(HIVELOG_DISABLE_DEBUG)
#   define HIVELOG_DISABLE_DEBUG
#endif
#if defined(HIVELOG_DISABLE_DEBUG) && !defined(HIVELOG_DISABLE_TRACE)
#   define HIVELOG_DISABLE_TRACE
#endif

#if defined(HIVE_DOXY_MODE)
/// @name Compile-time logging control
/// @{
#define HIVELOG_DISABLE_FATAL ///< @brief Define this macro to disable all logging at compile time.
#define HIVELOG_DISABLE_ERROR ///< @brief Define this macro to disable **ERROR** and below logging at compile time.
#define HIVELOG_DISABLE_WARN  ///< @brief Define this macro to disable **WARN** and below logging at compile time.
#define HIVELOG_DISABLE_INFO  ///< @brief Define this macro to disable **INFO** and below logging at compile time.
#define HIVELOG_DISABLE_DEBUG ///< @brief Define this macro to disable **DEBUG** and below logging at compile time.
#define HIVELOG_DISABLE_TRACE ///< @brief Define this macro to disable **TRACE** and below logging at compile time.
/// @}
#endif // HIVE_DOXY_MODE


// TRACE logging
#undef HIVELOG_TRACE
#undef HIVELOG_TRACE_STR
#undef HIVELOG_TRACE_BLOCK
#if defined(HIVELOG_DISABLE_TRACE) || defined(HIVE_DOXY_MODE)
/// @name TRACE logging
/// @{

/// @brief Send a log message at **TRACE** level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_TRACE(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at **TRACE** level.
/**
@param[in] logger The logger instance.
@param[in] message The log string.
@hideinitializer
*/
#define HIVELOG_TRACE_STR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Define *ENTER/LEAVE* block at **TRACE** level.
/**
@param[in] logger The logger instance.
@param[in] message The block name.
@hideinitializer
*/
#define HIVELOG_TRACE_BLOCK(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @}
#else // defined(HIVELOG_DISABLE_TRACE)

#define HIVELOG_TRACE(logger, message) \
    IMPL_HIVELOG_BODY(logger, message, LEVEL_TRACE)

#define HIVELOG_TRACE_STR(logger, message) \
    IMPL_HIVELOG_STR_BODY(logger, message, LEVEL_TRACE)

#define HIVELOG_TRACE_BLOCK(logger, message) \
    IMPL_HIVELOG_BLOCK_BODY(logger, message, LEVEL_TRACE)

#endif // defined(HIVELOG_DISABLE_TRACE)


// DEBUG logging
#undef HIVELOG_DEBUG
#undef HIVELOG_DEBUG_STR
#undef HIVELOG_DEBUG_BLOCK
#if defined(HIVELOG_DISABLE_DEBUG) || defined(HIVE_DOXY_MODE)
/// @name DEBUG logging
/// @{

/// @brief Send a log message at **DEBUG** level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_DEBUG(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at **DEBUG** level.
/**
@param[in] logger The logger instance.
@param[in] message The log string.
@hideinitializer
*/
#define HIVELOG_DEBUG_STR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Define *ENTER/LEAVE* block at **DEBUG** level.
/**
@param[in] logger The logger instance.
@param[in] message The block name.
@hideinitializer
*/
#define HIVELOG_DEBUG_BLOCK(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @}
#else // defined(HIVELOG_DISABLE_DEBUG)

#define HIVELOG_DEBUG(logger, message) \
    IMPL_HIVELOG_BODY(logger, message, LEVEL_DEBUG)

#define HIVELOG_DEBUG_STR(logger, message) \
    IMPL_HIVELOG_STR_BODY(logger, message, LEVEL_DEBUG)

#define HIVELOG_DEBUG_BLOCK(logger, message) \
    IMPL_HIVELOG_BLOCK_BODY(logger, message, LEVEL_DEBUG)

#endif // defined(HIVELOG_DISABLE_DEBUG)


// INFO logging
#undef HIVELOG_INFO
#undef HIVELOG_INFO_STR
#undef HIVELOG_INFO_BLOCK
#if defined(HIVELOG_DISABLE_INFO) || defined(HIVE_DOXY_MODE)
/// @name INFO logging
/// @{

/// @brief Send a log message at **INFO** level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_INFO(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at **INFO** level.
/**
@param[in] logger The logger instance.
@param[in] message The log string.
@hideinitializer
*/
#define HIVELOG_INFO_STR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Define *ENTER/LEAVE* block at **INFO** level.
/**
@param[in] logger The logger instance.
@param[in] message The block name.
@hideinitializer
*/
#define HIVELOG_INFO_BLOCK(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @}
#else // defined(HIVELOG_DISABLE_INFO)

#define HIVELOG_INFO(logger, message) \
    IMPL_HIVELOG_BODY(logger, message, LEVEL_INFO)

#define HIVELOG_INFO_STR(logger, message) \
    IMPL_HIVELOG_STR_BODY(logger, message, LEVEL_INFO)

#define HIVELOG_INFO_BLOCK(logger, message) \
    IMPL_HIVELOG_BLOCK_BODY(logger, message, LEVEL_INFO)

#endif // defined(HIVELOG_DISABLE_INFO)


// WARN logging
#undef HIVELOG_WARN
#undef HIVELOG_WARN_STR
#if defined(HIVELOG_DISABLE_WARN) || defined(HIVE_DOXY_MODE)
/// @name WARN logging
/// @{

/// @brief Send a log message at **WARN** level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_WARN(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at **WARN** level.
/**
@param[in] logger The logger instance.
@param[in] message The log string.
@hideinitializer
*/
#define HIVELOG_WARN_STR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @}
#else // defined(HIVELOG_DISABLE_WARN)

#define HIVELOG_WARN(logger, message) \
    IMPL_HIVELOG_BODY(logger, message, LEVEL_WARN)

#define HIVELOG_WARN_STR(logger, message) \
    IMPL_HIVELOG_STR_BODY(logger, message, LEVEL_WARN)

#endif // defined(HIVELOG_DISABLE_WARN)


// ERROR logging
#undef HIVELOG_ERROR
#undef HIVELOG_ERROR_STR
#if defined(HIVELOG_DISABLE_ERROR) || defined(HIVE_DOXY_MODE)
/// @name ERROR logging
/// @{

/// @brief Send a log message at **ERROR** level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_ERROR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at **ERROR** level.
/**
@param[in] logger The logger instance.
@param[in] message The log string.
@hideinitializer
*/
#define HIVELOG_ERROR_STR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @}
#else // defined(HIVELOG_DISABLE_ERROR)

#define HIVELOG_ERROR(logger, message) \
    IMPL_HIVELOG_BODY(logger, message, LEVEL_ERROR)

#define HIVELOG_ERROR_STR(logger, message) \
    IMPL_HIVELOG_STR_BODY(logger, message, LEVEL_ERROR)

#endif // defined(HIVELOG_DISABLE_ERROR)


// FATAL logging
#undef HIVELOG_FATAL
#undef HIVELOG_FATAL_STR
#if defined(HIVELOG_DISABLE_FATAL) || defined(HIVE_DOXY_MODE)
/// @name FATAL logging
/// @{

/// @brief Send a log message at **FATAL** level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_FATAL(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at **FATAL** level.
/**
@param[in] logger The logger instance.
@param[in] message The log string.
@hideinitializer
*/
#define HIVELOG_FATAL_STR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @}
#else // defined(HIVELOG_DISABLE_FATAL)

#define HIVELOG_FATAL(logger, message) \
    IMPL_HIVELOG_BODY(logger, message, LEVEL_FATAL)

#define HIVELOG_FATAL_STR(logger, message) \
    IMPL_HIVELOG_STR_BODY(logger, message, LEVEL_FATAL)

#endif // defined(HIVELOG_DISABLE_FATAL)

#endif // macro magic


///////////////////////////////////////////////////////////////////////////////
/** @page page_hive_log Logging tools

The following logging levels are defined (in order from the most important
to the less important):

|   level   |     enum constant      | macro to log message                                          | macro to disable at compile time |
|-----------|------------------------|---------------------------------------------------------------|----------------------------------|
| **FATAL** | hive::log::LEVEL_FATAL | #HIVELOG_FATAL \n #HIVELOG_FATAL_STR                          | #HIVELOG_DISABLE_FATAL           |
| **ERROR** | hive::log::LEVEL_ERROR | #HIVELOG_ERROR \n #HIVELOG_ERROR_STR                          | #HIVELOG_DISABLE_ERROR           |
| **WARN**  | hive::log::LEVEL_WARN  | #HIVELOG_WARN  \n #HIVELOG_WARN_STR                           | #HIVELOG_DISABLE_WARN            |
| **INFO**  | hive::log::LEVEL_INFO  | #HIVELOG_INFO  \n #HIVELOG_INFO_STR  \n #HIVELOG_INFO_BLOCK   | #HIVELOG_DISABLE_INFO            |
| **DEBUG** | hive::log::LEVEL_DEBUG | #HIVELOG_DEBUG \n #HIVELOG_DEBUG_STR \n #HIVELOG_DEBUG_BLOCK  | #HIVELOG_DISABLE_DEBUG           |
| **TRACE** | hive::log::LEVEL_TRACE | #HIVELOG_TRACE \n #HIVELOG_TRACE_STR \n #HIVELOG_TRACE_BLOCK  | #HIVELOG_DISABLE_TRACE           |

If your log message contains only static string (i.e. no additional parameters)
it's better to use corresponding `_STR` macro for overall application performance.
`_BLOCK` macro is used to track function calls - it produces *ENTER* and *LEAVE*
log messages.

You could disable any logging level at compile time. Note, that logging level
depends on the underlying level. For example, if you disable **INFO** level,
the **DEBUG** and **TRACE** will be disabled automatically.

It's reasonable to disable **DEBUG** and **TRACE** logging levels
for the release builds.


Targets
-------

The log target is an endpoint for all log messages. hive::log::Target is the
base class of all log targets. It also works like a **NULL** target -
all messages just disappear.

There are a few standard targets:
  - hive::log::Target::Stream sends log messages to an external stream like `std::cerr`.
  - hive::log::Target::File sends log messages to a text file.
  - hive::log::Target::Tie sends log messages to several child targets.

It is easy to create new log target by overriding
hive::log::Target::send() virtual method.

Each target may be binded with custom message format through
hive::log::Target::setFormat() which will be used for all
log messages written to this target.

It is possible to filter log messages by hive::log::Target::setMinimumLevel().
All messages with logging level less than that limit will disappear (i.e. won't
be written). Note, for hive::log::Target::Tie instances it's possible to
specify minimum logging level for each child target individually.


Formats
-------

How the log messages are written to the target is controlled by
a hive::log::Format class instance. This instance is bound to
the corresponding log target via hive::log::Target::setFormat() method.

There is simple format which is used by default:
~~~
timestamp logger level message
~~~

But it is easy to customize formatting:
  - by overriding hive::log::Format::format() virtual method
  - or by using custom format string with the `printf`-like syntax.

Have a look at the hive::log::Format class documentation for more details.


Loggers
-------

Loggers are used to separate different log streams. For each logger it's
possible to specify custom name, log target and logging level.

It's very easy to create logger instances, just provide the custom name:

~~~{.cpp}
hive::log::Logger a("/app/test/A");
hive::log::Logger b("/app/test/B");
~~~

All loggers are organized in tree hierarchy (name based with '/' separator).
The root of hierarhy is special hive::log::Logger::root() logger instance
which is also available via empty name.
For the example above the logger hiearachy will be:
  - the root logger hive::log::Logger::root()
    - `"app"`
      - `"test"`
        - `"A"`
        - `"B"`

All loggers except for hive::log::Logger::root() have
the hive::log::LEVEL_AS_PARENT logging level by default.
While logging this level is checked first,
then the target minimum logging level.
*/
