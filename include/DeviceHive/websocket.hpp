/** @file
@brief The DeviceHive Websocket service.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#ifndef __DEVICEHIVE_WEBSOCKET_HPP_
#define __DEVICEHIVE_WEBSOCKET_HPP_

#include <DeviceHive/service.hpp>

#include <hive/ws13.hpp>

#include <set>

namespace devicehive
{

/// @brief The Websocket service API.
/**
This class helps devices to comminucate with server via Websocket interface.
*/
class WebsocketServiceBase:
    public boost::enable_shared_from_this<WebsocketServiceBase>
{
    typedef WebsocketServiceBase This; ///< @brief The type alias.

protected:

    /// @brief The main constructor.
    /**
    @param[in] httpClient The HTTP client instance.
    @param[in] baseUrl The base URL.
    @param[in] name The custom name. Optional.
    */
    WebsocketServiceBase(http::Client::SharedPtr httpClient, String const& baseUrl, String const& name)
        : m_ios(httpClient->getIoService())
        , m_http(httpClient)
        , m_ws(ws13::WebSocket::create(name))
        , m_log("/devicehive/websocket/" + name)
        , m_baseUrl(baseUrl)
        , m_timeout_ms(60000)
    {}

public:

    /// @brief The trivial destructor.
    virtual ~WebsocketServiceBase()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<WebsocketServiceBase> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] httpClient The HTTP client instance.
    @param[in] baseUrl The base URL.
    @param[in] name The optional custom name.
    @return The new Websocket service instance.
    */
    static SharedPtr create(http::ClientPtr httpClient,
        String const& baseUrl, String const& name = String())
    {
        return SharedPtr(new This(httpClient, baseUrl, name));
    }

public:

    /// @brief Get the web request timeout.
    /**
    @return The web request timeout, milliseconds.
    */
    size_t getTimeout() const
    {
        return m_timeout_ms;
    }


    /// @brief Set web request timeout.
    /**
    @param[in] timeout_ms The web request timeout, milliseconds.
    @return Self reference.
    */
    This& setTimeout(size_t timeout_ms)
    {
        m_timeout_ms = timeout_ms;
        return *this;
    }

public:

    /// @brief Get HTTP client.
    /**
    @return The HTTP client.
    */
    http::ClientPtr getHttpClient() const
    {
        return m_http;
    }


    /// @brief Cancel all requests.
    void cancelAll()
    {
        // TODO: cancel only related requests
        m_http->cancelAll();

        close();
    }

public:

    /// @brief The "connected" callback type.
    typedef boost::function1<void, boost::system::error_code> ConnectedCallback;


    /// @brief Make connection.
    /**
    @param[in] callback The callback functor.
    */
    void asyncConnect(ConnectedCallback callback)
    {
        assert(!isOpen() && "already open");

        http::Url::Builder url(m_baseUrl);
        url.appendPath("device");
        // TODO: client websocket!!!

        m_ws->asyncConnect(url.build(), m_http,
            boost::bind(&This::onConnected,
                shared_from_this(),
                _1, _2, callback),
            m_timeout_ms);
    }


    /// @brief Is the connection open?
    /**
    @return `true` if connection is open.
    */
    bool isOpen() const
    {
        return m_ws->isOpen();
    }


    /// @brief Close the connection.
    void close(bool force = false)
    {
        if (m_ws->isOpen())
        {
            m_ws->close(force);
            m_ws->asyncListenForMessages(ws13::WebSocket::RecvMessageCallbackType());
            //m_ws->asyncListenForFrames(ws13::WebSocket::RecvFrameCallback());
            listenForActions(ActionReceivedCallback());
        }
    }

private:

    /// @brief The "connected" callback.
    /**
    @param[in] err The error code.
    @param[in] ws The corresponding WebSocket connection.
    @param[in] callback The callback functor.
    */
    void onConnected(boost::system::error_code err, ws13::WebSocket::SharedPtr ws, ConnectedCallback callback)
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
                << err << "] " << err.message());
        }

        if (callback)
            callback(err);
    }

