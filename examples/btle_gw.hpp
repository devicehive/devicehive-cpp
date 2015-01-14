/** @file
@brief The BTLE gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_simple_gw
*/
#ifndef __EXAMPLES_BTLE_GW_HPP_
#define __EXAMPLES_BTLE_GW_HPP_

#include <DeviceHive/restful.hpp>
#include <DeviceHive/websocket.hpp>
#include "basic_app.hpp"

// 'libbluetooth-dev' should be installed
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/// @brief The BTLE gateway example.
namespace btle_gw
{
    using namespace hive;

/// @brief Various contants and timeouts.
enum Timeouts
{
    STREAM_RECONNECT_TIMEOUT    = 10000, ///< @brief Try to open stream device each X milliseconds.
    SERVER_RECONNECT_TIMEOUT    = 10000, ///< @brief Try to open server connection each X milliseconds.
    RETRY_TIMEOUT               = 5000,  ///< @brief Common retry timeout, milliseconds.
    DEVICE_OFFLINE_TIMEOUT      = 5
};


/// @brief The simple gateway application.
/**
This application controls only one device connected via serial port or socket or pipe!

@see @ref page_simple_gw
*/
class Application:
    public basic_app::Application,
    public devicehive::IDeviceServiceEvents
{
    typedef basic_app::Application Base; ///< @brief The base type.
    typedef Application This; ///< @brief The type alias.

public:

    /// @brief Bluetooth device.
    class BluetoothDevice:
        private hive::NonCopyable
    {
    protected:

        /// @brief Main constructor.
        BluetoothDevice(boost::asio::io_service &ios, const String &name)
            : m_ios(ios)
            , m_name(name)
            , m_dev_id(-1)
            , m_dd(-1)
        {
            memset(&m_dev_addr, 0, sizeof(m_dev_addr));
        }

    public:
        typedef boost::shared_ptr<BluetoothDevice> SharedPtr;

        /// @brief The factory method.
        static SharedPtr create(boost::asio::io_service &ios, const String &name)
        {
            return SharedPtr(new BluetoothDevice(ios, name));
        }

    public:

        /// @brief Get IO service.
        boost::asio::io_service& get_io_service()
        {
            return m_ios;
        }


        /// @brief Get bluetooth device name.
        const String& getDeviceName() const
        {
            return m_name;
        }


        /// @brief Get device identifier.
        int getDeviceId() const
        {
            return m_dev_id;
        }


        /// @brief Get bluetooth address as string.
        String getDeviceAddressStr() const
        {
            char str[64];
            ba2str(&m_dev_addr, str);
            return String(str);
        }

    public:

        /// @brief The "open" operation callback type.
        typedef boost::function1<void, boost::system::error_code> OpenCallback;


        /// @brief Is device open?
        bool is_open() const
        {
            return m_dd >= 0;
        }


        /// @brief Open device asynchronously.
        void async_open(OpenCallback callback)
        {
            boost::system::error_code err;

            if (!m_name.empty())
                m_dev_id = hci_devid(m_name.c_str());
            else
                m_dev_id = hci_get_route(NULL);

            if (m_dev_id >= 0)
            {
                if (hci_devba(m_dev_id, &m_dev_addr) >= 0)
                {
                    // TODO: try to open device
                    m_dd = 0;
                }
                else
                    err = boost::system::error_code(errno, boost::system::system_category());
            }
            else
                err = boost::system::error_code(errno, boost::system::system_category());

            m_ios.post(boost::bind(callback, err));
        }


        /// @brief Close device.
        void close()
        {
            // TODO: close device
            m_dd = -1;
            m_dev_id = -1;
            memset(&m_dev_addr, 0,
                sizeof(m_dev_addr));
        }

    private:
        boost::asio::io_service &m_ios;
        String m_name;

        bdaddr_t m_dev_addr; ///< @brief The BT address.
        int m_dev_id;        ///< @brief The device identifier.
        int m_dd;            ///< @brief The device descriptor.
    };

    /// @brief The shared pointer type.
    typedef BluetoothDevice::SharedPtr BluetoothDevicePtr;

protected:

    /// @brief The default constructor.
    Application()
        : m_disableWebsockets(false)
        , m_disableWebsocketPingPong(false)
        , m_deviceRegistered(false)
    {}

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
        SharedPtr pthis(new This());

        String networkName = "C++ network";
        String networkKey = "";
        String networkDesc = "C++ device test network";

        String baseUrl = "http://ecloud.dataart.com/ecapi8";
        size_t web_timeout = 0; // zero - don't change
        String http_version;

        String bluetoothName = "";

        // custom device properties
        for (int i = 1; i < argc; ++i) // skip executable name
        {
            if (boost::algorithm::iequals(argv[i], "--help"))
            {
                std::cout << argv[0] << " [options]";
                std::cout << "\t--networkName <network name>\n";
                std::cout << "\t--networkKey <network authentication key>\n";
                std::cout << "\t--networkDesc <network description>\n";
                std::cout << "\t--server <server URL>\n";
                std::cout << "\t--web-timeout <timeout, seconds>\n";
                std::cout << "\t--http-version <major.minor HTTP version>\n";
                std::cout << "\t--no-ws disable automatic websocket service switching\n";
                std::cout << "\t--no-ws-ping-pong disable websocket ping/pong messages\n";
                std::cout << "\t--bluetooth <BTL device name or address>\n";

                exit(1);
            }
            else if (boost::algorithm::iequals(argv[i], "--networkName") && i+1 < argc)
                networkName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkKey") && i+1 < argc)
                networkKey = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkDesc") && i+1 < argc)
                networkDesc = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--server") && i+1 < argc)
                baseUrl = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--web-timeout") && i+1 < argc)
                web_timeout = boost::lexical_cast<UInt32>(argv[++i]);
            else if (boost::algorithm::iequals(argv[i], "--http-version") && i+1 < argc)
                http_version = argv[++i];
            else if (boost::iequals(argv[i], "--no-ws"))
                pthis->m_disableWebsockets = true;
            else if (boost::iequals(argv[i], "--no-ws-ping-pong"))
                pthis->m_disableWebsocketPingPong = true;
            else if (boost::algorithm::iequals(argv[i], "--bluetooth") && i+1 < argc)
                bluetoothName = argv[++i];
        }

        pthis->m_bluetooth = BluetoothDevice::create(pthis->m_ios, bluetoothName);
        pthis->m_network = devicehive::Network::create(networkName, networkKey, networkDesc);

        if (1) // create service
        {
            http::Url url(baseUrl);

            if (boost::iequals(url.getProtocol(), "ws")
                || boost::iequals(url.getProtocol(), "wss"))
            {
                if (pthis->m_disableWebsockets)
                    throw std::runtime_error("websockets are disabled by --no-ws switch");

                HIVELOG_INFO_STR(pthis->m_log, "WebSocket service is used");
                devicehive::WebsocketService::SharedPtr service = devicehive::WebsocketService::create(
                    http::Client::create(pthis->m_ios), baseUrl, pthis);
                service->setPingPongEnabled(!pthis->m_disableWebsocketPingPong);
                if (0 < web_timeout)
                    service->setTimeout(web_timeout*1000); // seconds -> milliseconds

                pthis->m_service = service;
            }
            else
            {
                HIVELOG_INFO_STR(pthis->m_log, "RESTful service is used");
                devicehive::RestfulService::SharedPtr service = devicehive::RestfulService::create(
                    http::Client::create(pthis->m_ios), baseUrl, pthis);
                if (0 < web_timeout)
                    service->setTimeout(web_timeout*1000); // seconds -> milliseconds
                if (!http_version.empty())
                {
                    int major = 1, minor = 1;
                    parseVersion(http_version, major, minor);
                    service->setHttpVersion(major, minor);
                }

                pthis->m_service = service;
            }
        }

        return pthis;
    }


