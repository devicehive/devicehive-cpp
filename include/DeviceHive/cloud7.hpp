/** @file
@brief The DeviceHive framework prototype (experimental).
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#ifndef __DEVICEHIVE_CLOUD7_HPP_
#define __DEVICEHIVE_CLOUD7_HPP_

#include <DeviceHive/cloud6.hpp>

#include <hive/ws13.hpp>

#if !defined(HIVE_PCH)
#endif // HIVE_PCH


/// @brief The DeviceHive framework prototype (experimental).
namespace cloud7
{
    using namespace hive;

    using cloud6::Network;
    using cloud6::NetworkPtr;
    using cloud6::Equipment;
    using cloud6::EquipmentPtr;
    using cloud6::Device;
    using cloud6::DevicePtr;
    using cloud6::Command;
    using cloud6::Notification;
    using cloud6::Serializer;


/// @brief The websocket could version 7 server API.
/**
This class helps devices to comminucate with server.
*/
class WebSocketAPI:
    public boost::enable_shared_from_this<WebSocketAPI>
{
    typedef WebSocketAPI This; ///< @brief The type alias.

private:

    /// @brief The main constructor.
    /**
    @param[in] httpClient The HTTP client instance.
    @param[in] baseUrl The base URL.
    @param[in] name The custom name. Optional.
    */
    WebSocketAPI(http::Client::SharedPtr httpClient, String const& baseUrl, String const& name)
        : m_ios(httpClient->getIoService())
        , m_http(httpClient)
        , m_ws(ws13::WebSocket::create(name))
        , m_baseUrl(baseUrl)
        , m_timeout_ms(60000)
        , m_log("/WebSocketAPI/" + name)
    {
        HIVELOG_TRACE_STR(m_log, "created");
    }


public:

    /// @brief The trivial destructor.
    ~WebSocketAPI()
    {
        HIVELOG_TRACE_STR(m_log, "deleted");
    }

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<WebSocketAPI> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] httpClient The HTTP client instance.
    @param[in] baseUrl The base URL.
    @param[in] name The optional custom name.
    @return The new instance.
    */
    static SharedPtr create(http::Client::SharedPtr httpClient,
        String const& baseUrl, String const& name = String())
    {
        return SharedPtr(new This(httpClient, baseUrl, name));
    }

public:

    /// @brief The "connect" callback type.
    typedef boost::function1<void, boost::system::error_code> ConnectCallback;


    /// @brief Is the connection open?
    /**
    @return `true` if connection is open.
    */
    bool isOpen() const
    {
        return m_ws->isOpen();
    }


    /// @brief Make connection.
    /**
    @param[in] callback The callback functor.
    */
    void asyncConnect(ConnectCallback callback)
    {
        assert(!m_ws->isOpen() && "already open");

        http::Url::Builder url(m_baseUrl);
        url.appendPath("device");

        m_ws->asyncConnect(url.build(), m_http,
            boost::bind(&This::onConnect,
                shared_from_this(),
                _1, _2, callback),
            m_timeout_ms);
    }


    /// @brief Close the connection.
    void close(bool force = false)
    {
        m_ws->close(force);
        m_ws->asyncListenForMessages(ws13::WebSocket::RecvMessageCallbackType());
        //m_ws->asyncListenForFrames(ws13::WebSocket::RecvFrameCallback());
        listenForActions(ActionReceivedCallback());
    }

private:

    /// @brief The "connect" callback.
    /**
    @param[in] err The error code.
    @param[in] ws The corresponding WebSocket connection.
    @param[in] callback The callback functor.
    */
    void onConnect(boost::system::error_code err, ws13::WebSocket::SharedPtr ws, ConnectCallback callback)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onConnect()");

        if (!err)
        {
            HIVELOG_INFO(m_log, "connected");

            m_ws->asyncListenForMessages(
                boost::bind(&This::onMessageReceived,
                shared_from_this(), _1, _2));
        }
        else
        {
            HIVELOG_ERROR(m_log, "connection failed: ["
                << err << "] - " << err.message());
        }

        if (callback)
        {
            m_ios.post(boost::bind(callback, err));
        }
    }

public:

    /// @brief The "action sent" callback type.
    typedef boost::function2<void, boost::system::error_code, json::Value> ActionSentCallback;


    /// @brief Send the action.
    /**
    @param[in] jaction The action.
    @param[in] callback The callback functor.
    */
    void asyncSendAction(json::Value const& jaction, ActionSentCallback callback)
    {
        assert(m_ws->isOpen() && "no connection");

        m_ws->asyncSendMessage(ws13::Message::create(json::toStr(jaction)),
            boost::bind(&This::onMessageSent, shared_from_this(), _1, _2, callback));
    }