public:

    /// @brief The "action sent" callback type.
    typedef boost::function2<void, boost::system::error_code, json::Value> ActionSentCallback;


    /// @brief Send an action.
    /**
    @param[in] jaction The action.
    @param[in] callback The callback functor.
    */
    void asyncSendAction(json::Value const& jaction, ActionSentCallback callback)
    {
        if (m_ws->isOpen())
        {
            m_ws->asyncSendMessage(ws13::Message::create(json::toStr(jaction)),
                boost::bind(&This::onMessageSent, shared_from_this(), _1, _2, callback));
        }
        else
            assert(!"no connection");
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
        json::Value jaction;

        try
        {
            jaction = json::fromStr(msg->getData());
        }
        catch (std::exception const& ex)
        {
            HIVELOG_ERROR(m_log, "failed to parse \"action\": " << ex.what());
            err = boost::asio::error::fault; // TODO: useful error code
        }

        if (callback)
            callback(err, jaction);
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
                HIVELOG_DEBUG(m_log, "converted to: " << json::toStrHH(jaction));
            }
            catch (std::exception const& ex)
            {
                HIVELOG_ERROR(m_log, "bad message: " << ex.what());
                err = boost::asio::error::fault; // TODO: useful error code
            }
        }

        if (m_actionReceivedCallback)
            m_actionReceivedCallback(err, jaction);
        else
            HIVELOG_WARN(m_log, "no action callback, ignored: " << jaction);
    }

private:
    boost::asio::io_service &m_ios; ///< @brief The IO service.
    http::Client::SharedPtr m_http; ///< @brief The HTTP client.
    ws13::WebSocketPtr m_ws; ///< @brief The websocket connection.

    hive::log::Logger m_log;        ///< @brief The logger.
    http::Url m_baseUrl;            ///< @brief The base URL.
    size_t m_timeout_ms;            ///< @brief The HTTP request timeout, milliseconds.
};



/// @brief The WebSocket service.
class WebsocketService:
    public WebsocketServiceBase,
    public IDeviceService
{
    typedef WebsocketServiceBase Base; ///< @brief The base class.
    typedef WebsocketService This; ///< @brief The type alias.

protected:

    /// @brief The main constructor.
    /**
    @param[in] httpClient The HTTP client instance.
    @param[in] baseUrl The base URL.
    @param[in] callbacks The events handler.
    @param[in] name The custom name. Optional.
    */
    WebsocketService(http::ClientPtr httpClient, String const& baseUrl, boost::shared_ptr<IDeviceServiceEvents> callbacks, String const& name)
        : Base(httpClient, baseUrl, name)
        , m_callbacks(callbacks)
        , m_requestId(0)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<WebsocketService> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] httpClient The HTTP client instance.
    @param[in] baseUrl The base URL.
    @param[in] callbacks The events handler.
    @param[in] name The custom name. Optional.
    @return The new instance.
    */
    static SharedPtr create(http::Client::SharedPtr httpClient, String const& baseUrl,
        boost::shared_ptr<IDeviceServiceEvents> callbacks, String const& name = String())
    {
        return SharedPtr(new This(httpClient, baseUrl, callbacks, name));
    }


    /// @brief Get the shared pointer.
    /**
    @return The shared pointer to this instance.
    */
    SharedPtr shared_from_this()
    {
        return boost::shared_dynamic_cast<This>(Base::shared_from_this());
    }

public: // IDeviceService

    /// @copydoc IDeviceService::cancelAll()
    virtual void cancelAll()
    {
        Base::cancelAll();
        m_devices.clear();
        m_actions.clear();
    }

public:

    /// @copydoc IDeviceService::asyncConnect()
    virtual void asyncConnect()
    {
        Base::asyncConnect(
            boost::bind(&This::onConnected,
                shared_from_this(), _1));
    }


    /// @copydoc IDeviceService::asyncGetServerInfo()
    virtual void asyncGetServerInfo()
    {
        if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
        {
            const UInt64 reqId = m_requestId++;

            json::Value jaction;
            jaction["action"] = "server/info";
            jaction["requestId"] = reqId;

            // no action tracking

            Base::asyncSendAction(jaction,
                boost::bind(&This::onActionSent,
                    shared_from_this(), _1, _2));
        }
        else
            assert(!"callback is dead or not initialized");
    }

