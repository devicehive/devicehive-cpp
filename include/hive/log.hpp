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
// TODO: custom format
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
  - hive::log::LEVEL_OFF no any logging at all
  - hive::log::LEVEL_AS_PARENT is used in loggers hierarchy
*/
enum Level
{
    LEVEL_TRACE,    ///< @brief The TRACE level.
    LEVEL_DEBUG,    ///< @brief The DEBUG level.
    LEVEL_INFO,     ///< @brief The INFO level.
    LEVEL_WARN,     ///< @brief The WARN level.
    LEVEL_ERROR,    ///< @brief The ERROR level.
    LEVEL_FATAL,    ///< @brief The FATAL level.
    LEVEL_OFF,      ///< @brief No logging.
    LEVEL_AS_PARENT ///< @brief The same as parent.
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

By default uses simple format:

~~~
timestamp logger level prefix message
~~~

This format is always available through defaultFormat() static method.

The getLevelName() static method may be used to convert logging level
into the text representation.
*/
class Format
{
protected:

    /// @brief The default constructor.
    Format()
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
    @return The new simple format instance.
    */
    static SharedPtr create()
    {
        return SharedPtr(new Format());
    }

public:

    /// @brief %Format the log message.
    /**
    Uses defaultFormat() method as implementation.

    @param[in,out] os The output stream.
    @param[in] msg The log message to format.
    */
    virtual void format(OStream &os, Message const& msg)
    {
        defaultFormat(os, msg);
    }


    /// @brief %Format the log message (simple format).
    /**
    @param[in,out] os The output stream.
    @param[in] msg The log message to format.
    */
    static void defaultFormat(OStream &os, Message const& msg)
    {
        os //<< "<<<"
            << msg.timestamp << ' ';
        if (msg.loggerName)
            os << msg.loggerName << ' ';
        os << getLevelName(msg.level)
            << ' ';
        if (msg.prefix)
            os << msg.prefix;
        if (msg.message)
            os << msg.message;
        os //<< ">>>"
            << '\n';
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
            case LEVEL_TRACE:     return "TRACE";
            case LEVEL_DEBUG:     return "DEBUG";
            case LEVEL_INFO:      return "INFO";
            case LEVEL_WARN:      return "WARN";
            case LEVEL_ERROR:     return "ERROR";
            case LEVEL_FATAL:     return "FATAL";

            case LEVEL_OFF:       return "OFF";
            case LEVEL_AS_PARENT: return "PARENT";
        }

        return "UNKNOWN";
    }
};


/// @brief The log target.
/**
This is the base class for all log targets - endpoint for all log messages.
Base class does nothing and works like NULL target.

See also the inner classes for standard targets: Stderr, File and Tie.

The target uses specified log message format (see setFormat() method)
or hive::log::Format::defaultFormat() if no any format provided.
*/
class Target
{
protected:

    /// @brief The default constructor.
    Target()
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
    @return The new "NULL" target instance.
    */
    static SharedPtr create()
    {
        return SharedPtr(new Target());
    }

public:

    /// @brief Send log message to the target.
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


    /// @brief Get current message format.
    /**
    @return The current message format. May be NULL.
    */
    Format::SharedPtr getFormat() const
    {
        return m_format;
    }

public: // common targets
    class File;
    class Stderr;
    class Tie;

private:
    Format::SharedPtr m_format; ///< @brief The message format.
};


