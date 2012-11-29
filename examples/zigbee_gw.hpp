/** @file
@brief The ZigBee gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
//@see @ref page_ex03
#ifndef __EXAMPLES_ZIGBEE_GW_HPP_
#define __EXAMPLES_ZIGBEE_GW_HPP_

#include <DeviceHive/gateway.hpp>
#include <DeviceHive/cloud6.hpp>
#include <DeviceHive/xbee.hpp>
#include "basic_app.hpp"


/// @brief The ZigBee gateway example.
namespace zigbee_gw
{
    using namespace hive;


/// @brief The ZigBee gateway application.
class Application:
    public basic_app::Application
{
    typedef basic_app::Application Base; ///< @brief The base class.
protected:

    /// @brief The default constructor.
    Application()
        : m_serialOpenTimer(m_ios)
        , m_serial(m_ios)
        , m_serialBaudrate(0)
        , m_xbeeFrameId(1)
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

        String networkName = "XBee C++ network";
        String networkKey = "";
        String networkDesc = "C++ device test network";

        String baseUrl = "http://ecloud.dataart.com/ecapi6";
        String serialPortName = "COM33";
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
                std::cout << "\t--serial <serial device name>\n";
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
        pthis->m_xbee = XBeeAPI::create(pthis->m_serial);
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
        //m_xbee->cancelAll();
        m_serialOpenTimer.cancel();
        m_serial.close();
        asyncListenForXBeeFrames(false); // stop listening to release shared pointer
        Base::stop();
    }

private:

    enum Const
    {
        XBEE_BROADCAST64 = 0xFFFF, ///< @brief The XBee broadcast MAC address.
        XBEE_BROADCAST16 = 0xFFFE, ///< @brief The XBee broadcast network address.
        XBEE_FRAGMENTATION = 256   ///< @brief The maximum XBee API payload size.
    };

private:

    /// @brief The remote device.
    /**
    */
    class RemoteDevice
    {
    public:
        UInt64 address64; ///< @brief The MAC address.
        UInt16 address16; ///< @brief The network address.
        boost::asio::streambuf m_rx_buf; ///< @brief The RX stream buffer.

        cloud6::DevicePtr device; ///< @brief The device.
        bool deviceRegistered;           ///< @brief The "registered" flag.
        std::vector<cloud6::Notification> delayedNotifications; ///< @brief The list of delayed notification.
        gateway::Engine gw; ///< @brief The gateway engine.

        /// @brief The default constructor.
        RemoteDevice()
            : address64(XBEE_BROADCAST64)
            , address16(XBEE_BROADCAST16)
            , deviceRegistered(false)
        {}
    };

    /// @brief The remote device shared pointer type.
    typedef boost::shared_ptr<RemoteDevice> RemoteDeviceSPtr;

private:

    /// @brief The list of remote devices.
    std::map<UInt64, RemoteDeviceSPtr> m_remoteDevices;

    /// @brief Get or create new remote device.
    /**
    @param[in] address The remote device MAC address.
    @return The remote device structure.
    */
    RemoteDeviceSPtr getRemoteDevice(UInt64 address)
    {
        if (RemoteDeviceSPtr rdev = m_remoteDevices[address])
            return rdev;
        else
        {
            rdev.reset(new RemoteDevice());
            m_remoteDevices[address] = rdev;
            rdev->address64 = address;
            return rdev;
        }
    }

