/** @file
@brief The simple gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_ex02
*/
#ifndef __EXAMPLES_SIMPLE_GW_HPP_
#define __EXAMPLES_SIMPLE_GW_HPP_

#include <DeviceHive/gateway.hpp>
#include <DeviceHive/cloud6.hpp>
#include "basic_app.hpp"


/// @brief The simple gateway example.
namespace simple_gw
{
    using namespace hive;


/// @brief The simple gateway application.
class Application:
    public basic_app::Application
{
    typedef basic_app::Application Base; ///< @brief The base type.
protected:

    /// @brief The default constructor.
    Application()
        : m_serialOpenTimer(m_ios)
        , m_serial(m_ios)
        , m_serialBaudrate(0)
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
        SharedPtr pthis(new Application());

        String networkName = "C++ network";
        String networkKey = "";
        String networkDesc = "C++ device test network";

        String baseUrl = "http://ecloud.dataart.com/ecapi6";
        String serialPortName = "COM31";
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
                std::cout << "\t--serial <serial device>\n";
                std::cout << "\t--baudrate <serial baudrate>\n";

                throw std::runtime_error("STOP");
            }
            else if (boost::algorithm::iequals(argv[i], "--networkName") && i+1 < argc)
                networkName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkKey") && i+1 < argc)
                networkKey = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkDesc") && i+1 < argc)
                networkDesc = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--server") && i+1 < argc)
                baseUrl = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--serial") && i+1 < argc)
                serialPortName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--baudrate") && i+1 < argc)
                serialBaudrate = boost::lexical_cast<UInt32>(argv[++i]);
        }

        pthis->m_api = cloud6::ServerAPI::create(http::Client::create(pthis->m_ios), baseUrl);
        pthis->m_gw = GatewayAPI::create(pthis->m_serial);
        pthis->m_serialPortName = serialPortName;
        pthis->m_serialBaudrate = serialBaudrate;
        pthis->m_networkName = networkName;
        pthis->m_networkKey = networkKey;
        pthis->m_networkDesc = networkDesc;

        return pthis;
    }


    /// @brief Get the shared pointer.
    /**
    @return The shared pointer to this instance.
    */
    SharedPtr shared_from_this()
    {
        return boost::shared_dynamic_cast<Application>(Base::shared_from_this());
    }

protected:

    /// @brief Start the application.
    /**
    Tries to open serial port.
    */
    virtual void start()
    {
        Base::start();
        asyncOpenSerial(0);
    }


    /// @brief Stop the application.
    /**
    Stops the "open" timer.
    */
    virtual void stop()
    {
        m_api->cancelAll();
        //m_gw->cancelAll();
        m_serialOpenTimer.cancel();
        m_serial.close();
        asyncListenForGatewayFrames(false); // stop listening to release shared pointer
        Base::stop();
    }

private:

    /// @brief Register the device.
    void asyncRegisterDevice()
    {
        HIVELOG_INFO(m_log, "register device: " << m_device->id);
        m_api->asyncRegisterDevice(m_device,
            boost::bind(&Application::onRegisterDevice,
                shared_from_this(), _1, _2));
    }


    /// @brief The "register device" callback.
    /**
    Starts listening for commands from the server.

    @param[in] err The error code.
    @param[in] device The device.
    */
    void onRegisterDevice(boost::system::error_code err, cloud6::DevicePtr device)
    {
        if (!err)
        {
            m_deviceRegistered = true;

            HIVELOG_INFO(m_log, "got \"register device\" response: " << device->id);
            asyncPollCommands(device);

            sendDelayedNotifications();
        }
        else
        {
            HIVELOG_ERROR(m_log, "register device error: ["
                << err << "] " << err.message());
        }
    }