public:

    /// @copydoc IDeviceService::asyncRegisterDevice()
    virtual void asyncRegisterDevice(DevicePtr device)
    {
        if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
        {
            const UInt64 reqId = m_requestId++;

            json::Value jaction;
            jaction["action"] = "device/save";
            jaction["requestId"] = reqId;
            jaction["deviceId"] = device->id;
            jaction["deviceKey"] = device->key;
            jaction["device"] = Serializer::toJson(device);

            m_actions[reqId] = boost::bind(&IDeviceServiceEvents::onRegisterDevice, cb, _1, device);

            Base::asyncSendAction(jaction,
                boost::bind(&This::onActionSent,
                    shared_from_this(), _1, _2));
        }
        else
            assert(!"callback is dead or not initialized");
    }


    /// @copydoc IDeviceService::asyncUpdateDeviceData()
    virtual void asyncUpdateDeviceData(DevicePtr device)
    {
        if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
        {
            const UInt64 reqId = m_requestId++;

            json::Value jaction;
            jaction["action"] = "device/save";
            jaction["requestId"] = reqId;
            jaction["deviceId"] = device->id;
            jaction["deviceKey"] = device->key;
            jaction["device"]["data"] = device->data;

            m_actions[reqId] = boost::bind(&IDeviceServiceEvents::onUpdateDeviceData, cb, _1, device);

            Base::asyncSendAction(jaction,
                boost::bind(&This::onActionSent,
                    shared_from_this(), _1, _2));
        }
        else
            assert(!"callback is dead or not initialized");
    }


public:

    /// @copydoc IDeviceService::asyncSubscribeForCommands()
    virtual void asyncSubscribeForCommands(DevicePtr device, String const& timestamp)
    {
        if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
        {
            const UInt64 reqId = m_requestId++;

            json::Value jaction;
            jaction["action"] = "command/subscribe";
            jaction["requestId"] = reqId;
            jaction["deviceId"] = device->id;
            jaction["deviceKey"] = device->key;
            if (!timestamp.empty())
                jaction["timestamp"] = timestamp;

            // no action tracking yet

            Base::asyncSendAction(jaction,
                boost::bind(&This::onActionSent,
                    shared_from_this(), _1, _2));

            m_devices.insert(device);
        }
        else
            assert(!"callback is dead or not initialized");
    }


    /// @copydoc IDeviceService::asyncUnsubscribeFromCommands()
    virtual void asyncUnsubscribeFromCommands(DevicePtr device)
    {
        if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
        {
            const UInt64 reqId = m_requestId++;

            json::Value jaction;
            jaction["action"] = "command/unsubscribe";
            jaction["requestId"] = reqId;
            jaction["deviceId"] = device->id;
            jaction["deviceKey"] = device->key;

            // no action tracking yet

            Base::asyncSendAction(jaction,
                boost::bind(&This::onActionSent,
                    shared_from_this(), _1, _2));

            m_devices.erase(device);
        }
        else
            assert(!"callback is dead or not initialized");
    }

public:

    /// @copydoc IDeviceService::asyncUpdateCommand()
    virtual void asyncUpdateCommand(DevicePtr device, CommandPtr command)
    {
        if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
        {
            const UInt64 reqId = m_requestId++;

            json::Value jaction;
            jaction["action"] = "command/update";
            jaction["requestId"] = reqId;
            jaction["deviceId"] = device->id;
            jaction["deviceKey"] = device->key;
            jaction["commandId"] = command->id;
            //jaction["command"] = Serializer::toJson(command);
            jaction["command"]["status"] = command->status;
            jaction["command"]["result"] = command->result;

            m_actions[reqId] = boost::bind(&IDeviceServiceEvents::onUpdateCommand, cb, _1, device, command);

            Base::asyncSendAction(jaction,
                boost::bind(&This::onActionSent,
                    shared_from_this(), _1, _2));
        }
        else
            assert(!"callback is dead or not initialized");
    }

public:

    /// @copydoc IDeviceService::asyncInsertNotification()
    virtual void asyncInsertNotification(DevicePtr device, NotificationPtr notification)
    {
        if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
        {
            const UInt64 reqId = m_requestId++;

            json::Value jaction;
            jaction["action"] = "notification/insert";
            jaction["requestId"] = reqId;
            jaction["deviceId"] = device->id;
            jaction["deviceKey"] = device->key;
            jaction["notification"] = Serializer::toJson(notification);

            m_actions[reqId] = boost::bind(&IDeviceServiceEvents::onInsertNotification, cb, _1, device, notification);

            Base::asyncSendAction(jaction,
                boost::bind(&This::onActionSent,
                    shared_from_this(), _1, _2));
        }
        else
            assert(!"callback is dead or not initialized");
    }