private:

    /// @brief Register the device.
    /**
    @param[in] rdev The remote device to register.
    */
    void asyncRegisterDevice(RemoteDeviceSPtr rdev)
    {
        HIVELOG_INFO(m_log, "register device: " << rdev->device->id);
        m_api->asyncRegisterDevice(rdev->device,
            boost::bind(&Application::onRegisterDevice,
                shared_from_this(), _1, _2, rdev));
    }


    /// @brief The "register device" callback.
    /**
    Starts listening for commands from the server.

    @param[in] err The error code.
    @param[in] rdev The corresponding remote device.
    */
    void onRegisterDevice(boost::system::error_code err, cloud6::DevicePtr, RemoteDeviceSPtr rdev)
    {
        if (!err)
        {
            rdev->deviceRegistered = true;

            HIVELOG_INFO(m_log, "got \"register device\" response: " << rdev->device->id);
            asyncPollCommands(rdev);

            sendDelayedNotifications(rdev);
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
    @param[in] rdev The remote device.
    */
    void asyncPollCommands(RemoteDeviceSPtr rdev)
    {
        HIVELOG_INFO(m_log, "poll commands for: " << rdev->device->id);

        m_api->asyncPollCommands(rdev->device,
            boost::bind(&Application::onPollCommands,
                shared_from_this(), _1, _2, _3, rdev));
    }


    /// @brief The "poll commands" callback.
    /**
    @param[in] err The error code.
    @param[in] commands The list of commands.
    @param[in] rdev The remote device.
    */
    void onPollCommands(boost::system::error_code err, cloud6::DevicePtr, std::vector<cloud6::Command> const& commands, RemoteDeviceSPtr rdev)
    {
        HIVELOG_INFO(m_log, "got " << commands.size()
            << " commands for: " << rdev->device->id);

        if (!err)
        {
            typedef std::vector<cloud6::Command>::const_iterator Iterator;
            for (Iterator i = commands.begin(); i != commands.end(); ++i)
                sendGatewayCommand(rdev, *i);

            // start poll again
            asyncPollCommands(rdev);
        }
        else
        {
            HIVELOG_ERROR(m_log, "poll commands error: ["
                << err << "] " << err.message());
        }
    }

private:

    /// @brief Send all delayed notifications.
    /**
    @param[in] rdev The remote device.
    */
    void sendDelayedNotifications(RemoteDeviceSPtr rdev)
    {
        const size_t N = rdev->delayedNotifications.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " delayed notifications");

        for (size_t i = 0; i < N; ++i)
        {
            cloud6::Notification ntf = rdev->delayedNotifications[i];
            m_api->asyncSendNotification(rdev->device, ntf);
        }
        rdev->delayedNotifications.clear();
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

                asyncListenForXBeeFrames(true);
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

        sendGatewayMessage(RemoteDeviceSPtr(), gateway::INTENT_REGISTRATION_REQUEST, data);
    }


    /// @brief Send the command to the device.
    /**
    @param[in] rdev The remote device.
    @param[in] cmd The command to send.
    */
    void sendGatewayCommand(RemoteDeviceSPtr rdev, cloud6::Command const& cmd)
    {
        const int intent = rdev->gw.findCommandIntentByName(cmd.name);
        if (0 <= intent)
        {
            HIVELOG_INFO(m_log, "command: \"" << cmd.name
                << "\" mapped to #" << intent << " intent");

            json::Value data;
            data["id"] = cmd.id;
            data["parameters"] = cmd.params;
            sendGatewayMessage(rdev, intent, data);
        }
        else
        {
            HIVELOG_WARN(m_log, "unknown command: \""
                << cmd.name << "\", ignored");
        }
    }


    /// @brief Send the custom gateway message.
    /**
    @param[in] rdev The remote device.
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    */
    void sendGatewayMessage(RemoteDeviceSPtr rdev, int intent, json::Value const& data)
    {
        const gateway::Engine &gw = rdev ? rdev->gw : m_gw;
        if (gateway::Frame::SharedPtr frame = gw.jsonToFrame(intent, data))
        {
            if (rdev)
                sendGatewayFrame(frame, rdev->address64, rdev->address16);
            else
                sendGatewayFrame(frame, XBEE_BROADCAST64, XBEE_BROADCAST16);
        }
        else
            HIVELOG_WARN_STR(m_log, "cannot convert frame to binary format");
    }


    /// @brief Send the gateway frame.
    /**
    @param[in] payload The custom gateway message payload.
    @param[in] da64 The destination MAC address.
    @param[in] da16 The destination network address.
    */
    void sendGatewayFrame(gateway::Frame::SharedPtr frame, UInt64 da64, UInt16 da16)
    {
        if (frame && !frame->empty())
        {
            const size_t len = frame->size();
            if (XBEE_FRAGMENTATION < len) // do fragmentation
            {
                std::vector<UInt8>::const_iterator data = frame->getContent().begin();
                const size_t N = len/XBEE_FRAGMENTATION - 1; // number of "full" frames

                for (size_t i = 0 ; i < N; ++i)
                {
                    xbee::Frame::ZBTransmitRequest payload(String(data, data + XBEE_FRAGMENTATION), m_xbeeFrameId++, da64, da16);
                    sendXBeeFrame(payload);
                    data += XBEE_FRAGMENTATION;
                }

                // send the rest of frames
                const size_t rest = (len - N*XBEE_FRAGMENTATION);
                const size_t len1 = rest/2;
                const size_t len2 = rest - len1;

                xbee::Frame::ZBTransmitRequest payload1(String(data, data + len1), m_xbeeFrameId++, da64, da16);
                data += len1;

                xbee::Frame::ZBTransmitRequest payload2(String(data, data + len2), m_xbeeFrameId++, da64, da16);
                data += len2;

                sendXBeeFrame(payload1);
                sendXBeeFrame(payload2);
            }
            else // just one frame
            {
                const std::vector<UInt8> &data = frame->getContent();
                xbee::Frame::ZBTransmitRequest payload(String(data.begin(), data.end()), m_xbeeFrameId++, da64, da16);
                sendXBeeFrame(payload);
            }
        }
    }

private:

    /// @brief Send the custom XBee frame.
    /**
    @param[in] payload The custom XBee message payload.
    */
    template<typename PayloadT>
    void sendXBeeFrame(PayloadT const& payload)
    {
        m_xbee->send(xbee::Frame::create(payload),
            boost::bind(&Application::onSendXBeeFrame,
                shared_from_this(), _1, _2));
    }


    /// @brief The "send frame" callback.
    /**
    @param[in] err The error code.
    @param[in] frame The frame sent.
    */
    void onSendXBeeFrame(boost::system::error_code err, xbee::Frame::SharedPtr frame)
    {
        if (!err && frame)
        {
            HIVELOG_DEBUG(m_log, "frame successfully sent #"
                << frame->getIntent() << ": ["
                << m_xbee->hexdump(frame) << "], "
                << frame->size() << " bytes:\n"
                << xbee::Debug::dump(frame));
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
    void asyncListenForXBeeFrames(bool enable)
    {
        m_xbee->recv(enable
            ? boost::bind(&Application::onRecvXBeeFrame, shared_from_this(), _1, _2)
            : XBeeAPI::RecvFrameCallback());
    }


    /// @brief The "recv frame" callback.
    /**
    @param[in] err The error code.
    @param[in] frame The frame received.
    */
    void onRecvXBeeFrame(boost::system::error_code err, xbee::Frame::SharedPtr frame)
    {
        if (!err)
        {
            if (frame)
            {
                HIVELOG_DEBUG(m_log, "frame received #"
                    << frame->getIntent() << ": ["
                    << m_xbee->hexdump(frame) << "], "
                    << frame->size() << " bytes:\n"
                    << xbee::Debug::dump(frame));
                handleRecvXBeeFrame(frame);
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

private:

    /// @brief Handle the received frame.
    /**
    @param[in] frame The frame.
    */
    void handleRecvXBeeFrame(xbee::Frame::SharedPtr frame)
    {
        switch (frame->getIntent())
        {
            case xbee::Frame::ZB_RECEIVE_PACKET:
            {
                xbee::Frame::ZBReceivePacket payload;
                if (frame->getPayload(payload))
                {
                    boost::shared_ptr<RemoteDevice> rdev = getRemoteDevice(payload.srcAddr64);
                    rdev->address16 = payload.srcAddr16;

                    { // put the data to the stream buffer
                        OStream os(&rdev->m_rx_buf);
                        os.write(payload.data.data(), payload.data.size());
                    }

                    HIVELOG_DEBUG(m_log, "got [" << dump::hex(payload.data) << "] from "
                        << dump::hex(payload.srcAddr64) << "/" << dump::hex(payload.srcAddr16));

                    gateway::Frame::ParseResult result = gateway::Frame::RESULT_SUCCESS;
                    if (gateway::Frame::SharedPtr frame = gateway::Frame::parseFrame(rdev->m_rx_buf, &result))
                    {
                        handleGatewayMessage(frame->getIntent(),
                            rdev->gw.frameToJson(frame), rdev);
                    }
                }
            } break;

            default:
                // do nothing
                break;
        }
    }

    /// @brief Handle the incomming message.
    /**
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    @param[in] rdev The remote device.
    */
    void handleGatewayMessage(int intent, json::Value const& data, RemoteDeviceSPtr rdev)
    {
        HIVELOG_INFO(m_log, "process intent #" << intent << " data: " << data << "\n");

        if (intent == gateway::INTENT_REGISTRATION_RESPONSE)
        {
            rdev->gw.handleRegisterResponse(data);

            { // create device
                String id = data["id"].asString();
                String key = data["key"].asString();
                String name = data["name"].asString();

                // TODO: get network from command line
                cloud6::NetworkPtr network = cloud6::Network::create(
                    m_networkName, m_networkKey, m_networkDesc);

                cloud6::Device::ClassPtr deviceClass = cloud6::Device::Class::create("", "", false, 10);
                cloud6::ServerAPI::Serializer::json2deviceClass(data["deviceClass"], deviceClass);

                rdev->device = cloud6::Device::create(id, name, key, deviceClass, network);
                rdev->device->status = "Online";

                rdev->delayedNotifications.clear();
                rdev->deviceRegistered = false;
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

                    rdev->device->equipment.push_back(cloud6::Equipment::create(id, name, type));
                }
            }

            asyncRegisterDevice(rdev);
        }

        else if (intent == gateway::INTENT_COMMAND_RESULT_RESPONSE)
        {
            if (rdev->device)
            {
                HIVELOG_DEBUG(m_log, "got command result");

                cloud6::Command cmd;
                cmd.id = data["id"].asUInt();
                cmd.status = data["status"].asString();
                cmd.result = data["result"].asString();

                m_api->asyncSendCommandResult(rdev->device, cmd);
            }
            else
                HIVELOG_WARN_STR(m_log, "got command result before registration, ignored");
        }

        else if (intent >= gateway::INTENT_USER)
        {
            if (rdev->device)
            {
                String name = rdev->gw.findNotificationNameByIntent(intent);
                if (!name.empty())
                {
                    HIVELOG_DEBUG(m_log, "got notification");

                    cloud6::Notification ntf;
                    ntf.name = name;
                    ntf.params = data;

                    if (rdev->deviceRegistered)
                        m_api->asyncSendNotification(rdev->device, ntf);
                    else
                    {
                        HIVELOG_DEBUG_STR(m_log, "device is not registered, notification delayed");
                        rdev->delayedNotifications.push_back(ntf);
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
    cloud6::ServerAPI::SharedPtr m_api; ///< @brief The cloud server %API.
    gateway::Engine m_gw; ///< @brief The gateway engine for system intents.

private:
    typedef xbee::API<boost::asio::serial_port> XBeeAPI; ///< @brief The XBee %API type.
    XBeeAPI::SharedPtr m_xbee; ///< @brief The XBee %API.
    size_t m_xbeeFrameId; ///< @brief The XBee TX frame identifier.
};


/// @brief The ZigBee gateway application entry point.
/**
Creates the Application instance and calls its Application::run() method.

@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
*/
inline void main(int argc, const char* argv[])
{
    { // configure logging
        log::Target::SharedPtr log_file = log::Target::File::create("zigbee_gw.log");
        log::Logger::root().setTarget(log::Target::Tie::create(
            log_file, log::Logger::root().getTarget()));
        log::Logger::root().setLevel(log::LEVEL_TRACE);
        log::Logger("/hive/http").setTarget(log_file).setLevel(log::LEVEL_DEBUG); // disable annoying messages
    }

    Application::create(argc, argv)->run();
}

} // zigbee_gw namespace


///////////////////////////////////////////////////////////////////////////////
/* @page page_ex03 C++ ZigBee gateway example

!!!UNDER CONSTRUCTION!!!


@example examples/zigbee_gw.hpp
*/

#endif // __EXAMPLES_ZIGBEE_GW_HPP_
