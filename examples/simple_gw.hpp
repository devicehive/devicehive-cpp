/** @file
@brief The simple gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_simple_gw
*/
#ifndef __EXAMPLES_SIMPLE_GW_HPP_
#define __EXAMPLES_SIMPLE_GW_HPP_

#include <DeviceHive/gateway.hpp>
#include <DeviceHive/cloud7.hpp>
#include "basic_app.hpp"


/// @brief The simple gateway example.
namespace simple_gw
{
    using namespace hive;

/// @brief Various contants and timeouts.
enum
{
    SERIAL_RECONNECT_TIMEOUT    = 10, ///< @brief Try to open serial port each X seconds.
    SERVER_RECONNECT_TIMEOUT    = 10, ///< @brief Try to open server connection each X seconds.
    RETRY_TIMEOUT               = 5,  ///< @brief Common retry timeout, seconds.
    DEVICE_OFFLINE_TIMEOUT      = 0
};


/// @brief The simple gateway application.
/**
Uses DeviceHive REST server interface.
*/
class Application:
    public basic_app::Application,
    public basic_app::DelayedTasks,
    public cloud6::ServerModuleREST,
    public gateway::SerialModule
{
    typedef basic_app::Application Base; ///< @brief The base type.
    typedef Application This; ///< @brief The this type alias.
protected:

    /// @brief The default constructor.
    Application()
        : DelayedTasks(m_ios, m_log)
        , ServerModuleREST(m_ios, m_log)
        , SerialModule(m_ios, m_log)
        , m_deviceRegistered(false)
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

        String networkName = "C++ network";
        String networkKey = "";
        String networkDesc = "C++ device test network";

        String baseUrl = "http://ecloud.dataart.com/ecapi7";
        size_t web_timeout = 0; // zero - don't change

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
            else if (boost::algorithm::iequals(argv[i], "--web-timeout") && i+1 < argc)
                web_timeout = boost::lexical_cast<UInt32>(argv[++i]);
            else if (boost::algorithm::iequals(argv[i], "--serial") && i+1 < argc)
                serialPortName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--baudrate") && i+1 < argc)
                serialBaudrate = boost::lexical_cast<UInt32>(argv[++i]);
        }

        if (serialPortName.empty())
            throw std::runtime_error("no serial port name provided");

        pthis->m_gw_api = GatewayAPI::create(pthis->m_serial);
        pthis->m_networkName = networkName;
        pthis->m_networkKey = networkKey;
        pthis->m_networkDesc = networkDesc;

        pthis->initDelayedTasks(pthis);
        pthis->initServerModuleREST(baseUrl, pthis);
        pthis->initSerialModule(serialPortName, serialBaudrate, pthis);
        if (0 < web_timeout)
            pthis->m_serverAPI->setTimeout(web_timeout*1000); // seconds -> milliseconds

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
        asyncOpenSerial(0); // ASAP
        asyncGetServerInfo();
    }


    /// @brief Stop the application.
    /**
    Stops the "open" timer.
    */
    virtual void stop()
    {
        cancelServerModuleREST();
        cancelSerialModule();
        asyncListenForGatewayFrames(false); // stop listening to release shared pointer
        cancelDelayedTasks();
        Base::stop();
    }