private:

    /// @brief Poll commands for the device.
    /**
    @param[in] device The device.
    */
    void asyncPollCommands(cloud6::DevicePtr device)
    {
        HIVELOG_INFO(m_log, "poll commands for: " << device->id);

        m_api->asyncPollCommands(device,
            boost::bind(&Application::onPollCommands,
                shared_from_this(), _1, _2, _3));
    }


    /// @brief The "poll commands" callback.
    /**
    @param[in] err The error code.
    @param[in] device The device.
    @param[in] commands The list of commands.
    */
    void onPollCommands(boost::system::error_code err, cloud6::DevicePtr device, std::vector<cloud6::Command> const& commands)
    {
        HIVELOG_INFO(m_log, "got " << commands.size()
            << " commands for: " << device->id);

        if (!err)
        {
            // TODO: check if 'commands' is array
            typedef std::vector<cloud6::Command>::const_iterator Iterator;
            for (Iterator i = commands.begin(); i != commands.end(); ++i)
                sendGatewayCommand(*i);

            // start poll again
            asyncPollCommands(device);
        }
        else
        {
            HIVELOG_ERROR(m_log, "poll commands error: ["
                << err << "] " << err.message());
        }
    }

private:

    /// @brief Send all delayed notifications.
    void sendDelayedNotifications()
    {
        const size_t N = m_delayedNotifications.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " delayed notifications");

        for (size_t i = 0; i < N; ++i)
        {
            cloud6::Notification ntf = m_delayedNotifications[i];
            m_api->asyncSendNotification(m_device, ntf);
        }
        m_delayedNotifications.clear();
    }

private:

    /// @brief Try to open serial device asynchronously.
    /**
    @param[in] wait_sec The number of seconds to wait before open.
    */
    void asyncOpenSerial(long wait_sec)
    {
        HIVELOG_TRACE(m_log, "try to open serial"
            " after " << wait_sec << " seconds");

        m_serialOpenTimer.expires_from_now(boost::posix_time::seconds(wait_sec));
        m_serialOpenTimer.async_wait(boost::bind(&Application::onOpenSerial,
            shared_from_this(), boost::asio::placeholders::error));
    }


    /// @brief Try to open serial device.
    /**
    @return The error code.
    */
    boost::system::error_code openSerial()
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
    void onOpenSerial(boost::system::error_code err)
    {
        if (!err)
        {
            boost::system::error_code serr = openSerial();
            if (!serr)
            {
                HIVELOG_DEBUG(m_log,
                    "got serial device \"" << m_serialPortName
                    << "\" at baudrate: " << m_serialBaudrate);

                asyncListenForGatewayFrames(true);
                sendGatewayRegistrationRequest();
            }
            else
            {
                HIVELOG_DEBUG(m_log, "cannot open serial device \""
                    << m_serialPortName << "\": ["
                    << serr << "] " << serr.message());

                // try to open later
                asyncOpenSerial(10); // TODO: reconnect timeout?
            }
        }
        else if (err == boost::asio::error::operation_aborted)
            HIVELOG_DEBUG_STR(m_log, "open serial device timer cancelled");
        else
            HIVELOG_ERROR(m_log, "open serial device timer error: ["
                << err << "] " << err.message());
    }


    /// @brief Reset the serial device.
    void resetSerial()
    {
        HIVELOG_WARN(m_log, "serial device reset");
        m_serial.close();
        asyncOpenSerial(0);
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
    @param[in] cmd The command to send.
    */
    void sendGatewayCommand(cloud6::Command const& cmd)
    {
        const int intent = m_gw->findCommandIntentByName(cmd.name);
        if (0 <= intent)
        {
            HIVELOG_INFO(m_log, "command: \"" << cmd.name
                << "\" mapped to #" << intent << " intent");

            json::Value data;
            data["id"] = cmd.id;
            data["parameters"] = cmd.params;
            sendGatewayMessage(intent, data);
        }
        else
        {
            HIVELOG_WARN(m_log, "unknown command: \""
                << cmd.name << "\", ignored");
        }
    }


    /// @brief Send the custom gateway message.
    /**
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    */
    void sendGatewayMessage(int intent, json::Value const& data)
    {
        if (gateway::Frame::SharedPtr frame = m_gw->jsonToFrame(intent, data))
        {
            m_gw->send(frame,
                boost::bind(&Application::onSendGatewayFrame,
                    shared_from_this(), _1, _2));
        }
        else
            HIVELOG_WARN_STR(m_log, "cannot convert frame to binary format");
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
            HIVELOG_DEBUG(m_log, "frame successfully sent #"
                << frame->getIntent() << ": ["
                << m_gw->hexdump(frame) << "], "
                << frame->size() << " bytes");
        }
        else if (err == boost::asio::error::operation_aborted)
        {
            HIVELOG_DEBUG_STR(m_log, "TX operation cancelled");
        }
        else
        {
            HIVELOG_ERROR(m_log, "failed to send frame: ["
                << err << "] " << err.message());
            resetSerial();
        }
    }