    /// @brief Get the shared pointer.
    /**
    @return The shared pointer to this instance.
    */
    SharedPtr shared_from_this()
    {
        return boost::dynamic_pointer_cast<This>(Base::shared_from_this());
    }

protected:

    /// @brief Start the application.
    /**
    Tries to open stream device.
    */
    virtual void start()
    {
        HIVELOG_TRACE_BLOCK(m_log, "start()");

        Base::start();
        m_service->asyncConnect();
        m_delayed->callLater( // ASAP
            boost::bind(&This::tryToOpenBluetoothDevice,
                shared_from_this()));
    }


    /// @brief Stop the application.
    /**
    Stops the "open" timer.
    */
    virtual void stop()
    {
        HIVELOG_TRACE_BLOCK(m_log, "stop()");

        m_service->cancelAll();
        if (m_bluetooth)
            m_bluetooth->close();

        Base::stop();
    }

private:

    /// @brief Try to open bluetooth device.
    /**
    */
    void tryToOpenBluetoothDevice()
    {
        if (m_bluetooth)
        {
            m_bluetooth->async_open(boost::bind(&This::onBluetoothDeviceOpen,
                shared_from_this(), _1));
        }
    }

    void onBluetoothDeviceOpen(boost::system::error_code err)
    {
        if (!err)
        {
            HIVELOG_INFO(m_log, "got bluetooth device OPEN: #"
                      << m_bluetooth->getDeviceId() << " "
                      << m_bluetooth->getDeviceAddressStr());
        }
        else
        {
            HIVELOG_DEBUG(m_log, "cannot open bluetooth device: ["
                << err << "] " << err.message());

            m_delayed->callLater(STREAM_RECONNECT_TIMEOUT,
                boost::bind(&This::tryToOpenBluetoothDevice,
                    shared_from_this()));
        }
    }