private: // ServerModuleREST

    /// @copydoc cloud6::ServerModuleREST::onGotServerInfo()
    virtual void onGotServerInfo(boost::system::error_code err, json::Value const& info)
    {
        ServerModuleREST::onGotServerInfo(err, info);

        if (!err)
        {
            // update the last command time!
            m_lastCommandTimestamp = info["serverTimestamp"].asString();
        }
    }


    /// @copydoc cloud6::ServerModuleREST::onRegisterDevice()
    virtual void onRegisterDevice(boost::system::error_code err, cloud6::DevicePtr device)
    {
        ServerModuleREST::onRegisterDevice(err, device);

        if (m_device != device)     // if device's changed
            return;                 // just do nothing

        if (!err)
        {
            m_deviceRegistered = true;

            asyncPollCommands(device, m_lastCommandTimestamp);
            sendDelayedNotifications();
        }
        else if (!terminated())
        {
            // try to register again later!
            // asyncRegisterDevice(device);
            callLater(RETRY_TIMEOUT*1000,
                boost::bind(&This::asyncRegisterDevice,
                    shared_from_this(), device));
        }
    }


    /// @copydoc cloud6::ServerModuleREST::onPollCommands()
    virtual void onPollCommands(boost::system::error_code err, cloud6::DevicePtr device, std::vector<cloud6::Command> const& commands)
    {
        ServerModuleREST::onPollCommands(err, device, commands);

        if (m_device != device)     // if device's changed
            return;                 // just do nothing

        if (!err)
        {
            typedef std::vector<cloud6::Command>::const_iterator Iterator;
            for (Iterator i = commands.begin(); i != commands.end(); ++i)
            {
                cloud6::Command cmd = *i; // copy to modify
                m_lastCommandTimestamp = cmd.timestamp;
                bool processed = true;

                try
                {
                    processed = sendGatewayCommand(cmd);
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

            asyncPollCommands(device, m_lastCommandTimestamp); // start poll again
        }
        else if (!terminated())
        {
            // start poll again later!
            callLater(RETRY_TIMEOUT*1000,
                boost::bind(&This::asyncPollCommands, shared_from_this(),
                    device, m_lastCommandTimestamp));
        }
    }


    /// @brief Send all delayed notifications.
    void sendDelayedNotifications()
    {
        const size_t N = m_delayedNotifications.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " delayed notifications");

        for (size_t i = 0; i < N; ++i)
        {
            asyncInsertNotification(m_device,
                m_delayedNotifications[i]);
        }
        m_delayedNotifications.clear();
    }

private: // SerialModule

    /// @copydoc gateway::SerialModule::onOpenSerial()
    void onOpenSerial(boost::system::error_code err)
    {
        SerialModule::onOpenSerial(err);

        if (!err)
        {
            asyncListenForGatewayFrames(true);
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

        sendGatewayMessage(gateway::INTENT_REGISTRATION_REQUEST, data);
    }


    /// @brief Send the command to the device.
    /**
    @param[in] cmd The command to send.
    @return `true` if command processed.
    */
    bool sendGatewayCommand(cloud6::Command const& cmd)
    {
        const int intent = m_gw.findCommandIntentByName(cmd.name);
        if (0 <= intent)
        {
            HIVELOG_INFO(m_log, "command: \"" << cmd.name
                << "\" mapped to #" << intent << " intent");

            json::Value data;
            data["id"] = cmd.id;
            data["parameters"] = cmd.params;
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
            HIVELOG_DEBUG(m_log, "frame successfully sent #"
                << frame->getIntent() << ": ["
                << m_gw_api->hexdump(frame) << "], "
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
            resetSerial(true);
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
                HIVELOG_DEBUG(m_log, "frame received #"
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
                    HIVELOG_ERROR(m_log, "failed handle received frame: " << ex.what());
                    resetSerial(true);
                }
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
            resetSerial(true);
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

            cloud6::NetworkPtr network = cloud6::Network::create(
                m_networkName, m_networkKey, m_networkDesc);

            cloud6::Device::ClassPtr deviceClass = cloud6::Device::Class::create("", "", false, DEVICE_OFFLINE_TIMEOUT);
            cloud6::Serializer::json2deviceClass(jdev["deviceClass"], deviceClass);

            m_device = cloud6::Device::create(id, name, key, deviceClass, network);
            m_device->status = "Online";

            m_delayedNotifications.clear();
            m_deviceRegistered = false;
        }

        { // update equipment
            json::Value const& equipment = jdev["equipment"];
            const size_t N = equipment.size();
            for (size_t i = 0; i < N; ++i)
            {
                json::Value const& eq = equipment[i];

                const String id = eq["code"].asString();
                const String name = eq["name"].asString();
                const String type = eq["type"].asString();

                m_device->equipment.push_back(cloud6::Equipment::create(id, name, type));
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
        HIVELOG_INFO(m_log, "process intent #" << intent << " data: " << data << "\n");

        if (intent == gateway::INTENT_REGISTRATION_RESPONSE)
        {
            m_gw.handleRegisterResponse(data);
            createDevice(data);
            cancelServerModuleREST(); // cancel all previous requests
            asyncRegisterDevice(m_device);
        }

        else if (intent == gateway::INTENT_REGISTRATION2_RESPONSE)
        {
            json::Value jdev = json::fromStr(data["json"].asString());
            m_gw.handleRegister2Response(jdev);
            createDevice(jdev);
            asyncRegisterDevice(m_device);
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

                asyncUpdateCommand(m_device, cmd);
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

                    if (m_deviceRegistered)
                    {
                        asyncInsertNotification(m_device,
                            cloud6::Notification(name, data["parameters"]));
                    }
                    else
                    {
                        HIVELOG_DEBUG_STR(m_log, "device is not registered, notification delayed");
                        m_delayedNotifications.push_back(cloud6::Notification(name, data["parameters"]));
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
    typedef gateway::API<boost::asio::serial_port> GatewayAPI; ///< @brief The gateway %API type.
    GatewayAPI::SharedPtr m_gw_api; ///< @brief The gateway %API.
    gateway::Engine m_gw; ///< @brief The gateway engine.

private:
    cloud6::DevicePtr m_device; ///< @brief The device.
    bool m_deviceRegistered; ///< @brief The DeviceHive cloud "registered" flag.
    String m_lastCommandTimestamp; ///< @brief The timestamp of the last received command.
    std::vector<cloud6::Notification> m_delayedNotifications; ///< @brief The list of delayed notification.
};


/// @brief The simple gateway application.
/**
Uses DeviceHive WebSocket server interface.
*/
class ApplicationWS:
    public basic_app::Application,
    public cloud7::ServerModuleWS,
    public gateway::SerialModule
{
    typedef basic_app::Application Base; ///< @brief The base type.
    typedef ApplicationWS This; ///< @brief The this type alias.
protected:

    /// @brief The default constructor.
    ApplicationWS()
        : ServerModuleWS(m_ios, m_log)
        , SerialModule(m_ios, m_log)
        , m_deviceRegistered(false)
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

        String networkName = "C++ network";
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

        if (serialPortName.empty())
            throw std::runtime_error("no serial port name provided");

        pthis->m_gw_api = GatewayAPI::create(pthis->m_serial);
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
        asyncOpenSerial(0); // ASAP
    }


    /// @brief Stop the application.
    /**
    Stops the "open" timer.
    */
    virtual void stop()
    {
        cancelServerModuleWS();
        cancelSerialModule();
        asyncListenForGatewayFrames(false); // stop listening to release shared pointer
        Base::stop();
    }

private: // ServerModuleWS

    /// @copydoc cloud7::ServerModuleWS::onConnectedToServer()
    virtual void onConnectedToServer(boost::system::error_code err)
    {
        ServerModuleWS::onConnectedToServer(err);

        if (!err)
        {
            sendDelayedCommandResults();
            sendDelayedNotifications();

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
                m_deviceRegistered = true;

                asyncSubscribeForCommands(m_device);
                sendDelayedNotifications();
            }
            else
                HIVELOG_ERROR(m_log, "failed to register");
        }

        else if (boost::iequals(action, "command/insert")) // new command
        {
            String const deviceId = params["deviceGuid"].asString();
            if (!m_device || deviceId != m_device->id)
                throw std::runtime_error("unknown device identifier");

            cloud7::Command cmd = cloud7::Serializer::json2cmd(params["command"]);
            if (m_serial.is_open())
            {
                bool processed = true;

                try
                {
                    processed = sendGatewayCommand(cmd);
                }
                catch (std::exception const& ex)
                {
                    HIVELOG_ERROR(m_log, "handle command error: "
                        << ex.what());

                    cmd.status = "Failed";
                    cmd.result = ex.what();
                }

                if (processed)
                    asyncUpdateCommand(m_device, cmd);
            }
            else
            {
                HIVELOG_WARN(m_log, "no serial device connected, command delayed");
                m_delayedCommands.push_back(cmd);
            }
        }
    }


    /// @brief Send all delayed notifications.
    void sendDelayedNotifications()
    {
        const size_t N = m_delayedNotifications.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " delayed notifications");

        for (size_t i = 0; i < N; ++i)
        {
            asyncInsertNotification(m_device,
                m_delayedNotifications[i]);
        }
        m_delayedNotifications.clear();
    }


    /// @brief Send all delayed command results.
    void sendDelayedCommandResults()
    {
        const size_t N = m_delayedCommandResults.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " delayed command results");

        for (size_t i = 0; i < N; ++i)
        {
            cloud7::Command cmd = m_delayedCommandResults[i];
            asyncUpdateCommand(m_device, cmd);
        }
        m_delayedCommandResults.clear();
    }


    /// @brief Send all delayed commands.
    void sendDelayedCommands()
    {
        const size_t N = m_delayedCommands.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " delayed commands");

        for (size_t i = 0; i < N; ++i)
        {
            cloud7::Command cmd = m_delayedCommands[i];
            sendGatewayCommand(cmd);
        }
        m_delayedCommands.clear();
    }

private: // SerialModule

    /// @copydoc gateway::SerialModule::onOpenSerial()
    void onOpenSerial(boost::system::error_code err)
    {
        SerialModule::onOpenSerial(err);

        if (!err)
        {
            asyncListenForGatewayFrames(true);
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

        sendGatewayMessage(gateway::INTENT_REGISTRATION_REQUEST, data);
    }


    /// @brief Send the command to the device.
    /**
    @param[in] cmd The command to send.
    @return `true` if command processed.
    */
    bool sendGatewayCommand(cloud7::Command const& cmd)
    {
        const int intent = m_gw.findCommandIntentByName(cmd.name);
        if (0 <= intent)
        {
            HIVELOG_INFO(m_log, "command: \"" << cmd.name
                << "\" mapped to #" << intent << " intent");

            json::Value data;
            data["id"] = cmd.id;
            data["parameters"] = cmd.params;
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
            HIVELOG_DEBUG(m_log, "frame successfully sent #"
                << frame->getIntent() << ": ["
                << m_gw_api->hexdump(frame) << "], "
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
            resetSerial(true);
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
                HIVELOG_DEBUG(m_log, "frame received #"
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
                    HIVELOG_ERROR(m_log, "failed handle received frame: " << ex.what());
                    resetSerial(true);
                }
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
            resetSerial(true);
        }
    }


    /// @brief Create device from the JSON data.
    /**
    @param[in] jdev The JSON device description.
    */
    void createDevice(json::Value const& jdev)
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

            m_device = cloud7::Device::create(id, name, key, deviceClass, network);
            m_device->status = "Online";

            m_delayedNotifications.clear();
            m_deviceRegistered = false;
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

                m_device->equipment.push_back(cloud7::Equipment::create(id, name, type));
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
        HIVELOG_INFO(m_log, "process intent #" << intent << " data: " << data << "\n");

        if (intent == gateway::INTENT_REGISTRATION_RESPONSE)
        {
            m_gw.handleRegisterResponse(data);
            createDevice(data);
            if (m_serverAPI->isOpen())
                asyncRegisterDevice(m_device);
        }

        else if (intent == gateway::INTENT_REGISTRATION2_RESPONSE)
        {
            json::Value jdev = json::fromStr(data["json"].asString());
            m_gw.handleRegister2Response(jdev);
            createDevice(jdev);
            if (m_serverAPI->isOpen())
                asyncRegisterDevice(m_device);
        }

        else if (intent == gateway::INTENT_COMMAND_RESULT_RESPONSE)
        {
            if (m_device)
            {
                HIVELOG_DEBUG(m_log, "got command result");

                cloud7::Command cmd;
                cmd.id = data["id"].asUInt();
                cmd.status = data["status"].asString();
                cmd.result = data["result"].asString();

                if (m_deviceRegistered && m_serverAPI->isOpen())
                    asyncUpdateCommand(m_device, cmd);
                else
                    m_delayedCommandResults.push_back(cmd);
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

                    if (m_deviceRegistered && m_serverAPI->isOpen())
                    {
                        asyncInsertNotification(m_device,
                            cloud7::Notification(name, data["parameters"]));
                    }
                    else
                    {
                        HIVELOG_DEBUG_STR(m_log, "device is not registered, notification delayed");
                        m_delayedNotifications.push_back(cloud7::Notification(name, data["parameters"]));
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
    typedef gateway::API<boost::asio::serial_port> GatewayAPI; ///< @brief The gateway %API type.
    GatewayAPI::SharedPtr m_gw_api; ///< @brief The gateway %API.
    gateway::Engine m_gw; ///< @brief The gateway engine.

private:
    cloud7::DevicePtr m_device; ///< @brief The device.
    bool m_deviceRegistered; ///< @brief The DeviceHive cloud "registered" flag.
    std::vector<cloud7::Notification> m_delayedNotifications; ///< @brief The list of delayed notification.
    std::vector<cloud7::Command> m_delayedCommandResults; ///< @brief The list of delayed command results.
    std::vector<cloud7::Command> m_delayedCommands; ///< @brief The list of delayed commands.
};


/// @brief The simple gateway application entry point.
/**
Creates the Application instance and calls its Application::run() method.

@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
@param[in] useNewWS Use new websocket service flag.
*/
inline void main(int argc, const char* argv[], bool useNewWS = false)
{
    { // configure logging
        using namespace hive::log;

        Target::SharedPtr log_file = Target::File::create("simple_gw.log");
        Target::SharedPtr log_console = Logger::root().getTarget();
        Logger::root().setTarget(Target::Tie::create(log_file, log_console));
        Logger::root().setLevel(LEVEL_TRACE);
        Logger("/hive/http").setTarget(log_file).setLevel(LEVEL_DEBUG); // disable annoying messages
        log_console->setFormat(Format::create("%N %L %M\n"));
        log_console->setMinimumLevel(LEVEL_DEBUG);
    }

    if (useNewWS)
        ApplicationWS::create(argc, argv)->run();
    else
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
