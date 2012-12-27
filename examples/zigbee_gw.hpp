/** @file
@brief The ZigBee gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_zigbee_gw
*/
#ifndef __EXAMPLES_ZIGBEE_GW_HPP_
#define __EXAMPLES_ZIGBEE_GW_HPP_

#include <DeviceHive/gateway.hpp>
#include <DeviceHive/cloud7.hpp>
#include <DeviceHive/xbee.hpp>
#include "basic_app.hpp"


/// @brief The ZigBee gateway example.
namespace zigbee_gw
{
    using namespace hive;

/// @brief Various contants and timeouts.
enum
{
    SERIAL_RECONNECT_TIMEOUT    = 10, ///< @brief Try to open serial port each X seconds.
    SERVER_RECONNECT_TIMEOUT    = 10, ///< @brief Try to open server connection each X seconds.
    DEVICE_OFFLINE_TIMEOUT      = 0
};


/// @brief The ZigBee gateway application.
/**
Uses DeviceHive REST server interface.
*/
class Application:
    public basic_app::Application,
    public cloud6::ServerModuleREST,
    public gateway::SerialModule
{
    typedef basic_app::Application Base; ///< @brief The base class.
    typedef Application This; ///< @brief The this type alias.
protected:

    /// @brief The default constructor.
    Application()
        : ServerModuleREST(m_ios, m_log)
        , SerialModule(m_ios, m_log)
        , m_xbeeFrameId(1)
    {
        HIVELOG_INFO_STR(m_log, "REST service is used");
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
        SharedPtr pthis(new This());

        String networkName = "XBee C++ network";
        String networkKey = "";
        String networkDesc = "C++ device test network";

        String baseUrl = "http://ecloud.dataart.com/ecapi7";
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

        if (serialPortName.empty())
            throw std::runtime_error("no serial port name provided");

        pthis->m_xbee = XBeeAPI::create(pthis->m_serial);
        pthis->m_networkName = networkName;
        pthis->m_networkKey = networkKey;
        pthis->m_networkDesc = networkDesc;

        pthis->initServerModuleREST(baseUrl, pthis);
        pthis->initSerialModule(serialPortName, serialBaudrate, pthis);
        return pthis;
    }


    /// @brief Get the shared pointer.
    /**
    @return The shared pointer to this instance.
    */
    SharedPtr shared_from_this()
    {
        return boost::shared_dynamic_cast<This>(Base::shared_from_this());
    }

protected:

    /// @brief Start the application.
    /**
    Tries to open serial port.
    */
    virtual void start()
    {
        Base::start();
        asyncOpenSerial(0); //ASAP
    }


    /// @brief Stop the application.
    /**
    Stops the "open" timer.
    */
    virtual void stop()
    {
        cancelServerModuleREST();
        cancelSerialModule();
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


    /// @brief The ZigBee device.
    /**
    */
    class ZDevice
    {
    public:
        UInt64 address64; ///< @brief The MAC address.
        UInt16 address16; ///< @brief The network address.
        boost::asio::streambuf m_rx_buf; ///< @brief The RX stream buffer.

        cloud6::DevicePtr device; ///< @brief The corresponding device.
        bool deviceRegistered;    ///< @brief The "registered" flag.
        std::vector<cloud6::Notification> delayedNotifications; ///< @brief The list of delayed notification.
        gateway::Engine gw; ///< @brief The gateway engine.

        /// @brief The default constructor.
        ZDevice()
            : address64(XBEE_BROADCAST64)
            , address16(XBEE_BROADCAST16)
            , deviceRegistered(false)
        {}
    };

    /// @brief The ZigBee device shared pointer type.
    typedef boost::shared_ptr<ZDevice> ZDeviceSPtr;

private:

    /// @brief The list of ZigBee devices.
    std::map<UInt64, ZDeviceSPtr> m_devices;

    /// @brief Get or create new ZigBee device.
    /**
    @param[in] address The device MAC address.
    @return The ZigBee device.
    */
    ZDeviceSPtr getZDevice(UInt64 address)
    {
        if (ZDeviceSPtr zdev = m_devices[address])
            return zdev;
        else
        {
            zdev.reset(new ZDevice());
            m_devices[address] = zdev;
            zdev->address64 = address;
            return zdev;
        }
    }


    /// @brief Find ZigBee device.
    /**
    @param[in] device The device to find.
    @return The ZigBee device or NULL.
    */
    ZDeviceSPtr findZDevice(cloud6::DevicePtr device) const
    {
        typedef std::map<UInt64, ZDeviceSPtr>::const_iterator Iterator;
        for (Iterator i = m_devices.begin(); i != m_devices.end(); ++i)
        {
            ZDeviceSPtr zdev = i->second;
            if (zdev->device == device)
                return zdev;
        }

        return ZDeviceSPtr(); // not found
    }

private: // ServerModuleREST

    /// @copydoc cloud6::ServerModuleREST::onRegisterDevice()
    virtual void onRegisterDevice(boost::system::error_code err, cloud6::DevicePtr device)
    {
        ServerModuleREST::onRegisterDevice(err, device);

        if (!err)
        {
            if (ZDeviceSPtr zdev = findZDevice(device))
            {
                zdev->deviceRegistered = true;
                sendDelayedNotifications(zdev);
                asyncPollCommands(device);
            }
            else
                HIVELOG_ERROR(m_log, "unknown device");
        }
    }


    /// @copydoc cloud6::ServerModuleREST::onPollCommands()
    void onPollCommands(boost::system::error_code err, cloud6::DevicePtr device, std::vector<cloud6::Command> const& commands)
    {
        ServerModuleREST::onPollCommands(err, device, commands);

        if (!err)
        {
            if (ZDeviceSPtr zdev = findZDevice(device))
            {
                typedef std::vector<cloud6::Command>::const_iterator Iterator;
                for (Iterator i = commands.begin(); i != commands.end(); ++i)
                {
                    cloud6::Command cmd = *i; // copy to modify
                    bool processed = true;

                    try
                    {
                        processed = sendGatewayCommand(zdev, *i);
                    }
                    catch (std::exception const& ex)
                    {
                        HIVELOG_ERROR(m_log, "handle command error: "
                            << ex.what());

                        cmd.status = "Failed";
                        cmd.result = ex.what();
                    }

                    if (processed)
                        asyncUpdateCommand(device, cmd);
                }

                asyncPollCommands(device); // start poll again
            }
            else
                HIVELOG_ERROR(m_log, "unknown device");
        }
    }


    /// @brief Send all delayed notifications.
    /**
    @param[in] zdev The ZigBee device.
    */
    void sendDelayedNotifications(ZDeviceSPtr zdev)
    {
        const size_t N = zdev->delayedNotifications.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " delayed notifications");

        for (size_t i = 0; i < N; ++i)
        {
            asyncInsertNotification(zdev->device,
                zdev->delayedNotifications[i]);
        }
        zdev->delayedNotifications.clear();
    }

private: // SerialModule

    /// @copydoc gateway::SerialModule::onOpenSerial()
    void onOpenSerial(boost::system::error_code err)
    {
        SerialModule::onOpenSerial(err);

        if (!err)
        {
            asyncListenForXBeeFrames(true);
            sendGatewayRegistrationRequest();
        }
        else
            asyncOpenSerial(SERIAL_RECONNECT_TIMEOUT); // try to open later
    }

private:

    /// @brief Send gateway registration request.
    void sendGatewayRegistrationRequest()
    {
        json::Value data;
        data["data"] = json::Value();

        sendGatewayMessage(ZDeviceSPtr(), gateway::INTENT_REGISTRATION_REQUEST, data);
    }


    /// @brief Send the command to the device.
    /**
    @param[in] zdev The ZigBee device.
    @param[in] cmd The command to send.
    @return `true` if command processed.
    */
    bool sendGatewayCommand(ZDeviceSPtr zdev, cloud6::Command const& cmd)
    {
        const int intent = zdev->gw.findCommandIntentByName(cmd.name);
        if (0 <= intent)
        {
            HIVELOG_INFO(m_log, "command: \"" << cmd.name
                << "\" mapped to #" << intent << " intent");

            json::Value data;
            data["id"] = cmd.id;
            data["parameters"] = cmd.params;
            if (!sendGatewayMessage(zdev, intent, data))
                throw std::runtime_error("invalid command format");
            return false; // device will report command result later!
        }
        else
            throw std::runtime_error("unknown command, ignored");

        return true;
    }


    /// @brief Send the custom gateway message.
    /**
    @param[in] zdev The ZigBee device.
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    @return `false` for invalid command.
    */
    bool sendGatewayMessage(ZDeviceSPtr zdev, int intent, json::Value const& data)
    {
        const gateway::Engine &gw = zdev ? zdev->gw : m_gw;
        if (gateway::Frame::SharedPtr frame = gw.jsonToFrame(intent, data))
        {
            if (zdev)
                sendGatewayFrame(frame, zdev->address64, zdev->address16);
            else
                sendGatewayFrame(frame, XBEE_BROADCAST64, XBEE_BROADCAST16);

            return true;
        }
        else
            HIVELOG_WARN_STR(m_log, "cannot convert frame to binary format");

        return false;
    }


    /// @brief Send the gateway frame.
    /**
    @param[in] frame The gateway frame to send.
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
            boost::bind(&This::onSendXBeeFrame,
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
            HIVELOG_DEBUG_STR(m_log, "TX operation cancelled");
        else
        {
            HIVELOG_ERROR(m_log, "failed to send frame: ["
                << err << "] " << err.message());
            resetSerial(true);
        }
    }

private:

    /// @brief Start/stop listen for RX frames.
    void asyncListenForXBeeFrames(bool enable)
    {
        m_xbee->recv(enable
            ? boost::bind(&This::onRecvXBeeFrame, shared_from_this(), _1, _2)
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

                try
                {
                    handleRecvXBeeFrame(frame);
                }
                catch (std::exception const& ex)
                {
                    HIVELOG_ERROR(m_log, "failed handle received frame: " << ex.what());
                    resetSerial(true);
                }
            }
            else
                HIVELOG_DEBUG_STR(m_log, "no frame received");
        }
        else if (err == boost::asio::error::operation_aborted)
            HIVELOG_DEBUG_STR(m_log, "RX operation cancelled");
        else
        {
            HIVELOG_ERROR(m_log, "failed to receive frame: ["
                << err << "] " << err.message());
            resetSerial(true);
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
                    boost::shared_ptr<ZDevice> zdev = getZDevice(payload.srcAddr64);
                    zdev->address16 = payload.srcAddr16;

                    { // put the data to the stream buffer
                        OStream os(&zdev->m_rx_buf);
                        os.write(payload.data.data(), payload.data.size());
                    }

                    HIVELOG_DEBUG(m_log, "got [" << dump::hex(payload.data) << "] from "
                        << dump::hex(payload.srcAddr64) << "/" << dump::hex(payload.srcAddr16));

                    gateway::Frame::ParseResult result = gateway::Frame::RESULT_SUCCESS;
                    if (gateway::Frame::SharedPtr frame = gateway::Frame::parseFrame(zdev->m_rx_buf, &result))
                    {
                        handleGatewayMessage(frame->getIntent(),
                            zdev->gw.frameToJson(frame), zdev);
                    }
                }
            } break;

            default:
                // do nothing
                break;
        }
    }


    /// @brief Create device from the JSON data.
    /**
    @param[in] zdev The ZigBee device.
    @param[in] jdev The JSON device description.
    */
    void createDevice(ZDeviceSPtr zdev, json::Value const& jdev)
    {
        { // create device
            String id = jdev["id"].asString();
            String key = jdev["key"].asString();
            String name = jdev["name"].asString();

            // TODO: get network from command line
            cloud6::NetworkPtr network = cloud6::Network::create(
                m_networkName, m_networkKey, m_networkDesc);

            cloud6::Device::ClassPtr deviceClass = cloud6::Device::Class::create("", "", false, DEVICE_OFFLINE_TIMEOUT);
            cloud6::Serializer::json2deviceClass(jdev["deviceClass"], deviceClass);

            zdev->device = cloud6::Device::create(id, name, key, deviceClass, network);
            zdev->device->status = "Online";

            zdev->delayedNotifications.clear();
            zdev->deviceRegistered = false;
        }

        { // update equipment
            json::Value const& equipment = jdev["equipment"];
            const size_t N = equipment.size();
            for (size_t i = 0; i < N; ++i)
            {
                json::Value const& eq = equipment[i];

                String id = eq["code"].asString();
                String name = eq["name"].asString();
                String type = eq["type"].asString();

                zdev->device->equipment.push_back(cloud6::Equipment::create(id, name, type));
            }
        }
    }


    /// @brief Handle the incomming message.
    /**
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    @param[in] zdev The ZigBee device.
    */
    void handleGatewayMessage(int intent, json::Value const& data, ZDeviceSPtr zdev)
    {
        HIVELOG_INFO(m_log, "process intent #" << intent << " data: " << data << "\n");

        if (intent == gateway::INTENT_REGISTRATION_RESPONSE)
        {
            zdev->gw.handleRegisterResponse(data);
            createDevice(zdev, data);
            asyncRegisterDevice(zdev->device);
        }

        else if (intent == gateway::INTENT_REGISTRATION2_RESPONSE)
        {
            json::Value jdev = json::fromStr(data["json"].asString());
            zdev->gw.handleRegister2Response(jdev);
            createDevice(zdev, jdev);
            asyncRegisterDevice(zdev->device);
        }

        else if (intent == gateway::INTENT_COMMAND_RESULT_RESPONSE)
        {
            if (zdev->device)
            {
                HIVELOG_DEBUG(m_log, "got command result");

                cloud6::Command cmd;
                cmd.id = data["id"].asUInt();
                cmd.status = data["status"].asString();
                cmd.result = data["result"].asString();

                asyncUpdateCommand(zdev->device, cmd);
            }
            else
                HIVELOG_WARN_STR(m_log, "got command result before registration, ignored");
        }

        else if (intent >= gateway::INTENT_USER)
        {
            if (zdev->device)
            {
                String name = zdev->gw.findNotificationNameByIntent(intent);
                if (!name.empty())
                {
                    HIVELOG_DEBUG(m_log, "got notification");

                    if (zdev->deviceRegistered)
                    {
                        asyncInsertNotification(zdev->device,
                            cloud6::Notification(name, data));
                    }
                    else
                    {
                        HIVELOG_DEBUG_STR(m_log, "device is not registered, notification delayed");
                        zdev->delayedNotifications.push_back(cloud6::Notification(name, data));
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
    String m_networkName; ///< @brief The network name.
    String m_networkKey; ///< @brief The network key.
    String m_networkDesc; ///< @brief The network description.

private:
    gateway::Engine m_gw; ///< @brief The gateway engine for system intents.

private:
    typedef xbee::API<boost::asio::serial_port> XBeeAPI; ///< @brief The XBee %API type.
    XBeeAPI::SharedPtr m_xbee; ///< @brief The XBee %API.
    size_t m_xbeeFrameId; ///< @brief The XBee TX frame identifier.
};


/// @brief The ZigBee gateway application.
/**
Uses DeviceHive WebSocket server interface.
*/
class ApplicationWS:
    public basic_app::Application,
    public cloud7::ServerModuleWS,
    public gateway::SerialModule
{
    typedef basic_app::Application Base; ///< @brief The base class.
    typedef ApplicationWS This; ///< @brief The this type alias.
protected:

    /// @brief The default constructor.
    ApplicationWS()
        : ServerModuleWS(m_ios, m_log)
        , SerialModule(m_ios, m_log)
        , m_xbeeFrameId(1)
    {
        HIVELOG_INFO_STR(m_log, "WebSocket service is used");
    }

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<ApplicationWS> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] argc The number of command line arguments.
    @param[in] argv The command line arguments.
    @return The new application instance.
    */
    static SharedPtr create(int argc, const char* argv[])
    {
        SharedPtr pthis(new This());

        String networkName = "XBee C++ network";
        String networkKey = "";
        String networkDesc = "C++ device test network";

        String baseUrl = "http://ecloud.dataart.com:8010/";
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

        if (serialPortName.empty())
            throw std::runtime_error("no serial port name provided");

        pthis->m_xbee = XBeeAPI::create(pthis->m_serial);
        pthis->m_networkName = networkName;
        pthis->m_networkKey = networkKey;
        pthis->m_networkDesc = networkDesc;

        pthis->initServerModuleWS(baseUrl, pthis);
        pthis->initSerialModule(serialPortName, serialBaudrate, pthis);
        return pthis;
    }


    /// @brief Get the shared pointer.
    /**
    @return The shared pointer to this instance.
    */
    SharedPtr shared_from_this()
    {
        return boost::shared_dynamic_cast<This>(Base::shared_from_this());
    }

protected:

    /// @brief Start the application.
    /**
    Tries to open serial port.
    */
    virtual void start()
    {
        Base::start();
        asyncConnectToServer(0); // ASAP
        asyncOpenSerial(0); //ASAP
    }


    /// @brief Stop the application.
    /**
    Stops the "open" timer.
    */
    virtual void stop()
    {
        cancelServerModuleWS();
        cancelSerialModule();
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


    /// @brief The ZigBee device.
    /**
    */
    class ZDevice
    {
    public:
        UInt64 address64; ///< @brief The MAC address.
        UInt16 address16; ///< @brief The network address.
        boost::asio::streambuf m_rx_buf; ///< @brief The RX stream buffer.

        cloud7::DevicePtr device; ///< @brief The corresponding device.
        bool deviceRegistered;    ///< @brief The "registered" flag.
        std::vector<cloud7::Notification> delayedNotifications; ///< @brief The list of delayed notification.
        gateway::Engine gw; ///< @brief The gateway engine.

        /// @brief The default constructor.
        ZDevice()
            : address64(XBEE_BROADCAST64)
            , address16(XBEE_BROADCAST16)
            , deviceRegistered(false)
        {}
    };

    /// @brief The ZigBee device shared pointer type.
    typedef boost::shared_ptr<ZDevice> ZDeviceSPtr;

private:

    /// @brief The list of ZigBee devices.
    std::map<UInt64, ZDeviceSPtr> m_devices;

    /// @brief Get or create new ZigBee device.
    /**
    @param[in] address The device MAC address.
    @return The ZigBee device.
    */
    ZDeviceSPtr getZDevice(UInt64 address)
    {
        if (ZDeviceSPtr zdev = m_devices[address])
            return zdev;
        else
        {
            zdev.reset(new ZDevice());
            m_devices[address] = zdev;
            zdev->address64 = address;
            return zdev;
        }
    }


    /// @brief Find ZigBee device.
    /**
    @param[in] device The device to find.
    @return The ZigBee device or NULL.
    */
    ZDeviceSPtr findZDevice(cloud7::DevicePtr device) const
    {
        typedef std::map<UInt64, ZDeviceSPtr>::const_iterator Iterator;
        for (Iterator i = m_devices.begin(); i != m_devices.end(); ++i)
        {
            ZDeviceSPtr zdev = i->second;
            if (zdev->device == device)
                return zdev;
        }

        return ZDeviceSPtr(); // not found
    }


    /// @brief Find ZigBee device by identifier.
    /**
    @param[in] deviceId The device identifier to find.
    @return The ZigBee device or NULL.
    */
    ZDeviceSPtr findZDeviceById(String const& deviceId) const
    {
        typedef std::map<UInt64, ZDeviceSPtr>::const_iterator Iterator;
        for (Iterator i = m_devices.begin(); i != m_devices.end(); ++i)
        {
            ZDeviceSPtr zdev = i->second;
            if (zdev->device && zdev->device->id == deviceId)
                return zdev;
        }

        return ZDeviceSPtr(); // not found
    }

private: // ServerModuleWS

    /// @copydoc cloud7::ServerModuleWS::onConnectedToServer()
    virtual void onConnectedToServer(boost::system::error_code err)
    {
        ServerModuleWS::onConnectedToServer(err);

        if (!err)
        {
            //sendDelayedCommandResults();
            //sendDelayedNotifications();

            if (m_serial.is_open())
                sendGatewayRegistrationRequest();
        }
        else if (!terminated())
        {
            // try to reconnect again
            asyncConnectToServer(SERVER_RECONNECT_TIMEOUT);
        }
    }


    /// @copydoc cloud7::ServerModuleWS::onActionReceived()
    virtual void onActionReceived(boost::system::error_code err, json::Value const& jaction)
    {
        ServerModuleWS::onActionReceived(err, jaction);

        if (!err)
        {
            try
            {
                String const action = jaction["action"].asString();
                handleAction(action, jaction);
            }
            catch (std::exception const& ex)
            {
                HIVELOG_ERROR(m_log,
                    "failed to process action: "
                    << ex.what() << "\n");
            }
        }
        else if (!terminated())
        {
            // try to reconnect again
            asyncConnectToServer(SERVER_RECONNECT_TIMEOUT);
        }
    }


    /// @brief Handle received action.
    /**
    @param[in] action The action name.
    @param[in] params The action parameters.
    */
    void handleAction(String const& action, json::Value const& params)
    {
        if (boost::iequals(action, "device/save")) // got registration
        {
            if (boost::iequals(params["status"].asString(), "success"))
            {
                String const deviceId = params["device"]["id"].asString();
                if (ZDeviceSPtr zdev = findZDeviceById(deviceId))
                {
                    zdev->deviceRegistered = true;
                    sendDelayedNotifications(zdev);
                    asyncSubscribeForCommands(zdev->device);
                }
                else
                    HIVELOG_ERROR(m_log, "unknown device");
            }
            else
                HIVELOG_ERROR(m_log, "failed to register");
        }

        else if (boost::iequals(action, "command/insert")) // new command
        {
            String const deviceId = params["deviceGuid"].asString();
            cloud7::Command cmd = cloud7::Serializer::json2cmd(params["command"]);
            if (m_serial.is_open())
            {
                if (ZDeviceSPtr zdev = findZDeviceById(deviceId))
                {
                    bool processed = true;

                    try
                    {
                        processed = sendGatewayCommand(zdev, cmd);
                    }
                    catch (std::exception const& ex)
                    {
                        HIVELOG_ERROR(m_log, "handle command error: "
                            << ex.what());

                        cmd.status = "Failed";
                        cmd.result = ex.what();
                    }

                    if (processed)
                        asyncUpdateCommand(zdev->device, cmd);
                }
                else
                    HIVELOG_ERROR(m_log, "unknown device");
            }
            else
            {
                HIVELOG_WARN(m_log, "no serial device connected, command delayed");
                //m_delayedCommands.push_back(cmd);
            }
        }
    }


    /// @brief Send all delayed notifications.
    /**
    @param[in] zdev The ZigBee device.
    */
    void sendDelayedNotifications(ZDeviceSPtr zdev)
    {
        const size_t N = zdev->delayedNotifications.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " delayed notifications");

        for (size_t i = 0; i < N; ++i)
        {
            asyncInsertNotification(zdev->device,
                zdev->delayedNotifications[i]);
        }
        zdev->delayedNotifications.clear();
    }

private: // SerialModule

    /// @copydoc gateway::SerialModule::onOpenSerial()
    void onOpenSerial(boost::system::error_code err)
    {
        SerialModule::onOpenSerial(err);

        if (!err)
        {
            asyncListenForXBeeFrames(true);
            sendGatewayRegistrationRequest();
        }
        else
            asyncOpenSerial(SERIAL_RECONNECT_TIMEOUT); // try to open later
    }

private:

    /// @brief Send gateway registration request.
    void sendGatewayRegistrationRequest()
    {
        json::Value data;
        data["data"] = json::Value();

        sendGatewayMessage(ZDeviceSPtr(), gateway::INTENT_REGISTRATION_REQUEST, data);
    }


    /// @brief Send the command to the device.
    /**
    @param[in] zdev The ZigBee device.
    @param[in] cmd The command to send.
    @return `true` if command processed.
    */
    bool sendGatewayCommand(ZDeviceSPtr zdev, cloud7::Command const& cmd)
    {
        const int intent = zdev->gw.findCommandIntentByName(cmd.name);
        if (0 <= intent)
        {
            HIVELOG_INFO(m_log, "command: \"" << cmd.name
                << "\" mapped to #" << intent << " intent");

            json::Value data;
            data["id"] = cmd.id;
            data["parameters"] = cmd.params;
            if (!sendGatewayMessage(zdev, intent, data))
                throw std::runtime_error("invalid command format");
            return false; // device will report command result later!
        }
        else
            throw std::runtime_error("unknown command, ignored");

        return true;
    }


    /// @brief Send the custom gateway message.
    /**
    @param[in] zdev The ZigBee device.
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    @return `false` for invalid command.
    */
    bool sendGatewayMessage(ZDeviceSPtr zdev, int intent, json::Value const& data)
    {
        const gateway::Engine &gw = zdev ? zdev->gw : m_gw;
        if (gateway::Frame::SharedPtr frame = gw.jsonToFrame(intent, data))
        {
            if (zdev)
                sendGatewayFrame(frame, zdev->address64, zdev->address16);
            else
                sendGatewayFrame(frame, XBEE_BROADCAST64, XBEE_BROADCAST16);

            return true;
        }
        else
            HIVELOG_WARN_STR(m_log, "cannot convert frame to binary format");

        return false;
    }


    /// @brief Send the gateway frame.
    /**
    @param[in] frame The gateway frame to send.
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
            boost::bind(&This::onSendXBeeFrame,
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
            HIVELOG_DEBUG_STR(m_log, "TX operation cancelled");
        else
        {
            HIVELOG_ERROR(m_log, "failed to send frame: ["
                << err << "] " << err.message());
            resetSerial(true);
        }
    }

private:

    /// @brief Start/stop listen for RX frames.
    void asyncListenForXBeeFrames(bool enable)
    {
        m_xbee->recv(enable
            ? boost::bind(&This::onRecvXBeeFrame, shared_from_this(), _1, _2)
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

                try
                {
                    handleRecvXBeeFrame(frame);
                }
                catch (std::exception const& ex)
                {
                    HIVELOG_ERROR(m_log, "failed handle received frame: " << ex.what());
                    resetSerial(true);
                }
            }
            else
                HIVELOG_DEBUG_STR(m_log, "no frame received");
        }
        else if (err == boost::asio::error::operation_aborted)
            HIVELOG_DEBUG_STR(m_log, "RX operation cancelled");
        else
        {
            HIVELOG_ERROR(m_log, "failed to receive frame: ["
                << err << "] " << err.message());
            resetSerial(true);
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
                    boost::shared_ptr<ZDevice> zdev = getZDevice(payload.srcAddr64);
                    zdev->address16 = payload.srcAddr16;

                    { // put the data to the stream buffer
                        OStream os(&zdev->m_rx_buf);
                        os.write(payload.data.data(), payload.data.size());
                    }

                    HIVELOG_DEBUG(m_log, "got [" << dump::hex(payload.data) << "] from "
                        << dump::hex(payload.srcAddr64) << "/" << dump::hex(payload.srcAddr16));

                    gateway::Frame::ParseResult result = gateway::Frame::RESULT_SUCCESS;
                    if (gateway::Frame::SharedPtr frame = gateway::Frame::parseFrame(zdev->m_rx_buf, &result))
                    {
                        handleGatewayMessage(frame->getIntent(),
                            zdev->gw.frameToJson(frame), zdev);
                    }
                }
            } break;

            default:
                // do nothing
                break;
        }
    }


    /// @brief Create device from the JSON data.
    /**
    @param[in] zdev The ZigBee device.
    @param[in] jdev The JSON device description.
    */
    void createDevice(ZDeviceSPtr zdev, json::Value const& jdev)
    {
        { // create device
            String id = jdev["id"].asString();
            String key = jdev["key"].asString();
            String name = jdev["name"].asString();

            // TODO: get network from command line
            cloud7::NetworkPtr network = cloud7::Network::create(
                m_networkName, m_networkKey, m_networkDesc);

            cloud7::Device::ClassPtr deviceClass = cloud7::Device::Class::create("", "", false, DEVICE_OFFLINE_TIMEOUT);
            cloud7::Serializer::json2deviceClass(jdev["deviceClass"], deviceClass);

            zdev->device = cloud7::Device::create(id, name, key, deviceClass, network);
            zdev->device->status = "Online";

            zdev->delayedNotifications.clear();
            zdev->deviceRegistered = false;
        }

        { // update equipment
            json::Value const& equipment = jdev["equipment"];
            const size_t N = equipment.size();
            for (size_t i = 0; i < N; ++i)
            {
                json::Value const& eq = equipment[i];

                String id = eq["code"].asString();
                String name = eq["name"].asString();
                String type = eq["type"].asString();

                zdev->device->equipment.push_back(cloud7::Equipment::create(id, name, type));
            }
        }
    }


    /// @brief Handle the incomming message.
    /**
    @param[in] intent The message intent.
    @param[in] data The custom message data.
    @param[in] zdev The ZigBee device.
    */
    void handleGatewayMessage(int intent, json::Value const& data, ZDeviceSPtr zdev)
    {
        HIVELOG_INFO(m_log, "process intent #" << intent << " data: " << data << "\n");

        if (intent == gateway::INTENT_REGISTRATION_RESPONSE)
        {
            zdev->gw.handleRegisterResponse(data);
            createDevice(zdev, data);
            asyncRegisterDevice(zdev->device);
        }

        else if (intent == gateway::INTENT_REGISTRATION2_RESPONSE)
        {
            json::Value jdev = json::fromStr(data["json"].asString());
            zdev->gw.handleRegister2Response(jdev);
            createDevice(zdev, jdev);
            asyncRegisterDevice(zdev->device);
        }

        else if (intent == gateway::INTENT_COMMAND_RESULT_RESPONSE)
        {
            if (zdev->device)
            {
                HIVELOG_DEBUG(m_log, "got command result");

                cloud7::Command cmd;
                cmd.id = data["id"].asUInt();
                cmd.status = data["status"].asString();
                cmd.result = data["result"].asString();

                asyncUpdateCommand(zdev->device, cmd);
            }
            else
                HIVELOG_WARN_STR(m_log, "got command result before registration, ignored");
        }

        else if (intent >= gateway::INTENT_USER)
        {
            if (zdev->device)
            {
                String name = zdev->gw.findNotificationNameByIntent(intent);
                if (!name.empty())
                {
                    HIVELOG_DEBUG(m_log, "got notification");

                    if (zdev->deviceRegistered)
                    {
                        asyncInsertNotification(zdev->device,
                            cloud7::Notification(name, data));
                    }
                    else
                    {
                        HIVELOG_DEBUG_STR(m_log, "device is not registered, notification delayed");
                        zdev->delayedNotifications.push_back(cloud7::Notification(name, data));
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
    String m_networkName; ///< @brief The network name.
    String m_networkKey; ///< @brief The network key.
    String m_networkDesc; ///< @brief The network description.

private:
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
@param[in] useNewWS Use new websocket service flag.
*/
inline void main(int argc, const char* argv[], bool useNewWS = true)
{
    { // configure logging
        log::Target::SharedPtr log_file = log::Target::File::create("zigbee_gw.log");
        log::Target::SharedPtr log_console = log::Logger::root().getTarget();
        log::Logger::root().setTarget(log::Target::Tie::create(log_file, log_console));
        log::Logger::root().setLevel(log::LEVEL_TRACE);
        log::Logger("/hive/http").setTarget(log_file).setLevel(log::LEVEL_DEBUG); // disable annoying messages
        log_console->setFormat(log::Format::create("%N %L %M\n"));
        log_console->setMinimumLevel(log::LEVEL_DEBUG);
    }

    if (useNewWS)
        ApplicationWS::create(argc, argv)->run();
    else
        Application::create(argc, argv)->run();
}

} // zigbee_gw namespace


///////////////////////////////////////////////////////////////////////////////
/** @page page_zigbee_gw ZigBee gateway example

ZigBee gateway example is very similar to @ref page_simple_gw except that
end-devices are connected to the gateway via XBee module. Of course it is
possible to connect more than one remote device.


This page is under construction!
================================


@example examples/zigbee_gw.hpp
*/

#endif // __EXAMPLES_ZIGBEE_GW_HPP_