    /// @brief Reset the bluetooth device.
    /**
    @brief tryToReopen if `true` then try to reopen stream device as soon as possible.
    */
    virtual void resetBluetoothDevice(bool tryToReopen)
    {
        HIVELOG_WARN(m_log, "bluetooth device RESET");
        if (m_bluetooth)
            m_bluetooth->close();

        if (tryToReopen && !terminated())
        {
            m_delayed->callLater( // ASAP
                boost::bind(&This::tryToOpenBluetoothDevice,
                    shared_from_this()));
        }
    }

private: // IDeviceServiceEvents

    /// @copydoc devicehive::IDeviceServiceEvents::onConnected()
    virtual void onConnected(ErrorCode err)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onConnected()");

        if (!err)
        {
            HIVELOG_DEBUG_STR(m_log, "connected to the server");
            m_service->asyncGetServerInfo();
        }
        else
            handleServiceError(err, "connection");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onServerInfo()
    virtual void onServerInfo(boost::system::error_code err, devicehive::ServerInfo info)
    {
        if (!err)
        {
            // don't update last command timestamp on reconnect
            if (m_lastCommandTimestamp.empty())
                m_lastCommandTimestamp = info.timestamp;

            // try to switch to websocket protocol
            if (!m_disableWebsockets && !info.alternativeUrl.empty())
                if (devicehive::RestfulService::SharedPtr rest = boost::dynamic_pointer_cast<devicehive::RestfulService>(m_service))
            {
                HIVELOG_INFO(m_log, "switching to Websocket service: " << info.alternativeUrl);
                rest->cancelAll();

                devicehive::WebsocketService::SharedPtr service = devicehive::WebsocketService::create(
                    rest->getHttpClient(), info.alternativeUrl, shared_from_this());
                service->setPingPongEnabled(!m_disableWebsocketPingPong);
                service->setTimeout(rest->getTimeout());
                m_service = service;

                // connect again as soon as possible
                m_delayed->callLater(boost::bind(&devicehive::IDeviceService::asyncConnect, m_service));
                return;
            }
        }
        else
            handleServiceError(err, "getting server info");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onRegisterDevice()
    virtual void onRegisterDevice(boost::system::error_code err, devicehive::DevicePtr device)
    {
        if (m_device != device)     // if device's changed
            return;                 // just do nothing

        if (!err)
        {
            m_deviceRegistered = true;

            m_service->asyncSubscribeForCommands(m_device, m_lastCommandTimestamp);
            sendPendingNotifications();
        }
        else
            handleServiceError(err, "registering device");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onInsertCommand()
    virtual void onInsertCommand(ErrorCode err, devicehive::DevicePtr device, devicehive::CommandPtr command)
    {
        if (m_device != device)     // if device's changed
            return;                 // just do nothing

        if (!err)
        {
            m_lastCommandTimestamp = command->timestamp;
            bool processed = true;

            try
            {
                processed = handleGatewayCommand(command);
            }
            catch (std::exception const& ex)
            {
                HIVELOG_ERROR(m_log, "handle command error: "
                    << ex.what());

                command->status = "Failed";
                command->result = ex.what();
            }

            if (processed)
                m_service->asyncUpdateCommand(device, command);
        }
        else
            handleServiceError(err, "polling command");
    }

private:

    /// @brief Handle gateway command.
    bool handleGatewayCommand(devicehive::CommandPtr command)
    {
        command->status = "Success";

        if (boost::iequals(command->name, "Hello"))
            command->result = "Good to see you!";
        else
            throw std::runtime_error("Unknown command");

        return true; // processed
    }


    /// @brief Send all pending notifications.
    void sendPendingNotifications()
    {
        const size_t N = m_pendingNotifications.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " pending notifications");

        for (size_t i = 0; i < N; ++i)
        {
            m_service->asyncInsertNotification(m_device,
                m_pendingNotifications[i]);
        }
        m_pendingNotifications.clear();
    }

private:

    /// @brief Handle the service error.
    /**
    @param[in] err The error code.
    @param[in] hint The custom hint.
    */
    void handleServiceError(boost::system::error_code err, const char *hint)
    {
        if (!terminated())
        {
            HIVELOG_ERROR(m_log, (hint ? hint : "something")
                << " failed: [" << err << "] " << err.message());

            m_service->cancelAll();

            HIVELOG_DEBUG_STR(m_log, "try to connect later...");
            m_delayed->callLater(SERVER_RECONNECT_TIMEOUT,
                boost::bind(&devicehive::IDeviceService::asyncConnect, m_service));
        }
    }

private:
    BluetoothDevicePtr m_bluetooth;  ///< @brief The bluetooth device.

private:
    devicehive::IDeviceServicePtr m_service; ///< @brief The cloud service.
    bool m_disableWebsockets;       ///< @brief No automatic websocket switch.
    bool m_disableWebsocketPingPong; ///< @brief Disable websocket PING/PONG messages.

private:
    devicehive::DevicePtr m_device; ///< @brief The device.
    devicehive::NetworkPtr m_network; ///< @brief The network.
    String m_lastCommandTimestamp; ///< @brief The timestamp of the last received command.
    bool m_deviceRegistered; ///< @brief The DeviceHive cloud "registered" flag.

private:
    // TODO: list of pending commands
    std::vector<devicehive::NotificationPtr> m_pendingNotifications; ///< @brief The list of pending notification.
};


/// @brief The BTLE gateway application entry point.
/**
Creates the Application instance and calls its Application::run() method.

@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
*/
inline void main(int argc, const char* argv[])
{
    { // configure logging
        using namespace hive::log;

        Target::File::SharedPtr file = Target::File::create("btle_gw.log");
        file->setAutoFlushLevel(LEVEL_TRACE)
             .setMaxFileSize(1*1024*1024)
             .setNumberOfBackups(1)
             .startNew();

        Target::SharedPtr console = Logger::root().getTarget();
        console->setFormat(Format::create("%N: %M\n"))
                .setMinimumLevel(LEVEL_INFO);

        Logger::root().setTarget(Target::Tie::create(file, console))
                      .setLevel(LEVEL_TRACE);

        // disable annoying messages
        Logger("/hive/websocket").setTarget(file);
        Logger("/hive/http").setTarget(file);
    }

    Application::create(argc, argv)->run();
}

} // btle_gw namespace


///////////////////////////////////////////////////////////////////////////////
/** @page page_btle_gw BTLE gateway example

This page is under construction!

The libbluetooth-dev package should be installed.

@example examples/btle_gw.hpp
*/

#endif // __EXAMPLES_BTLE_GW_HPP_
