/** @file
@brief The simple gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_simple_gw
*/
#ifndef __EXAMPLES_SIMPLE_GW_HPP_
#define __EXAMPLES_SIMPLE_GW_HPP_

#include <DeviceHive/gateway.hpp>
#include <DeviceHive/restful.hpp>
#include <DeviceHive/websocket.hpp>
#include "basic_app.hpp"


/// @brief The simple gateway example.
namespace simple_gw
{
    using namespace hive;

/// @brief Various contants and timeouts.
enum Timeouts
{
    STREAM_RECONNECT_TIMEOUT    = 10000, ///< @brief Try to open stream device each X milliseconds.
    SERVER_RECONNECT_TIMEOUT    = 10000, ///< @brief Try to open server connection each X milliseconds.
    RETRY_TIMEOUT               = 5000,  ///< @brief Common retry timeout, milliseconds.
    DEVICE_OFFLINE_TIMEOUT      = 0
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

        String serialPortName = "";
        UInt32 serialBaudrate = 9600;

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
                std::cout << "\t--serial <serial device>\n";
                std::cout << "\t--baudrate <serial baudrate>\n";

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
            else if (boost::algorithm::iequals(argv[i], "--serial") && i+1 < argc)
                serialPortName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--baudrate") && i+1 < argc)
                serialBaudrate = boost::lexical_cast<UInt32>(argv[++i]);
        }

        if (serialPortName.empty())
            throw std::runtime_error("no serial port name provided");

        pthis->m_stream = gateway::StreamDevice::Serial::create(pthis->m_ios, serialPortName, serialBaudrate);
        pthis->m_gw_api = GatewayAPI::create(*pthis->m_stream);
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
            boost::bind(&This::tryToOpenStreamDevice,
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
        if (m_stream)
            m_stream->close();
        asyncListenForGatewayFrames(false); // stop listening to release shared pointer
        Base::stop();
    }

private:

    /// @brief Try to open stream device.
    /**
    */
    void tryToOpenStreamDevice()
    {
        if (m_stream)
        {
            m_stream->async_open(boost::bind(&This::onStreamDeviceOpen,
                shared_from_this(), _1));
        }
    }

    void onStreamDeviceOpen(boost::system::error_code err)
    {
        if (!err)
        {
            HIVELOG_INFO(m_log, "got stream device OPEN");

            asyncListenForGatewayFrames(true);
            sendGatewayRegistrationRequest();
        }
        else
        {
            HIVELOG_DEBUG(m_log, "cannot open stream device: ["
                << err << "] " << err.message());

            m_delayed->callLater(STREAM_RECONNECT_TIMEOUT,
                boost::bind(&This::tryToOpenStreamDevice,
                    shared_from_this()));
        }
    }



