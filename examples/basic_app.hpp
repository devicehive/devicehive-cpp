/** @file
@brief The simplest example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_ex00
*/
#ifndef __EXAMPLES_BASIC_APP_HPP_
#define __EXAMPLES_BASIC_APP_HPP_

#include <DeviceHive/cloud6.hpp>

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


/// @brief The simplest example.
namespace basic_app
{
    using namespace hive;

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

@see @ref page_ex00
*/
class Application:
    public boost::enable_shared_from_this<Application>
{
protected:

    /// @brief The default constructor.
    Application()
        : m_signals(m_ios)
        , m_log("Application")
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
        start();

        while (!terminated())
        {
            m_ios.reset();
            m_ios.run();
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
        m_signals.cancel();
        m_terminated += 1;
        m_ios.stop();
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
            HIVELOG_INFO(m_log, "got signal #" << signo);

            switch (signo)
            {
                case SIGTERM:
                case SIGINT:
                    stop();
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

protected:
    boost::asio::io_service m_ios; ///< @brief The IO service.
    boost::asio::signal_set m_signals; ///< @brief The signal set.
    log::Logger m_log; ///< @brief The application logger.

private:
    volatile int m_terminated; ///< @brief The "terminated" flag.
};


/// @brief The Server API submodule.
/**
This is helper class.

You have to call initServerModule() method before use any of the class methods.
The best place to do that is static factory method of your application.
*/
class ServerModule
{
    /// @brief The this type alias.
    typedef ServerModule This;

protected:

    /// @brief The main constructor.
    /**
    @param[in] ios The IO service.
    @param[in] logger The logger.
    */
    ServerModule(boost::asio::io_service &ios, log::Logger const& logger)
        : m_httpClient(http::Client::create(ios))
        , m_log_(logger)
    {}


    /// @brief The trivial destructor.
    virtual ~ServerModule()
    {}


    /// @brief Initialize server module.
    /**
    @param[in] baseUrl The server base URL.
    @param[in] pthis The this pointer.
    */
    void initServerModule(String const& baseUrl, boost::shared_ptr<ServerModule> pthis)
    {
        m_serverAPI = cloud6::ServerAPI::create(m_httpClient, baseUrl);
        m_this = pthis;
    }


    /// @brief Cancel all server tasks.
    void cancelServerModule()
    {
        m_serverAPI->cancelAll();
    }

protected:

    /// @brief Register the device asynchronously.
    /**
    @param[in] device The device to register.
    */
    virtual void asyncRegisterDevice(cloud6::DevicePtr device)
    {
        assert(!m_this.expired() && "Application is dead or not initialized");

        HIVELOG_INFO(m_log_, "register device: " << device->id);
        m_serverAPI->asyncRegisterDevice(device,
            boost::bind(&This::onRegisterDevice,
                m_this.lock(), _1, _2));
    }


    /// @brief The "register device" callback.
    /**
    Starts listening for commands from the server and starts "update" timer.

    @param[in] err The error code.
    @param[in] device The device.
    */
    virtual void onRegisterDevice(boost::system::error_code err, cloud6::DevicePtr device)
    {
        if (!err)
            HIVELOG_INFO(m_log_, "got \"register device\" response: " << device->id);
        else
        {
            HIVELOG_ERROR(m_log_, "register device error: ["
                << err << "] " << err.message());
        }
    }

protected:

    /// @brief Poll commands asynchronously.
    /**
    @param[in] device The device to poll commands for.
    */
    virtual void asyncPollCommands(cloud6::DevicePtr device)
    {
        assert(!m_this.expired() && "Application is dead or not initialized");

        HIVELOG_INFO(m_log_, "poll commands for: " << device->id);
        m_serverAPI->asyncPollCommands(device,
            boost::bind(&This::onPollCommands,
                m_this.lock(), _1, _2, _3));
    }


    /// @brief The "poll commands" callback.
    /**
    @param[in] err The error code.
    @param[in] device The device.
    @param[in] commands The list of commands.
    */
    virtual void onPollCommands(boost::system::error_code err, cloud6::DevicePtr device,
        std::vector<cloud6::Command> const& commands)
    {
        if (!err)
        {
            HIVELOG_INFO(m_log_, "got " << commands.size()
                << " commands for: " << device->id);
        }
        else if (err == boost::asio::error::operation_aborted)
            HIVELOG_DEBUG_STR(m_log_, "poll commands operation aborted");
        else
        {
            HIVELOG_ERROR(m_log_, "poll commands error: ["
                << err << "] " << err.message());
        }
    }

protected:
    http::Client::SharedPtr m_httpClient; ///< @brief The HTTP client.
    cloud6::ServerAPI::SharedPtr m_serverAPI; ///< @brief The server API.

private:
    boost::weak_ptr<ServerModule> m_this; ///< @brief The weak pointer to this.
    log::Logger m_log_; ///< @brief The module logger.
};


/// @brief The Serial module.
/**
This is helper class.

You have to call initSerialModule() method before use any of the class methods.
The best place to do that is static factory method of your application.
*/
class SerialModule
{
    ///< @brief The this type alias.
    typedef SerialModule This;

protected:

    /// @brief The main constructor.
    /**
    @param[in] ios The IO service.
    @param[in] logger The logger.
    */
    SerialModule(boost::asio::io_service &ios, log::Logger const& logger)
        : m_serialOpenTimer(ios)
        , m_serial(ios)
        , m_serialBaudrate(0)
        , m_log_(logger)
    {}


    /// @brief The trivial destructor.
    virtual ~SerialModule()
    {}


    /// @brief Initialize serial module.
    /**
    @param[in] portName The serial port name.
    @param[in] baudrate The serial baudrate.
    @param[in] pthis The this pointer.
    */
    void initSerialModule(String const& portName, UInt32 baudrate, boost::shared_ptr<SerialModule> pthis)
    {
        m_serialPortName = portName;
        m_serialBaudrate = baudrate;
        m_this = pthis;
    }


    /// @brief Cancel all serial tasks.
    void cancelSerialModule()
    {
        m_serialOpenTimer.cancel();
        m_serial.close();
    }

protected:

    /// @brief Try to open serial device asynchronously.
    /**
    @param[in] wait_sec The number of seconds to wait before open.
    */
    virtual void asyncOpenSerial(long wait_sec)
    {
        assert(!m_this.expired() && "Application is dead or not initialized");

        HIVELOG_TRACE(m_log_, "try to open serial after " << wait_sec << " seconds");
        m_serialOpenTimer.expires_from_now(boost::posix_time::seconds(wait_sec));
        m_serialOpenTimer.async_wait(boost::bind(&This::onTryToOpenSerial,
            m_this.lock(), boost::asio::placeholders::error));
    }


    /// @brief Try to open serial device.
    /**
    @return The error code.
    */
    virtual boost::system::error_code openSerial()
    {
        boost::asio::serial_port & port = m_serial;
        boost::system::error_code err;

        port.close(err); // (!) ignore error
        port.open(m_serialPortName, err);
        if (err) return err;

        // set baud rate
        port.set_option(boost::asio::serial_port::baud_rate(m_serialBaudrate), err);
        if (err) return err;

        // set character size
        port.set_option(boost::asio::serial_port::character_size(), err);
        if (err) return err;

        // set flow control
        port.set_option(boost::asio::serial_port::flow_control(), err);
        if (err) return err;

        // set stop bits
        port.set_option(boost::asio::serial_port::stop_bits(), err);
        if (err) return err;

        // set parity
        port.set_option(boost::asio::serial_port::parity(), err);
        if (err) return err;

        return err; // OK
    }


    /// @brief Try to open serial port device callback.
    /**
    This method is called as the "open serial timer" callback.

    @param[in] err The error code.
    */
    virtual void onTryToOpenSerial(boost::system::error_code err)
    {
        if (!err)
            onOpenSerial(openSerial());
        else if (err == boost::asio::error::operation_aborted)
            HIVELOG_DEBUG_STR(m_log_, "open serial device timer cancelled");
        else
        {
            HIVELOG_ERROR(m_log_, "open serial device timer error: ["
                << err << "] " << err.message());
        }
    }


    /// @brief The serial port is opened.
    /**
    @param[in] err The error code.
    */
    virtual void onOpenSerial(boost::system::error_code err)
    {
        if (!err)
        {
            HIVELOG_DEBUG(m_log_,
                "got serial device \"" << m_serialPortName
                << "\" at baudrate: " << m_serialBaudrate);
        }
        else
        {
            HIVELOG_DEBUG(m_log_, "cannot open serial device \""
                << m_serialPortName << "\": ["
                << err << "] " << err.message());
        }
    }


    /// @brief Reset the serial device.
    /**
    @brief tryToReopen if `true` then try to reopen serial as soon as possible.
    */
    virtual void resetSerial(bool tryToReopen)
    {
        HIVELOG_WARN(m_log_, "serial device reset");
        m_serial.close();

        if (tryToReopen)
            asyncOpenSerial(0); // ASAP
    }

protected:
    boost::asio::deadline_timer m_serialOpenTimer; ///< @brief Open the serial port device timer.
    boost::asio::serial_port m_serial; ///< @brief The serial port device.
    String m_serialPortName; ///< @brief The serial port name.
    UInt32 m_serialBaudrate; ///< @brief The serial baudrate.

private:
    boost::weak_ptr<SerialModule> m_this; ///< @brief The weak pointer to this.
    log::Logger m_log_; ///< @brief The module logger.
};


/// @brief The application skeleton entry point.
/**
Creates the Application instance and calls its Application::run() method.

@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
*/
inline void main(int argc, const char* argv[])
{
    log::Logger::root().setLevel(log::LEVEL_TRACE);
    Application::create(argc, argv)->run();
}

} // basic_app namespace


///////////////////////////////////////////////////////////////////////////////
/** @page page_ex00 C++ application skeleton

This is the test application skeleton and base class for all test application examples.

It does almost nothing: handles system signals and closes the application.
But it contains a few key objects which are widely used by derived classes.

See basic_app::Application documentation for more details or Examples/basic_app.hpp file for full source code.


@example examples/basic_app.hpp
*/

#endif // __EXAMPLES_BASIC_APP_HPP_