private:

    /// @brief The "action sent" callback.
    /**
    @param[in] err The error code.
    @param[in] msg The sent message.
    @param[in] callback The callback functor.
    */
    void onMessageSent(boost::system::error_code err, ws13::Message::SharedPtr msg, ActionSentCallback callback)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onActionSent()");

        if (callback)
        {
            json::Value jaction = json::fromStr(msg->getData());
            m_ios.post(boost::bind(callback, err, jaction)); // TODO: avoid copy of action?
        }
    }

public:

    /// @brief The "action received" callback type.
    typedef boost::function2<void, boost::system::error_code, json::Value const&> ActionReceivedCallback;


    /// @brief Listen for the actions.
    /**
    @param[in] callback The callback functor.
    NULL to stop listening.
    */
    void listenForActions(ActionReceivedCallback callback)
    {
        HIVELOG_DEBUG(m_log, (callback ? "start":"stop")
            << " listening for the actions");

        m_actionReceivedCallback = callback;
    }

private:
    ActionReceivedCallback m_actionReceivedCallback; ///< @brief The "action received" callback.

    /// @brief The message handler.
    /**
    @param[in] err The error code.
    @param[in] msg The received websocket message.
    */
    void onMessageReceived(boost::system::error_code err, ws13::Message::SharedPtr msg)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onMessageReceived()");

        json::Value jaction;

        if (!err)
        {
            try
            {
                if (!msg || !msg->isText())
                    throw std::runtime_error("null or not a text");

                HIVELOG_DEBUG(m_log, "text received: \"" << msg->getData() << "\"");
                jaction = json::fromStr(msg->getData());
                HIVELOG_DEBUG(m_log, "converted to: " << json::toStrH(jaction));
            }
            catch (std::exception const& ex)
            {
                HIVELOG_ERROR(m_log, "bad message: " << ex.what());
                err = boost::system::errc::make_error_code(boost::system::errc::bad_message);
            }
        }

        if (m_actionReceivedCallback)
            m_ios.post(boost::bind(m_actionReceivedCallback, err, jaction));
        else
            HIVELOG_WARN(m_log, "no action callback, ignored: " << jaction);
    }

private:
    boost::asio::io_service &m_ios; ///< @brief The IO service.
    http::Client::SharedPtr m_http; ///< @brief The HTTP client.
    ws13::WebSocketPtr m_ws; ///< @brief The websocket connection.

    http::Url m_baseUrl;            ///< @brief The base URL.
    size_t m_timeout_ms;            ///< @brief The HTTP request timeout, milliseconds.
    log::Logger m_log;              ///< @brief The logger.
};



/// @brief The WebSocket Server API submodule.
/**
This is helper class.

You have to call initWsServerModule() method before use any of the class methods.
The best place to do that is static factory method of your application.
*/
class ServerModuleWS
{
    /// @brief The this type alias.
    typedef ServerModuleWS This;

protected:

    /// @brief The main constructor.
    /**
    @param[in] ios The IO service.
    @param[in] logger The logger.
    */
    ServerModuleWS(boost::asio::io_service &ios, log::Logger const& logger)
        : m_httpClient(http::Client::create(ios))
        , m_serverOpenTimer(ios)
        , m_log_(logger)
    {}


    /// @brief The trivial destructor.
    virtual ~ServerModuleWS()
    {}


    /// @brief Initialize server module.
    /**
    @param[in] baseUrl The server base URL.
    @param[in] pthis The this pointer.
    */
    void initServerModuleWS(String const& baseUrl, boost::shared_ptr<ServerModuleWS> pthis)
    {
        m_serverAPI = WebSocketAPI::create(m_httpClient, baseUrl);
        m_this = pthis;
    }


    /// @brief Cancel all server tasks.
    void cancelServerModuleWS()
    {
        m_serverAPI->close();
        m_serverOpenTimer.cancel();
    }

protected:

    /// @brief Connect to the server.
    /**
    @param[in] wait_sec The number of seconds to wait before open.
    */
    virtual void asyncConnectToServer(long wait_sec)
    {
        assert(!m_this.expired() && "Application is dead or not initialized");

        HIVELOG_TRACE(m_log_, "try to open server connection after " << wait_sec << " seconds");
        m_serverOpenTimer.expires_from_now(boost::posix_time::seconds(wait_sec));
        m_serverOpenTimer.async_wait(boost::bind(&This::onTryToConnectToServer,
            m_this.lock(), boost::asio::placeholders::error));
    }


    /// @brief Try to connect to the server.
    virtual void onTryToConnectToServer(boost::system::error_code err)
    {
        if (!err)
        {
            m_serverAPI->asyncConnect(
                boost::bind(&This::onConnectedToServer,
                    m_this.lock(), _1));
        }
        else
        {
            HIVELOG_ERROR(m_log_, "connection timer error: ["
                << err << "] " << err.message());
        }
    }