private:

    /// @brief The "connected" handler.
    /**
    @param[in] err The error code.
    */
    void onConnected(boost::system::error_code err)
    {
        if (!err)
        {
            Base::listenForActions(
                boost::bind(&This::onActionReceived,
                    shared_from_this(), _1, _2));
        }

        if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
        {
            cb->onConnected(err);
        }
        else
            assert(!"callback is dead or not initialized");
    }


    /// @brief The "action sent" handler.
    /**
    @param[in] err The error code.
    @param[in] jaction The sent action.
    */
    void onActionSent(boost::system::error_code err, json::Value const& jaction)
    {
        if (err)
        {
            // notify the user if we cannot send action!
            done(err, jaction);
        }
    }


    /// @brief The "action received" handler.
    /**
    @param[in] err The error code.
    @param[in] jaction The received action.
    */
    void onActionReceived(boost::system::error_code err, json::Value const& jaction)
    {
        if (!err)
        {
            const String action = jaction["action"].asString();

            if (boost::iequals(action, "server/info"))
            {
                if (!boost::iequals(jaction["status"].asString(), "success"))
                    err = boost::asio::error::fault; // TODO: useful error code
                else if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
                {
                    const json::Value jinfo = jaction["info"];

                    ServerInfo info;
                    info.api_version = jinfo["apiVersion"].asString();
                    info.timestamp = jinfo["serverTimestamp"].asString();
                    info.alternativeUrl = jinfo["restServerUrl"].asString();

                    cb->onServerInfo(err, info);
                }
            }

            else if (boost::iequals(action, "device/save")) // got "register device" or "update device" response
            {
                if (!boost::iequals(jaction["status"].asString(), "success"))
                    err = boost::asio::error::fault; // TODO: useful error code
            }

            else if (boost::iequals(action, "command/update")
                  || boost::iequals(action, "command/subscribe")
                  || boost::iequals(action, "command/unsubscribe"))
            {
                if (!boost::iequals(jaction["status"].asString(), "success"))
                    err = boost::asio::error::fault; // TODO: useful error code
            }

            else if (boost::iequals(action, "command/insert"))
            {
                if (DevicePtr device = findDeviceById(jaction["deviceGuid"].asString()))
                    if (boost::shared_ptr<IDeviceServiceEvents> cb = m_callbacks.lock())
                {
                    CommandPtr command = Command::create();
                    Serializer::fromJson(jaction["command"], command);
                    cb->onInsertCommand(err, device, command);
                }
            }
        }

        done(err, jaction);
    }


    /// @brief Finish the action.
    /**
    This method calls the corresponding callback functor if any.

    @param[in] err The error code.
    @param[in] jaction The action to finish.
    */
    void done(boost::system::error_code err, json::Value const& jaction)
    {
        const UInt64 reqId = jaction["requestId"].asUInt();
        std::map<UInt64, ActionCallback>::iterator it = m_actions.find(reqId);
        if (it != m_actions.end())
        {
            it->second(err);
            m_actions.erase(it);
        }
    }


    /// @brief Find device by UUID.
    /**
    @param[in] id The device identifer.
    @return The device or NULL.
    */
    DevicePtr findDeviceById(String const& id) const
    {
        std::set<DevicePtr>::const_iterator i = m_devices.begin();
        std::set<DevicePtr>::const_iterator e = m_devices.end();
        for (; i != e; ++i)
        {
            DevicePtr device = *i;
            if (device->id == id)
                return device;
        }

        return DevicePtr(); // not found
    }

private:
    boost::weak_ptr<IDeviceServiceEvents> m_callbacks;
    std::set<DevicePtr> m_devices;

private:
    typedef boost::function1<void, boost::system::error_code> ActionCallback;
    std::map<UInt64, ActionCallback> m_actions;
    UInt64 m_requestId;
};

} // devicehive namespace

#endif // __DEVICEHIVE_WEBSOCKET_HPP_