/// @brief The "File" target.
/**
Sends log messages to the file stream.

All write operations may be automatically flushed to disk but of course it
reduces the overal application performance. The "auto-flush" logging level
is used to control this process. By default it's hive::log::LEVEL_WARN,
so all WARNING, ERROR and FATAL message will be automatically flushed,
other messages (INFO, DEBUG, TRACE) will not.

A log file size limit might be specified as long as a maximum number
of log file backups. The file size limit isn't perfect precised and has the
log message boundary. That means that the whole log message will be written
although there is only one byte of free space in the log file. Once log file
is full the "backup" procedure will be started:

|   before   |    after   | comments                   |
|------------|------------|---------------------------|
| `my.log.4` |            | deleted (the oldest)       |
| `my.log.3` | `my.log.4` | rename: `log.3` -> `log.4` |
| `my.log.2` | `my.log.3` | rename: `log.2` -> `log.3` |
| `my.log.1` | `my.log.2` | rename: `log.1` -> `log.2` |
| `my.log.0` | `my.log.1` | rename: `log.0` -> `log.1` |
| `my.log`   | `my.log.0` | rename: `log` -> `log.0`   |
|            | `my.log`   | created (the newest)       |

Note, it's not recommended to provide log file size limit with no backups.

By default there is no any file size limit and therefore no backups.
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
    explicit File(String const& fileName, Level autoFlushLevel)
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

public:

    /// @brief Send log message to the target.
    /**
    @param[in] msg The log message.
    */
    virtual void send(Message const& msg) const
    {
        if (!m_file.is_open()) // try to open/reopen
        {
            if (m_needNewFile)
                startNewFile();

            m_file.clear();
            m_file.open(m_fileName.c_str(),
                std::ios::app); // append!
        }

        if (m_file.is_open()) // write message
        {
            if (Format::SharedPtr fmt = getFormat())
                fmt->format(m_file, msg);
            else
                Format::defaultFormat(m_file, msg);

            if (m_autoFlushLevel <= msg.level)
                m_file.flush();

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
                next = prev;
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


/// @brief The "Stderr" target.
/**
Sends log messages to the standard error stream.
*/
class Target::Stderr:
    public Target
{
protected:

    /// @brief The default constructor.
    explicit Stderr()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Stderr> SharedPtr;


    /// @brief The factory method.
    /**
    @return The new "Stderr" target instance.
    */
    static SharedPtr create()
    {
        return SharedPtr(new Stderr());
    }

public:

    /// @brief Send log message to the target.
    /**
    @param[in] msg The log message.
    */
    virtual void send(Message const& msg) const
    {
        if (Format::SharedPtr fmt = getFormat())
            fmt->format(std::cerr, msg);
        else
            Format::defaultFormat(std::cerr, msg);
    }
};


/// @brief The "Tie" target.
/**
Sends log messages to the several targets.

You can add many child targets using add() method.
There is no way to remove child target, it's only
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
*/
class Target::Tie:
    public Target
{
protected:

    /// @brief The default constructor.
    Tie()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Tie> SharedPtr;


    /// @brief The factory method.
    /**
    If you need more than four child targets use add() method.

    @param[in] a The first child target. May be NULL.
    @param[in] b The second child target. May be NULL.
    @param[in] c The third child target. May be NULL.
    @param[in] d The fourth child target. May be NULL.
    @return The new "Tie" target instance.
    */
    static SharedPtr create(Target::SharedPtr a = Target::SharedPtr(),
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

    /// @brief Send log message to the target.
    /**
    @param[in] msg The log message.
    */
    virtual void send(Message const& msg) const
    {
        Container::const_iterator i = m_childs.begin();
        Container::const_iterator const e = m_childs.end();
        for (; i != e; ++i)
            (*i)->send(msg);
    }

public:

    /// @brief Add child target.
    /**
    @param[in] target The child target.
    @return The self reference.
    */
    Tie& add(Target::SharedPtr target)
    {
        if (target)
            m_childs.push_back(target);
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

private:
    typedef Target::SharedPtr Child; ///< @brief The child target type.
    typedef std::vector<Child> Container; ///< @brief The list of child targets type.
    Container m_childs; ///< @brief The list of child targets.
};


/// @brief The logger.
/**
This is the main class of the logging tools.

All loggers organized into tree hierarchy. The root logger available
through root() static method. By default root() logger send messages
to the standard error stream at the hive::log::LEVEL_WARN level.
*/
class Logger
{
private:

    /// @brief Implementation.
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
        @param[in] log_name The logger name.
        @param[in] log_level The logging level.
        */
        Impl(String const& log_name, Level log_level)
            : level(log_level)
            , name(log_name)
        {}

    public:

        /// @brief The shared pointer type.
        typedef boost::shared_ptr<Impl> SharedPtr;

        /// @brief The log childs.
        std::vector<SharedPtr> childs;


        /// @brief Find child by name.
        /**
        @param[in] name The logger name.
        @return The child or NULL.
        */
        SharedPtr find(String const& name)
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
    Is used for "root" logger.

    The log level is WARN and the target is "Stderr" by default.
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
    void send(Level level, const char* message, const char *prefix = 0, const char* file = 0, int line = 0) const
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
    @return The root logger implementation.
    */
    static Impl::SharedPtr getRootImpl()
    {
        Impl::SharedPtr impl(new Impl(String(), LEVEL_WARN));
        impl->target = Target::Stderr::create();
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
At construction sends "Enter" log message.
At destruction sends "Leave" log message.

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
    Sends the "Enter" log message.

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
    Sends the "Leave" log message.
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


/// @brief Define "enter/leave" log block.
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
#define HIVELOG_DISABLE_ERROR ///< @brief Define this macro to disable ERROR and below logging at compile time.
#define HIVELOG_DISABLE_WARN  ///< @brief Define this macro to disable WARN and below logging at compile time.
#define HIVELOG_DISABLE_INFO  ///< @brief Define this macro to disable INFO and below logging at compile time.
#define HIVELOG_DISABLE_DEBUG ///< @brief Define this macro to disable DEBUG and below logging at compile time.
#define HIVELOG_DISABLE_TRACE ///< @brief Define this macro to disable TRACE and below logging at compile time.
/// @}
#endif // HIVE_DOXY_MODE


// TRACE logging
#undef HIVELOG_TRACE
#undef HIVELOG_TRACE_STR
#undef HIVELOG_TRACE_BLOCK
#if defined(HIVELOG_DISABLE_TRACE) || defined(HIVE_DOXY_MODE)
/// @name TRACE logging
/// @{

/// @brief Send a log message at TRACE level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_TRACE(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at TRACE level.
/**
@param[in] logger The logger instance.
@param[in] message The log string.
@hideinitializer
*/
#define HIVELOG_TRACE_STR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Define "enter/leave" block at TRACE level.
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

/// @brief Send a log message at DEBUG level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_DEBUG(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at DEBUG level.
/**
@param[in] logger The logger instance.
@param[in] message The log string.
@hideinitializer
*/
#define HIVELOG_DEBUG_STR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Define "enter/leave" block at DEBUG level.
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

/// @brief Send a log message at INFO level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_INFO(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at INFO level.
/**
@param[in] logger The logger instance.
@param[in] message The log string.
@hideinitializer
*/
#define HIVELOG_INFO_STR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Define "enter/leave" block at INFO level.
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

/// @brief Send a log message at WARN level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_WARN(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at WARN level.
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

/// @brief Send a log message at ERROR level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_ERROR(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at ERROR level.
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

/// @brief Send a log message at FATAL level.
/**
@param[in] logger The logger instance.
@param[in] message The log message.
@hideinitializer
*/
#define HIVELOG_FATAL(logger, message) \
    IMPL_HIVELOG_DISABLED(logger, message)

/// @brief Send a log string at FATAL level.
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

This page is under construction!
================================

If your log message contains only static string (i.e. no additional parameters)
it's better for overall performance to use corresponding `_STR` macroses.

Disable logging at compile time
-------------------------------

You can define the following macroses before include hive/log.hpp:
  - #HIVELOG_DISABLE_FATAL
  - #HIVELOG_DISABLE_ERROR
  - #HIVELOG_DISABLE_WARN
  - #HIVELOG_DISABLE_INFO
  - #HIVELOG_DISABLE_DEBUG
  - #HIVELOG_DISABLE_TRACE

Log levels are depend on the parent level. So if you disable INFO level,
the DEBUG and TRACE will be disabled automatically.

It's reasonable to disable DEBUG and TRACE logging for release builds.


Targets
-------

hive::log::Target is the base class for all targets.
There are a few standard targets:
  - hive::log::Target::File sends log messages to the text file.
  - hive::log::Target::Stderr sends log messages to the standard error stream.
  - hive::log::Target::Tie sends log messages to the several child targets.

The instances of these classes should be created with corresponding create()
factory methods.

Each target may be binded with custom message format which will be used for
all log messages.


Formats
-------

Currently the only one simple format is supported:
  - hive::log::Format::defaultFormat().


Loggers
-------

Logger is used to separate different log streams. For each logger it's possible
to specify custom name, and logging level.

All loggers are organized in tree hierarchy (logger name based).

For example, if you create the following loggers:

~~~{.cpp}
hive::log::Logger a("/app/test/A");
hive::log::Logger b("/app/test/B");
~~~

The hiearachy will be:
  - the root logger hive::log::Logger::root()
    - `"app"`
      - `"test"`
        - `"A"`
        - `"B"`

All loggers have the hive::log::LEVEL_AS_PARENT logging level by default.
*/