    /// @brief Connected to the server handler.
    virtual void onConnectedToServer(boost::system::error_code err)
    {
        if (!err)
        {
            HIVELOG_INFO_STR(m_log_, "connected");

            m_serverAPI->listenForActions(
                boost::bind(&This::onActionReceived,
                    m_this.lock(), _1, _2));
        }
        else
        {
            HIVELOG_ERROR(m_log_, "connection error: ["
                << err << "] " << err.message());

            // TODO: reconnect after timeout!
        }
    }

protected:

    /// @brief The "action received" handler.
    /**
    @param[in] err The error code.
    @param[in] jaction The received action.
    */
    virtual void onActionReceived(boost::system::error_code err, json::Value const& jaction)
    {
        HIVELOG_DEBUG(m_log_, "got action: " << json::toStrH(jaction));
    }

protected:

    /// @brief Send the action.
    /**
    @param[in] action The action to send.
    */
    virtual void asyncSendAction(json::Value const& action)
    {
        assert(!m_this.expired() && "Application is dead or module not initialized");

        m_serverAPI->asyncSendAction(action,
            boost::bind(&This::onActionSent,
            m_this.lock(), _1, _2));
    }


    /// @brief The "action sent" handler.
    /**
    @param[in] err The error code.
    @param[in] action The sent action.
    */
    virtual void onActionSent(boost::system::error_code err, json::Value action)
    {
        HIVELOG_DEBUG(m_log_, "action sent: " << json::toStrH(action));
    }

protected:

    /// @brief Send the authenticate action.
    /**
    @param[in] deviceId The device identifier.
    @param[in] deviceKey The device key.
    */
    void asyncAuthenticate(String const& deviceId, String const& deviceKey)
    {
        json::Value jaction;
        jaction["action"] = "authenticate";
        jaction["deviceId"] = deviceId;
        jaction["deviceKey"] = deviceKey;

        asyncSendAction(jaction);
    }


    /// @brief Send the register device action.
    /**
    @param[in] device The device to register.
    */
    void asyncRegisterDevice(DevicePtr device)
    {
        json::Value jaction;
        jaction["action"] = "device/save";
        jaction["deviceId"] = device->id;
        jaction["deviceKey"] = device->key;
        jaction["device"] = Serializer::device2json(device);

        asyncSendAction(jaction);
    }


    /// @brief Subscribe for device commands.
    /**
    @param[in] device The device to poll commands for.
    */
    void asyncSubscribeForCommands(DevicePtr device)
    {
        json::Value jaction;
        jaction["action"] = "command/subscribe";
        jaction["deviceId"] = device->id;
        jaction["deviceKey"] = device->key;

        asyncSendAction(jaction);
    }


    /// @brief Unsubscribe from commands asynchronously.
    /**
    @param[in] device The device to unsubscribe.
    */
    void asyncUnsubscribeForCommands(DevicePtr device)
    {
        json::Value jaction;
        jaction["action"] = "command/unsubscribe";
        jaction["deviceId"] = device->id;
        jaction["deviceKey"] = device->key;

        asyncSendAction(jaction);
    }


    /// @brief Update command result.
    /**
    @param[in] device The device.
    @param[in] command The command to update.
    */
    void asyncUpdateCommand(DevicePtr device, Command const& command)
    {
        json::Value jaction;
        jaction["action"] = "command/update";
        jaction["deviceId"] = device->id;
        jaction["deviceKey"] = device->key;
        jaction["commandId"] = command.id;
        //jaction["command"] = Serializer::cmd2json(command);
        jaction["command"]["status"] = command.status;
        jaction["command"]["result"] = command.result;

        asyncSendAction(jaction);
    }


    /// @brief Insert notification.
    /**
    @param[in] device The device.
    @param[in] notification The notification to insert.
    */
    void asyncInsertNotification(DevicePtr device, Notification const& notification)
    {
        json::Value jaction;
        jaction["action"] = "notification/insert";
        jaction["deviceId"] = device->id;
        jaction["deviceKey"] = device->key;
        jaction["notification"] = Serializer::ntf2json(notification);

        asyncSendAction(jaction);
    }

protected:
    http::Client::SharedPtr m_httpClient; ///< @brief The HTTP client.
    WebSocketAPI::SharedPtr m_serverAPI; ///< @brief The server API.
    boost::asio::deadline_timer m_serverOpenTimer; ///< @brief Open the server connection.

private:
    boost::weak_ptr<This> m_this; ///< @brief The weak pointer to this.
    log::Logger m_log_; ///< @brief The module logger.
};

} // cloud7 namespace

#endif // __DEVICEHIVE_CLOUD7_HPP_