private:

    /// @brief Start/stop listen for RX frames.
    void asyncListenForGatewayFrames(bool enable)
    {
        m_gw->recv(enable
            ? boost::bind(&Application::onRecvGatewayFrame, shared_from_this(), _1, _2)
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
                HIVELOG_DEBUG(m_log, "frame received #"
                    << frame->getIntent() << ": ["
                    << m_gw->hexdump(frame) << "], "
                    << frame->size() << " bytes");
                handleGatewayMessage(frame->getIntent(),
                    m_gw->frameToJson(frame));
            }
            else
                HIVELOG_DEBUG_STR(m_log, "no frame received");
        }
        else if (err == boost::asio::error::operation_aborted)
        {
            HIVELOG_DEBUG_STR(m_log, "RX operation cancelled");
        }
        else
        {
            HIVELOG_ERROR(m_log, "failed to receive frame: ["
                << err << "] " << err.message());
            resetSerial();
        }
    }


    /// @brief Handle the incomming message.
    /**
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    */
    void handleGatewayMessage(int intent, json::Value const& data)
    {
        HIVELOG_INFO(m_log, "process intent #" << intent << " data: " << data << "\n");

        if (intent == gateway::INTENT_REGISTRATION_RESPONSE)
        {
            m_gw->handleRegisterResponse(data);

            { // create device
                String id = data["id"].asString();
                String key = data["key"].asString();
                String name = data["name"].asString();

                // TODO: get network from command line
                cloud6::NetworkPtr network = cloud6::Network::create(
                    m_networkName, m_networkKey, m_networkDesc);

                cloud6::Device::ClassPtr deviceClass = cloud6::Device::Class::create("", "", false, 10);
                cloud6::ServerAPI::Serializer::json2deviceClass(data["deviceClass"], deviceClass);

                m_device = cloud6::Device::create(id, name, key, deviceClass, network);
                m_device->status = "Online";

                m_delayedNotifications.clear();
                m_deviceRegistered = false;
            }

            { // update equipment
                json::Value const& equipment = data["equipment"];
                const size_t N = equipment.size();
                for (size_t i = 0; i < N; ++i)
                {
                    json::Value const& eq = equipment[i];

                    String id = eq["code"].asString();
                    String name = eq["name"].asString();
                    String type = eq["type"].asString();

                    m_device->equipment.push_back(cloud6::Equipment::create(id, name, type));
                }
            }

            asyncRegisterDevice();
        }

        else if (intent == gateway::INTENT_COMMAND_RESULT_RESPONSE)
        {
            if (m_device)
            {
                HIVELOG_DEBUG(m_log, "got command result");

                cloud6::Command cmd;
                cmd.id = data["id"].asUInt();
                cmd.status = data["status"].asString();
                cmd.result = data["result"].asString();

                m_api->asyncSendCommandResult(m_device, cmd);
            }
            else
                HIVELOG_WARN_STR(m_log, "got command result before registration, ignored");
        }

        else if (intent >= gateway::INTENT_USER)
        {
            if (m_device)
            {
                String name = m_gw->findNotificationNameByIntent(intent);
                if (!name.empty())
                {
                    HIVELOG_DEBUG(m_log, "got notification");

                    cloud6::Notification ntf;
                    ntf.name = name;
                    ntf.params = data;

                    if (m_deviceRegistered)
                        m_api->asyncSendNotification(m_device, ntf);
                    else
                    {
                        HIVELOG_DEBUG_STR(m_log, "device is not registered, notification delayed");
                        m_delayedNotifications.push_back(ntf);
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
    boost::asio::deadline_timer m_serialOpenTimer; ///< @brief Open the serial port device timer.
    boost::asio::serial_port m_serial; ///< @brief The serial port device.
    String m_serialPortName; ///< @brief The serial port name.
    UInt32 m_serialBaudrate; ///< @brief The serial baudrate.

    String m_networkName; ///< @brief The network name.
    String m_networkKey; ///< @brief The network key.
    String m_networkDesc; ///< @brief The network description.

private:
    typedef gateway::API<boost::asio::serial_port> GatewayAPI; ///< @brief The gateway API type.
    GatewayAPI::SharedPtr m_gw; ///< @brief The gateway API.

private:
    cloud6::DevicePtr m_device; ///< @brief The device.
    bool m_deviceRegistered; ///< @brief The DeviceHive cloud "registered" flag.
    std::vector<cloud6::Notification> m_delayedNotifications; ///< @brief The list of delayed notification.

private:
    cloud6::ServerAPI::SharedPtr m_api; ///< @brief The cloud server API.
};

} // simple_gw namespace


///////////////////////////////////////////////////////////////////////////////
/** @page page_ex02 C++ gateway example

[MSP430]: http://www.ti.com/tool/msp-exp430g2 "TI MSP430 LaunchPad"
[MSP430G2553]: http://www.ti.com/product/msp430g2553 "TI MSP430G2553 microcontroller"
[PIC16F88]: http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en010243 "Microchip PIC16F88 microcontroller"


This simple example aims to demonstrate usage of binary DeviceHive protocol from C++ application.

In terms of DeviceHive framework this example is **gateway** implementation:

![DeviceHive framework](@ref preview2.png)

Our target device is one of small microcontrollers (like [MSP430] development board or [PIC16F88] based board)
which runs microcode and manages the following peripheral:
    - simple LED, allowing user to switch it *ON* or *OFF*
    - simple push button, providing user with custom asynchronous events


Because of limited resources it's impossible to use RESTful protocol on such small devices.
So device uses binary protocol to communicate with an intermediate node - gateway.
Gateway is used to convert binary messages to the JSON messages of RESTful protocol and visa versa.

The full source code available in Examples/simple_gw.hpp file.

It's highly recommended to study @ref page_ex01 first.


Requirements {#page_ex02_requirements}
======================================

Please take a look at corresponding @ref page_ex01_requirements section of @ref page_ex01.
All requirements are exactly the same.


Application {#page_ex02_application}
====================================

Application is a console program and it's reperented by an instance of simple_gw::Application.
This instance can be created using simple_gw::Application::create() factory method.
Application does all its useful work in simple_gw::Application::run() method.

~~~{.cpp}
using namespace simple_gw;
int main(int argc, const char* argv[])
{
    Application::create(argc, argv)->run();
    return 0;
}
~~~


Binary message layout {#page_ex02_layout}
=========================================

Each binary message has it's own layout - the binary data format - according to binary protocol specification.
Simple, layout is the list of data fields of various types (integers, string, etc.) and it's names.
Layout also may be treated as a binary<->JSON conversion rule.

Message layout is represented by gateway::Layout class. Besides system layouts wich have known format,
each endpoint device should provide layout for device-specific messages during registration procedure.

Each message is identified by unique intent number. The gateway::LayoutManager class manages
such intent numbers and corresponding layouts.


Control flow {#page_ex02_application_flow}
------------------------------------------

At the start we should send registration request to the target device connected via serial cable.
Once we got registration response we register our device on the server using simple_gw::Application::asyncRegisterDevice() method.

If registration is successful we start listening for commands from the DeviceHive server using simple_gw::Application::asyncPollCommands() method.
Once we received command we convert it from JSON to binary format and send converted command to the device via serial port.

At the same time we listen for notifications from serial port. Once we got notification from device
we convert it from binary to JSON format and send converted notification to the DeviceHive server.


Command line arguments {#page_ex02_application_cmdline}
-------------------------------------------------------

Application supports the following command line options:

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
./xtest --serial COM31 --baudrate 9600
~~~


Make and run {#page_ex02_make_and_run}
======================================

To build example application just run the following command:

~~~{.sh}
make xtest-02
~~~

To build example application using custom toolchain use `CROSS_COMPILE` variable:

~~~{.sh}
make xtest-02 CROSS_COMPILE=arm-unknown-linux-gnueabi-
~~~

Now you can copy executable to your gateway device and use it.


@example Examples/simple_gw.hpp
*/

#endif // __EXAMPLES_SIMPLE_GW_HPP_

/*
register request:   C5C30100000000000000000000000000
                    c5c301000000000076
*/
