/** @file
@brief The simplest example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_basic_app
*/
#ifndef __EXAMPLES_BASIC_APP_HPP_
#define __EXAMPLES_BASIC_APP_HPP_

#include <hive/defs.hpp>
#include <hive/log.hpp>

#if !defined(HIVE_PCH)
#   include <boost/enable_shared_from_this.hpp>
#   include <boost/algorithm/string.hpp>
#   include <boost/lexical_cast.hpp>
#   include <boost/shared_ptr.hpp>
#   include <boost/asio.hpp>
#   include <boost/bind.hpp>
#endif // HIVE_PCH

#include <set>

/// @brief The simplest example.
namespace basic_app
{
    using namespace hive;


/// @brief The custom delayed task.
class DelayedTask
{
    typedef DelayedTask This; ///< @brief The type alias.

protected:

    /// @brief The main constructor.
    /**
    @param[in] ios The IO service.
    */
    DelayedTask(boost::asio::io_service &ios)
        : m_timer(ios)
        , m_started(false)
        , m_cancelled(false)
    {}

public:

    /// @brief The trivial destructor.
    virtual ~DelayedTask()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<DelayedTask> SharedPtr;

    /// @brief The factory method.
    /**
    @param[in] ios The IO service.
    @param[in] timeout_ms The timeout, milliseconds.
    @return The delayed task instance.
    */
    static SharedPtr create(boost::asio::io_service &ios, long timeout_ms)
    {
        SharedPtr pthis(new This(ios));

        if (0 < timeout_ms)
        {
            pthis->m_timer.expires_from_now(boost::posix_time::milliseconds(timeout_ms));
            pthis->m_timer.async_wait(
                boost::bind(&This::onTimer, pthis,
                boost::asio::placeholders::error));
            pthis->m_started = true;
        }
        else
        {
            // call as soon as possible
            ios.post(boost::bind(&This::onTimer, pthis,
                boost::system::error_code()));
        }

        return pthis;
    }


    /// @brief Cancel the task.
    /**
    This method cancels current task.
    */
    void cancel()
    {
        m_cancelled = true;
        if (m_started)
            m_timer.cancel();
    }

public:

    /// @brief The callback type.
    typedef boost::function0<void> Callback;


    /// @brief Call this method when task is done.
    /**
    This callback will be called when this task is finished (successful or not).

    @param[in] cb The callback to call.
    */
    void callWhenDone(Callback cb)
    {
        if (!m_callback)
            m_callback = cb;
        else
            m_callback = boost::bind(&This::tie, m_callback, cb);
    }

private:

    /// @brief Call two callbacks.
    /**
    @param[in] cb1 The first callback.
    @param[in] cb2 The second callback.
    */
    static void tie(Callback cb1, Callback cb2)
    {
        cb1();
        cb2();
    }

private:

    /// @brief Run the delayed task.
    void onTimer(boost::system::error_code err)
    {
        if (!err && !m_cancelled)
        {
            if (Callback cb = m_callback)
            {
                m_callback = Callback();
                cb(); // call the method!
            }
        }
        else
            m_callback = Callback();
    }

private:
    boost::asio::deadline_timer m_timer; ///< @brief The deadline timer.
    boost::function0<void> m_callback; ///< @brief The callback method.

    bool m_started; ///< @brief The timer "started" flag.
    bool m_cancelled; ///< @brief The "cancelled" flag.
};


/// @brief The set of custom delayed tasks.
class DelayedTaskList:
    public boost::enable_shared_from_this<DelayedTaskList>
{
    typedef DelayedTaskList This; ///< @brief The type alias.

protected:

    /// @brief The main constructor.
    /**
    @param[in] ios The IO service.
    */
    DelayedTaskList(boost::asio::io_service &ios)
        : m_ios(ios)
    {}

public:

    /// @brief The trivial destructor.
    virtual ~DelayedTaskList()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<DelayedTaskList> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] ios The IO service.
    */
    static SharedPtr create(boost::asio::io_service &ios)
    {
        return SharedPtr(new This(ios));
    }

public:

    /// @brief Cancel all delayed tasks.
    void cancelAll()
    {
        typedef std::set<DelayedTask::SharedPtr>::iterator Iterator;
        Iterator i = m_tasks.begin();
        Iterator e = m_tasks.end();
        for (; i != e; ++i)
        {
            (*i)->cancel();
        }

        m_tasks.clear();
    }

public:

    /// @brief The "call later" callback type.
    typedef DelayedTask::Callback Callback;


    /// @brief Create the delayed task.
    /**
    @param[in] timeout_ms The delay timeout, milliseconds.
    @return The delayed task instance.
    */
    DelayedTask::SharedPtr callLater(long timeout_ms)
    {
        DelayedTask::SharedPtr task = DelayedTask::create(m_ios, timeout_ms);

        m_tasks.insert(task); // push_back(task)
        task->callWhenDone(
            boost::bind(&This::onDone,
            shared_from_this(), task));

        return task;
    }


    /// @brief Call the task later.
    /**
    @param[in] timeout_ms The delay timeout, milliseconds.
    @param[in] callback The callback to call later.
    @return The delayed task instance.
    */
    DelayedTask::SharedPtr callLater(long timeout_ms, Callback callback)
    {
        DelayedTask::SharedPtr task = callLater(timeout_ms);
        task->callWhenDone(callback);
        return task;
    }


    /// @brief Call the task as soon as possible.
    /**
    @param[in] callback The callback to call later.
    @return The delayed task instance.
    */
    DelayedTask::SharedPtr callLater(Callback callback)
    {
        DelayedTask::SharedPtr task = callLater(0);
        task->callWhenDone(callback);
        return task;
    }

private:

    /// @brief The "task done" callback.
    /**
    @param[in] task The delayed task.
    */
    void onDone(DelayedTask::SharedPtr task)
    {
        m_tasks.erase(task); // remove(task)
    }

private:
    boost::asio::io_service &m_ios; ///< @brief The IO service.

    /// @brief The set of delayed tasks.
    std::set<DelayedTask::SharedPtr> m_tasks;
};


/// @brief The application skeleton.
/**
This is the base class for all other examples.
Actually it does almost nothing but contains a several key objects:

- Application::m_ios which is used by all IO operations
- Application::m_signals which is used to catch SIGTERM and SIGINT system signals
- Application::m_log which is used to log all important events

The derived classes may override the start() and stop() methods to perform
its own initialization and finilization tasks.

Use create() factory method to create new instances of the application.
See main() function for example.

@see @ref page_basic_app
*/
class Application:
    public boost::enable_shared_from_this<Application>
{
protected:

    /// @brief The default constructor.
    Application()
        : m_signals(m_ios)
        , m_log("Application")
        , m_delayed(DelayedTaskList::create(m_ios))
        , m_terminated(0)
    {
        // initialize signals
        m_signals.add(SIGTERM);
        m_signals.add(SIGINT);
        // m_signals.add(SIGBREAK); // TODO: on Windows?

        HIVELOG_TRACE_STR(m_log, "created");
    }

public:

    /// @brief The trivial destructor.
    virtual ~Application()
    {
        HIVELOG_TRACE_STR(m_log, "deleted");
    }

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Application> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] argc The number of command line arguments.
    @param[in] argv The command line arguments.
    @return The new application instance.
    */
    static SharedPtr create(int argc, const char* argv[])
    {
        return SharedPtr(new Application());
    }

public:

    /// @brief Run the application.
    void run()
    {
        try
        {
            start();

            while (!terminated())
            {
                m_ios.reset();
                m_ios.run();
            }
        }
        catch (std::exception const& ex)
        {
            HIVELOG_FATAL(m_log, ex.what());
            throw;
        }
    }

protected:

    /// @brief Is application terminated?
    /**
    @return `true` if application terminated.
    */
    bool terminated() const
    {
        return m_terminated != 0;
    }


    /// @brief Start the application.
    /**
    Resets "terminated" flag and starts listening for system signals.

    @warning Do not forget to call base method **first** when override.
    */
    virtual void start()
    {
        m_terminated = 0;
        asyncWaitForSignals();
    }


    /// @brief Stop the application.
    /**
    Stops the application: sets the "terminated" flag and cancels all asynchronous operations.

    @warning Do not forget to call base method **last** when override.
    */
    virtual void stop()
    {
        m_delayed->cancelAll();
        m_signals.cancel();
        m_terminated += 1;
        //m_ios.stop();
    }

private:

    /// @brief Start waiting for signals.
    void asyncWaitForSignals()
    {
        m_signals.async_wait(boost::bind(&Application::onGotSignal,
            shared_from_this(), boost::asio::placeholders::error,
                boost::asio::placeholders::signal_number));
        HIVELOG_DEBUG_STR(m_log, "waits for Ctrl+C...");
    }


    /// @brief The signal handler.
    /**
    @param[in] err The error code.
    @param[in] signo The signal number.
    */
    void onGotSignal(boost::system::error_code err, int signo)
    {
        if (!err)
        {
            HIVELOG_INFO(m_log, "got "
                << getSignalName(signo)
                << " signal");

            switch (signo)
            {
                case SIGTERM:
                case SIGINT:
                    m_terminated += 1; // force 'terminated' flag
                    stop();            // since stop() is virtual
                    break;

                default:
                    break;
            }

            // start again
            if (!terminated())
                asyncWaitForSignals();
        }
        else if (err == boost::asio::error::operation_aborted)
            HIVELOG_DEBUG_STR(m_log, "listening for signals aborted");
        else
        {
            HIVELOG_ERROR(m_log, "signal error: ["
                << err << "] " << err.message());
        }
    }


    /// @brief Get the signal name.
    /**
    @param[in] signo The signal number.
    @return The signal name string.
    */
    static String getSignalName(int signo)
    {
        switch (signo)
        {
            case SIGINT:    return "SIGINT";
            case SIGSEGV:   return "SIGSEGV";
            case SIGTERM:   return "SIGTERM";
            default:        break;
        }

        // just print the number
        OStringStream oss;
        oss << "#" << signo;
        return oss.str();
    }

protected:
    boost::asio::io_service m_ios; ///< @brief The IO service.
    boost::asio::signal_set m_signals; ///< @brief The signal set.
    hive::log::Logger m_log; ///< @brief The application logger.

    DelayedTaskList::SharedPtr m_delayed; ///< @brief List of delayed tasks.

private:
    volatile int m_terminated; ///< @brief The "terminated" flag.
};


/// @brief The application skeleton entry point.
/**
Creates the Application instance and calls its Application::run() method.

@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
*/
inline void main(int argc, const char* argv[])
{
    { // configure logging
        using namespace hive::log;

        Logger::root().setLevel(LEVEL_TRACE);
    }

    Application::create(argc, argv)->run();
}

} // basic_app namespace


///////////////////////////////////////////////////////////////////////////////
/** @page page_basic_app Application skeleton

This is the test application skeleton and base class for all application
examples.

It does almost nothing: handles system signals and closes the application.
But it contains a few key objects which are widely used by derived classes.

See basic_app::Application documentation for more details
or examples/basic_app.hpp file for full source code.

@example examples/basic_app.hpp
*/

#endif // __EXAMPLES_BASIC_APP_HPP_
