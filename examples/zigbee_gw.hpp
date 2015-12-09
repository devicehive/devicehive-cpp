/** @file
@brief The ZigBee gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_zigbee_gw
*/
#ifndef __EXAMPLES_ZIGBEE_GW_HPP_
#define __EXAMPLES_ZIGBEE_GW_HPP_

#include <DeviceHive/gateway.hpp>
#include <DeviceHive/restful.hpp>
#include <DeviceHive/websocket.hpp>
#include <DeviceHive/xbee.hpp>
#include "basic_app.hpp"

namespace examples
{

/// @brief The ZigBee gateway example.
namespace zigbee_gw
{
    using namespace hive;

/// @brief Various contants and timeouts.
enum Timeouts
{
    SERIAL_RECONNECT_TIMEOUT    = 10000, ///< @brief Try to open serial port each X milliseconds.
    SERVER_RECONNECT_TIMEOUT    = 10000, ///< @brief Try to open server connection each X milliseconds.
    RETRY_TIMEOUT               = 5000,  ///< @brief Common retry timeout, milliseconds.
    DEVICE_OFFLINE_TIMEOUT      = 0
};


/// @brief The ZigBee gateway application.
/**
This application controls many devices connected via XBee module!
*/
class Application:
    public basic_app::Application,
    public devicehive::IDeviceServiceEvents
{
    typedef basic_app::Application Base; ///< @brief The base class.
    typedef Application This; ///< @brief The type alias.

protected:

    /// @brief The default constructor.
    Application()
        : m_disableWebsockets(false)
        , m_disableWebsocketPingPong(false)
        , m_serial(m_ios)
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
        SharedPtr pthis(new This());

        String networkName = "XBee C++ network";
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
                std::cout << "\t--serial <serial device name>\n";
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

        pthis->m_serialPortName = serialPortName;
        pthis->m_serialBaudrate = serialBaudrate;
        pthis->m_xbee = XBeeAPI::create(pthis->m_serial);
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
    Tries to open serial port.
    */
    virtual void start()
    {
        Base::start();
        m_service->asyncConnect();
        m_delayed->callLater( // ASAP
            boost::bind(&This::tryToOpenSerial,
                shared_from_this()));
    }


    /// @brief Stop the application.
    /**
    Stops the "open" timer.
    */
    virtual void stop()
    {
        m_service->cancelAll();
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


    /// @brief The ZigBee device.
    /**
    */
    class ZDevice
    {
    public:
        UInt64 address64; ///< @brief The MAC address.
        UInt16 address16; ///< @brief The network address.
        boost::asio::streambuf m_rx_buf; ///< @brief The RX stream buffer.

        devicehive::DevicePtr device; ///< @brief The corresponding device.
        bool deviceRegistered;    ///< @brief The "registered" flag.
        String m_lastCommandTimestamp; ///< @brief The timestamp of the last received command.
        std::vector<devicehive::NotificationPtr> delayedNotifications; ///< @brief The list of delayed notification.
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
    ZDeviceSPtr findZDevice(devicehive::DevicePtr device) const
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

private:

    /// @brief Try to open serial port device.
    /**
    */
    void tryToOpenSerial()
    {
        boost::system::error_code err = openSerial();

        if (!err)
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
                << err << "] " << err.message());

            m_delayed->callLater(SERIAL_RECONNECT_TIMEOUT,
                boost::bind(&This::tryToOpenSerial,
                    shared_from_this()));
        }
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


    /// @brief Reset the serial device.
    /**
    @brief tryToReopen if `true` then try to reopen serial as soon as possible.
    */
    virtual void resetSerial(bool tryToReopen)
    {
        HIVELOG_WARN(m_log, "serial device reset");
        m_serial.close();

        if (tryToReopen && !terminated())
        {
            m_delayed->callLater( // ASAP
                boost::bind(&This::tryToOpenSerial,
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
            HIVELOG_INFO_STR(m_log, "connected to the server");
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
        if (!err)
        {
            if (ZDeviceSPtr zdev = findZDevice(device))
            {
                zdev->deviceRegistered = true;
                sendDelayedNotifications(zdev);
                m_service->asyncSubscribeForCommands(device,
                    zdev->m_lastCommandTimestamp);
            }
            else
                HIVELOG_ERROR(m_log, "unknown device");
        }
        else
            handleServiceError(err, "registering device");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onInsertCommand()
    virtual void onInsertCommand(ErrorCode err, devicehive::DevicePtr device, devicehive::CommandPtr command)
    {
        if (!err)
        {
            if (ZDeviceSPtr zdev = findZDevice(device))
            {
                zdev->m_lastCommandTimestamp = command->timestamp;
                bool processed = true;

                try
                {
                    processed = sendGatewayCommand(zdev, command);
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
                HIVELOG_ERROR(m_log, "unknown device");
        }
            handleServiceError(err, "polling command");
    }

private:

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
            m_service->asyncInsertNotification(zdev->device,
                zdev->delayedNotifications[i]);
        }
        zdev->delayedNotifications.clear();
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

        sendGatewayMessage(ZDeviceSPtr(), gateway::INTENT_REGISTRATION_REQUEST, data);
    }


    /// @brief Send the command to the device.
    /**
    @param[in] zdev The ZigBee device.
    @param[in] command The command to send.
    @return `true` if command processed.
    */
    bool sendGatewayCommand(ZDeviceSPtr zdev, devicehive::CommandPtr command)
    {
        const int intent = zdev->gw.findCommandIntentByName(command->name);
        if (0 <= intent)
        {
            HIVELOG_INFO(m_log, "command: \"" << command->name
                << "\" mapped to #" << intent << " intent");

            json::Value data;
            data["id"] = command->id;
            data["parameters"] = command->params;
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

            devicehive::Device::ClassPtr deviceClass = devicehive::Device::Class::create("", "", false, DEVICE_OFFLINE_TIMEOUT);
            devicehive::Serializer::fromJson(jdev["deviceClass"], deviceClass);

            zdev->device = devicehive::Device::create(id, name, key, deviceClass, m_network);
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

                String code = eq["code"].asString();
                String name = eq["name"].asString();
                String type = eq["type"].asString();

                zdev->device->equipment.push_back(devicehive::Equipment::create(code, name, type));
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
            m_service->asyncRegisterDevice(zdev->device);
        }

        else if (intent == gateway::INTENT_REGISTRATION2_RESPONSE)
        {
            json::Value jdev = json::fromStr(data["json"].asString());
            zdev->gw.handleRegister2Response(jdev);
            createDevice(zdev, jdev);
            m_service->asyncRegisterDevice(zdev->device);
        }

        else if (intent == gateway::INTENT_COMMAND_RESULT_RESPONSE)
        {
            if (zdev->device)
            {
                HIVELOG_DEBUG(m_log, "got command result");

                devicehive::CommandPtr command = devicehive::Command::create();
                command->id = data["id"].asUInt();
                command->status = data["status"].asString();
                command->result = data["result"].asString();

                m_service->asyncUpdateCommand(zdev->device, command);
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

                    devicehive::NotificationPtr notification = devicehive::Notification::create(name, data["parameters"]);

                    if (zdev->deviceRegistered)
                    {
                        m_service->asyncInsertNotification(zdev->device, notification);
                    }
                    else
                    {
                        HIVELOG_DEBUG_STR(m_log, "device is not registered, notification delayed");
                        zdev->delayedNotifications.push_back(notification);
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
    devicehive::NetworkPtr m_network; ///< @brief The network.
    gateway::Engine m_gw; ///< @brief The gateway engine for system intents.

private:
    devicehive::IDeviceServicePtr m_service; ///< @brief The cloud service.
    bool m_disableWebsockets;       ///< @brief No automatic websocket switch.
    bool m_disableWebsocketPingPong; ///< @brief Disable websocket PING/PONG messages.

private:
    boost::asio::serial_port m_serial; ///< @brief The serial port device.
    String m_serialPortName; ///< @brief The serial port name.
    UInt32 m_serialBaudrate; ///< @brief The serial baudrate.

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
        using namespace hive::log;

        Target::SharedPtr log_file = Target::File::create("zigbee_gw.log");
        Target::SharedPtr log_console = Logger::root().getTarget();
        Logger::root().setTarget(Target::Tie::create(log_file, log_console));
        Logger::root().setLevel(LEVEL_TRACE);
        Logger("/gateway/API").setTarget(log_file); // disable annoying messages
        Logger("/xbee/API").setTarget(log_file); // disable annoying messages
        Logger("/hive/websocket").setTarget(log_file).setLevel(LEVEL_DEBUG); // disable annoying messages
        Logger("/hive/http").setTarget(log_file).setLevel(LEVEL_DEBUG); // disable annoying messages
        log_console->setFormat(Format::create("%N %L %M\n"));
        log_console->setMinimumLevel(LEVEL_DEBUG);
    }

    Application::create(argc, argv)->run();
}

} // zigbee_gw namespace
} // examples namespace


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