    /// @brief Reset the stream device.
    /**
    @brief tryToReopen if `true` then try to reopen stream device as soon as possible.
    */
    virtual void resetStreamDevice(bool tryToReopen)
    {
        HIVELOG_WARN(m_log, "stream device RESET");
        if (m_stream)
            m_stream->close();

        if (tryToReopen && !terminated())
        {
            m_delayed->callLater( // ASAP
                boost::bind(&This::tryToOpenStreamDevice,
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

            if (m_stream && m_stream->is_open())
            {
                sendGatewayRegistrationRequest();
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
                processed = sendGatewayCommand(command);
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

    /// @brief Send gateway registration request.
    void sendGatewayRegistrationRequest()
    {
        json::Value data;
        data["data"] = json::Value();

        sendGatewayMessage(gateway::INTENT_REGISTRATION_REQUEST, data);
    }


    /// @brief Send the command to the device.
    /**
    @param[in] command The command to send.
    @return `true` if command processed.
    */
    bool sendGatewayCommand(devicehive::CommandPtr command)
    {
        const int intent = m_gw.findCommandIntentByName(command->name);
        if (0 <= intent)
        {
            HIVELOG_INFO(m_log, "command: \"" << command->name
                << "\" mapped to #" << intent << " intent");

            json::Value data;
            data["id"] = command->id;
            data["parameters"] = command->params;
            if (!sendGatewayMessage(intent, data))
                throw std::runtime_error("invalid command format");
            return false; // device will report command result later!
        }
        else
            throw std::runtime_error("unknown command, ignored");

        return true;
    }


    /// @brief Send the custom gateway message.
    /**
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    @return `false` for invalid command.
    */
    bool sendGatewayMessage(int intent, json::Value const& data)
    {
        HIVELOG_INFO(m_log, "sending frame #" << intent << " data: " << data);

        if (gateway::Frame::SharedPtr frame = m_gw.jsonToFrame(intent, data))
        {
            m_gw_api->send(frame,
                boost::bind(&This::onSendGatewayFrame,
                    shared_from_this(), _1, _2));
            return true;
        }
        else
            HIVELOG_WARN_STR(m_log, "cannot convert frame to binary format");

        return false;
    }


    /// @brief The "send frame" callback.
    /**
    @param[in] err The error code.
    @param[in] frame The frame sent.
    */
    void onSendGatewayFrame(boost::system::error_code err, gateway::Frame::SharedPtr frame)
    {
        if (!err && frame)
        {
            HIVELOG_TRACE(m_log, "frame successfully sent #"
                << frame->getIntent() << ": ["
                << m_gw_api->hexdump(frame) << "], "
                << frame->size() << " bytes");
        }
        else
        {
            HIVELOG_ERROR(m_log, "failed to send frame: ["
                << err << "] " << err.message());
            resetStreamDevice(true);
        }
    }

private:

    /// @brief Start/stop listen for RX frames.
    void asyncListenForGatewayFrames(bool enable)
    {
        m_gw_api->recv(enable
            ? boost::bind(&This::onRecvGatewayFrame, shared_from_this(), _1, _2)
            : GatewayAPI::RecvFrameCallback());
    }


    /// @brief The "recv frame" callback.
    /**
    @param[in] err The error code.
    @param[in] frame The frame received.
    */
    void onRecvGatewayFrame(boost::system::error_code err, gateway::Frame::SharedPtr frame)
    {
        if (!err)
        {
            if (frame)
            {
                HIVELOG_TRACE(m_log, "frame received #"
                    << frame->getIntent() << ": ["
                    << m_gw_api->hexdump(frame) << "], "
                    << frame->size() << " bytes");

                try
                {
                    handleGatewayMessage(frame->getIntent(),
                        m_gw.frameToJson(frame));
                }
                catch (std::exception const& ex)
                {
                    HIVELOG_ERROR(m_log, "failed to handle received frame: " << ex.what());
                    resetStreamDevice(true);
                }
            }
            else
                HIVELOG_DEBUG_STR(m_log, "no frame received");
        }
        else
        {
            HIVELOG_ERROR(m_log, "failed to receive frame: ["
                << err << "] " << err.message());
            resetStreamDevice(true);
        }
    }


    /// @brief Create device from the JSON data.
    /**
    @param[in] jdev The JSON device description.
    */
    void createDevice(json::Value const& jdev)
    {
        { // create device
            const String id = jdev["id"].asString();
            const String key = jdev["key"].asString();
            const String name = jdev["name"].asString();

            devicehive::Device::ClassPtr deviceClass = devicehive::Device::Class::create("", "", false, DEVICE_OFFLINE_TIMEOUT);
            devicehive::Serializer::fromJson(jdev["deviceClass"], deviceClass);

            m_device = devicehive::Device::create(id, name, key, deviceClass, m_network);
            m_device->status = "Online";

            m_pendingNotifications.clear();
            m_deviceRegistered = false;
        }

        { // update equipment
            json::Value const& equipment = jdev["equipment"];
            const size_t N = equipment.size();
            for (size_t i = 0; i < N; ++i)
            {
                json::Value const& eq = equipment[i];

                const String code = eq["code"].asString();
                const String name = eq["name"].asString();
                const String type = eq["type"].asString();

                m_device->equipment.push_back(devicehive::Equipment::create(code, name, type));
            }
        }
    }


    /// @brief Handle the incomming message.
    /**
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    */
    void handleGatewayMessage(int intent, json::Value const& data)
    {
        HIVELOG_INFO(m_log, "process received frame #" << intent << " data: " << data);

        if (intent == gateway::INTENT_REGISTRATION_RESPONSE)
        {
            if (m_device)
                m_service->asyncUnsubscribeFromCommands(m_device);
            m_gw.handleRegisterResponse(data);
            createDevice(data);
            m_service->asyncRegisterDevice(m_device);
        }

        else if (intent == gateway::INTENT_REGISTRATION2_RESPONSE)
        {
            if (m_device)
                m_service->asyncUnsubscribeFromCommands(m_device);
            json::Value jdev = json::fromStr(data["json"].asString());
            m_gw.handleRegister2Response(jdev);
            createDevice(jdev);
            m_service->asyncRegisterDevice(m_device);
        }

        else if (intent == gateway::INTENT_COMMAND_RESULT_RESPONSE)
        {
            if (m_device)
            {
                HIVELOG_DEBUG(m_log, "got command result");

                devicehive::CommandPtr command = devicehive::Command::create();
                command->id = data["id"].asUInt();
                command->status = data["status"].asString();
                command->result = data["result"];

                m_service->asyncUpdateCommand(m_device, command);
            }
            else
                HIVELOG_WARN_STR(m_log, "got command result before registration, ignored");
        }

        else if (intent >= gateway::INTENT_USER)
        {
            if (m_device)
            {
                String name = m_gw.findNotificationNameByIntent(intent);
                if (!name.empty())
                {
                    HIVELOG_DEBUG(m_log, "got notification");

                    devicehive::NotificationPtr notification = devicehive::Notification::create(name, data["parameters"]);

                    if (m_deviceRegistered)
                    {
                        m_service->asyncInsertNotification(m_device, notification);
                    }
                    else
                    {
                        HIVELOG_DEBUG_STR(m_log, "device is not registered, notification delayed");
                        m_pendingNotifications.push_back(notification);
                    }
                }
                else
                    HIVELOG_WARN(m_log, "unknown notification: " << intent << ", ignored");
            }
            else
                HIVELOG_WARN_STR(m_log, "got notification before registration, ignored");
        }

        else
            HIVELOG_WARN(m_log, "invalid intent: " << intent << ", ignored");
    }

private:
    typedef gateway::API<gateway::StreamDevice> GatewayAPI; ///< @brief The gateway %API type.
    gateway::StreamDevicePtr m_stream;  ///< @brief The stream device.
    GatewayAPI::SharedPtr m_gw_api; ///< @brief The gateway %API.
    gateway::Engine m_gw; ///< @brief The gateway engine.

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


/// @brief The simple gateway application entry point.
/**
Creates the Application instance and calls its Application::run() method.

@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
*/
inline void main(int argc, const char* argv[])
{
    { // configure logging
        using namespace hive::log;

        Target::File::SharedPtr log_file = Target::File::create("simple_gw.log");
        Target::SharedPtr log_console = Logger::root().getTarget();
        Logger::root().setTarget(Target::Tie::create(log_file, log_console));
        Logger::root().setLevel(LEVEL_TRACE);
        Logger("/gateway/API").setTarget(log_file); // disable annoying messages
        Logger("/hive/websocket").setTarget(log_file); // disable annoying messages
        Logger("/hive/http").setTarget(log_file); // disable annoying messages
        log_console->setFormat(Format::create("%N: %M\n"));
        log_console->setMinimumLevel(LEVEL_INFO);
        log_file->setAutoFlushLevel(LEVEL_TRACE);
        log_file->setMaxFileSize(1*1024*1024);
        log_file->setNumberOfBackups(1);
        log_file->startNew();
    }

    Application::create(argc, argv)->run();
}

} // simple_gw namespace


///////////////////////////////////////////////////////////////////////////////
/** @page page_simple_gw Simple gateway example

[MSP430]: http://www.ti.com/tool/msp-exp430g2 "TI MSP430 LaunchPad"
[MSP430G2553]: http://www.ti.com/product/msp430g2553 "TI MSP430G2553 microcontroller"
[PIC16F88]: http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en010243 "Microchip PIC16F88 microcontroller"


This simple example aims to demonstrate usage of binary DeviceHive protocol
from C++ application.

In terms of DeviceHive framework this example is **Gateway** implementation:

![DeviceHive framework](@ref preview2.png)

Our target device is one of small microcontrollers (like [MSP430] development
board or [PIC16F88] based board) which runs microcode and manages the following
peripheral:
    - simple LED, allowing user to switch it *ON* or *OFF*
    - simple push button, providing user with custom asynchronous events

Because of limited resources it's impossible to use RESTful protocol on
such small devices. So device uses binary protocol to communicate with an
intermediate node - gateway. Gateway is used to convert binary messages
to the JSON messages of RESTful protocol and visa versa.

The full source code available in examples/simple_gw.hpp file.

It's highly recommended to study @ref page_simple_dev first.

Please see @ref page_start to get information about how to prepare toolchain
and build the *boost* library.


Application
===========

Application is a console program and it's reperented by an instance of
simple_gw::Application. This instance can be created using
simple_gw::Application::create() factory method. Application does all
its useful work in simple_gw::Application::run() method.

~~~{.cpp}
using namespace simple_gw;
int main(int argc, const char* argv[])
{
    Application::create(argc, argv)->run();
    return 0;
}
~~~


Binary message layout
=====================

Each binary message has it's own layout - the binary data format - according
to binary protocol specification. Simple, layout is the list of data fields
of various types (integers, string, etc.) and it's names.
Layout also may be treated as a binary<->JSON conversion rule.

Message layout is represented by gateway::Layout class. Besides system layouts
wich have known format, each endpoint device should provide layout for
device-specific messages during registration procedure.

Each message is identified by unique intent number. The gateway::LayoutManager
class manages such intent numbers and corresponding layouts.


Control flow
------------

At the start we should send registration request to the target device connected
via serial cable. Once we got registration response we register our device on
the server using simple_gw::Application::asyncRegisterDevice() method.

If registration is successful we start listening for commands from the
DeviceHive server using simple_gw::Application::asyncPollCommands() method.
Once we received command we convert it from JSON to binary format and send
converted command to the device via serial port.

At the same time we listen for notifications from serial port. Once we got
notification from device we convert it from binary to JSON format and send
converted notification to the DeviceHive server.


Command line arguments
----------------------

Application supports the following command line arguments:

|                                   Option | Description
|-----------------------------------------|-----------------------------------------------
|       `--networkName <name>`             | `<name>` is the network name
|        `--networkKey <key>`              | `<key>` is the network authentication key
|       `--networkDesc <desc>`             | `<desc>` is the network description
|            `--server <URL>`              | `<URL>` is the server URL
|            `--serial <serial device>`    | `<serial device>` is the serial port name
|          `--baudrate <serial baudrate>`  | `<serial baudrate>` is the serial port baudrate

For example, to start application run the following command:

~~~{.sh}
./simple_gw --serial COM31 --baudrate 9600
~~~


Make and run
============

To build example application just run the following command:

~~~{.sh}
make simple_gw
~~~

To build example application using custom toolchain use `CROSS_COMPILE` variable:

~~~{.sh}
make simple_gw CROSS_COMPILE=arm-unknown-linux-gnueabi-
~~~

Now you can copy executable to your gateway device and use it.


@example examples/simple_gw.hpp
*/

#endif // __EXAMPLES_SIMPLE_GW_HPP_

/*
register request: c5c301000000000076
*/
