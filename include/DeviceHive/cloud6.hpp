/** @file
@brief The DeviceHive framework prototype (experimental).
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#ifndef __SANDBOX_CLOUD6_HPP_
#define __SANDBOX_CLOUD6_HPP_

#include <hive/defs.hpp>
#include <hive/http.hpp>
#include <hive/json.hpp>
#include <hive/log.hpp>

#if !defined(HIVE_PCH)
#   include <boost/algorithm/string.hpp>
#   include <boost/lexical_cast.hpp>
#   include <boost/shared_ptr.hpp>
#   include <boost/asio.hpp>
#   include <fstream>
#   include <sstream>
#   include <iomanip>
#endif // HIVE_PCH


/// @brief The DeviceHive framework prototype (experimental).
namespace cloud6
{
    using namespace hive;


/// @brief The network.
/**
Represents an isolation entity that encapsulates multiple devices with controlled access.

Main properties are network's name and key.
*/
class Network
{
public:
    UInt64 id; ///< @brief The DB identifier from server.
    String name; ///< @brief The network name.
    String key; ///< @brief The authorization key.
    String desc; ///< @brief The description.

protected:

    /// @brief The default constructor.
    Network()
        : id(0)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Network> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] name The network name.
    @param[in] key The authorization key.
    @param[in] desc The description.
    @return The new network instance.
    */
    static SharedPtr create(String const& name, String const& key, String const& desc)
    {
        SharedPtr pthis(new Network());
        pthis->name = name;
        pthis->key = key;
        pthis->desc = desc;
        return pthis;
    }
};

/// @brief The network shared pointer type.
typedef Network::SharedPtr NetworkPtr;


/// @brief The equipment.
/**
Represents meta-information about one unit of equipment devices have onboard.

The equipment identifier may be any non-empty string.

Main properties are name and type.
*/
class Equipment
{
public:
    String id; ///< @brief The unique identifier.
    String name; ///< @brief The custom name.
    String type; ///< @brief The equipment type.

protected:

    /// @brief The default constructor.
    Equipment()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Equipment> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] id The identifier.
    @param[in] name The custom name.
    @param[in] type The equipment type.
    @return The new equipment instance.
    */
    static SharedPtr create(String const& id, String const& name, String const& type)
    {
        SharedPtr pthis(new Equipment());
        pthis->id = id;
        pthis->name = name;
        pthis->type = type;
        return pthis;
    }


    /// @brief The trivial destructor.
    virtual ~Equipment()
    {}
};

/// @brief The equipment shared pointer type.
typedef Equipment::SharedPtr EquipmentPtr;


/// @brief The device.
/**
Represents a unit that runs microcode and controls a set of equipment.

The device identifier should be a valid GUID string written in lower case!

Contains network, device class and list of equipment.
*/
class Device
{
public:

    /// @brief The device class.
    /**
    Represents an entity that holds all meta-information about particular device type.
    */
    class Class
    {
    public:
        UInt64 id; ///< @brief The DB identifier from server.
        String name;   ///< @brief The name.
        String version; ///< @brief The version.
        bool isPermanent; ///< @brief The "permanent" flag.
        int offlineTimeout; ///< @brief The offline timeout, seconds.

    protected:

        /// @brief The default constructor.
        Class()
            : id(0), isPermanent(false),
              offlineTimeout(0)
        {}

    public:

        /// @brief The shared pointer type.
        typedef boost::shared_ptr<Class> SharedPtr;


        /// @brief The factory method.
        /**
        @param[in] name The name.
        @param[in] ver The version.
        @param[in] isPermanent The "permanent" flag.
        @param[in] offlineTimeout The offline timeout, seconds.
        @return The new device class instance.
        */
        static SharedPtr create(String const& name, String const& ver,
            bool isPermanent = false, int offlineTimeout = 0)
        {
            SharedPtr pthis(new Class());
            pthis->name = name;
            pthis->version = ver;
            pthis->isPermanent = isPermanent;
            pthis->offlineTimeout = offlineTimeout;
            return pthis;
        }
    };

    /// @brief The device class shared pointer type.
    typedef Class::SharedPtr ClassPtr;

public:
    String id; ///< @brief The unique identifier (UUID).
    String name; ///< @brief The custom name.
    String key; ///< @brief The authorization key.
    String status; ///< @brief The custom device status.

    NetworkPtr network; ///< @brief The corresponding network.
    ClassPtr deviceClass; ///< @brief The corresponding device class.

    std::vector<EquipmentPtr> equipment; ///< @brief The equipment.

protected:

    /// @brief The default constructor.
    Device()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Device> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] id The unique identifier.
    @param[in] name The name.
    @param[in] key The authorization key.
    @param[in] deviceClass The device class.
    @param[in] network The network.
    @return The new device.
    */
    static SharedPtr create(String const& id, String const& name,
        String const& key, ClassPtr deviceClass, NetworkPtr network)
    {
        SharedPtr pthis(new Device());
        pthis->id = id;
        pthis->name = name;
        pthis->key = key;
        pthis->network = network;
        pthis->deviceClass = deviceClass;
        return pthis;
    }

public:

    /// @brief Find equipment by identifier.
    /**
    @param[in] id The equipment identifier.
    @return The equipment or NULL.
    */
    EquipmentPtr findEquipmentById(String const& id)
    {
        const size_t N = equipment.size();
        for (size_t i = 0; i < N; ++i)
        {
            if (equipment[i]->id == id)
                return equipment[i];
        }

        return EquipmentPtr(); // not found
    }
};

/// @brief The device shared pointer type.
typedef Device::SharedPtr DevicePtr;


/// @brief The command.
/**
Represents a message dispatched by clients for devices.
*/
class Command
{
public:
    UInt64 id; ///< @brief The command identifier.
    String name; ///< @brief The command name.
    json::Value params; ///< @brief The command parameters.
    int lifetime; ///< @brief The number of seconds until this command expires.
    int flags; ///< @brief Optional flags.
    String status; ///< @brief Command status.
    String result; ///< @brief Command result.

public:

    /// @brief The default constructor.
    Command()
        : id(0), lifetime(0), flags(0)
    {}
};


/// @brief The notification.
/**
Represents a message dispatched by devices for clients.
*/
class Notification
{
public:
    UInt64 id; ///< @brief The notification identifier.
    String name; ///< @brief The notification name.
    json::Value params; ///< @brief The notification parameters.

public:

    /// @brief The default constructor.
    Notification()
        : id(0)
    {}
};


/// @brief The could version 6 server API.
/**
This class helps devices to comminucate with server.

There are a lot of API wrapper methods:
    - asyncRegisterDevice()
    - asyncPollCommands()
    - asyncSendCommandResult()
    - asyncSendNotification()

All of these methods accept callback functor which is used to report result of each operation.
*/
class ServerAPI:
    public boost::enable_shared_from_this<ServerAPI>
{
    typedef ServerAPI ThisType; ///< @brief The type alias.
public:

/// @brief The JSON serializer.
/**
This class helps to serialize/deserialize key DeviceHive objects to/from JSON values.
*/
class Serializer
{
public:

    /// @brief Convert the JSON value to the notification.
    /**
    @param[in] jval The JSON value to convert.
    @return The notification.
    */
    static Notification json2ntf(json::Value const& jval)
    {
        try
        {
            Notification ntf;
            ntf.id = jval["id"].asUInt();
            ntf.name = jval["notification"].asString();
            ntf.params = jval["parameters"];
            return ntf;
        }
        catch (std::exception const& ex)
        {
            OStringStream ess;
            ess << "failed to deserialize Notification:\n"
                << ex.what();
            throw std::runtime_error(ess.str().c_str());
        }
    }


    /// @brief Convert the notification to the JSON value.
    /**
    @param[in] ntf The notification to convert.
    @return The JSON value.
    */
    static json::Value ntf2json(Notification const& ntf)
    {
        json::Value jval;
        //jval["id"] = ntf.id;
        jval["notification"] = ntf.name;
        jval["parameters"] = ntf.params;
        return jval;
    }

public:

    /// @brief Convert the JSON value to the command.
    /**
    @param[in] jval The JSON value to convert.
    @return The command.
    */
    static Command json2cmd(json::Value const& jval)
    {
        try
        {
            Command cmd;
            cmd.id = jval["id"].asUInt();
            cmd.name = jval["command"].asString();
            cmd.params = jval["parameters"];
            cmd.lifetime = int(jval["lifetime"].asInt());
            cmd.flags = int(jval["flags"].asInt());
            cmd.status = jval["status"].asString();
            cmd.result = jval["result"].asString();
            return cmd;
        }
        catch (std::exception const& ex)
        {
            OStringStream ess;
            ess << "failed to deserialize Command:\n"
                << ex.what();
            throw std::runtime_error(ess.str().c_str());
        }
    }


    /// @brief Convert the command to the JSON value.
    /**
    @param[in] cmd The command to convert.
    @return The JSON value.
    */
    static json::Value cmd2json(Command const& cmd)
    {
        json::Value jval;
        //jval["id"] = cmd.id;
        jval["command"] = cmd.name;
        jval["parameters"] = cmd.params;
        jval["lifetime"] = cmd.lifetime;
        jval["flags"] = cmd.flags;
        jval["status"] = cmd.status;
        jval["result"] = cmd.result;
        return jval;
    }

public:

    /// @brief Update device class from the JSON value.
    /**
    @param[in] jval The JSON value.
    @param[in,out] deviceClass The device class to update.
    */
    static void json2deviceClass(json::Value const& jval, Device::ClassPtr deviceClass)
    {
        try
        {
            deviceClass->id = jval["id"].asUInt();
            deviceClass->name = jval["name"].asString();
            deviceClass->version = jval["version"].asString();
            deviceClass->isPermanent = jval["isPermanent"].asBool();
            deviceClass->offlineTimeout = int(jval["offlineTimeout"].asInt());
        }
        catch (std::exception const& ex)
        {
            OStringStream ess;
            ess << "failed to update Device Class:\n"
                << ex.what();
            throw std::runtime_error(ess.str().c_str());
        }
    }


    /// @brief Convert device class to the JSON value.
    /**
    @param[in] deviceClass The device class.
    @return The JSON value.
    */
    static json::Value deviceClass2json(Device::ClassPtr deviceClass)
    {
        json::Value jval;
        //jval["id"] = deviceClass->id;
        jval["name"] = deviceClass->name;
        jval["version"] = deviceClass->version;
        jval["isPermanent"] = deviceClass->isPermanent;
        if (0 < deviceClass->offlineTimeout)
            jval["offlineTimeout"] = deviceClass->offlineTimeout;
        return jval;
    }

public:

    /// @brief Update network from the JSON value.
    /**
    @param[in] jval The JSON value.
    @param[in,out] network The network to update.
    */
    static void json2network(json::Value const& jval, NetworkPtr network)
    {
        try
        {
            network->id = jval["id"].asUInt();
            network->name = jval["name"].asString();
            //network->key = jval["key"].asString();
            network->desc = jval["description"].asString();
        }
        catch (std::exception const& ex)
        {
            OStringStream ess;
            ess << "failed to update Network:\n"
                << ex.what();
            throw std::runtime_error(ess.str().c_str());
        }
    }


    /// @brief Convert network to the JSON value.
    /**
    @param[in] network The network.
    @return The JSON value.
    */
    static json::Value network2json(NetworkPtr network)
    {
        json::Value jval;
        //jval["id"] = json::UInt64(network->id);
        jval["name"] = network->name;
        if (!network->key.empty())
            jval["key"] = network->key;
        jval["description"] = network->desc;
        return jval;
    }

public:

    /// @brief Update equipment from the JSON value.
    /**
    @param[in] jval The JSON value.
    @param[in,out] device The device to update equipment for.
    */
    static void json2equipment(json::Value const& jval, DevicePtr device)
    {
        try
        {
            const String id = jval["code"].asString();
            if (id.empty())
                throw std::runtime_error("identifier is empty");

            EquipmentPtr eq = device->findEquipmentById(id);
            if (!eq)
                throw std::runtime_error("equipment not found");

            eq->name = jval["name"].asString();
            eq->type = jval["type"].asString();
        }
        catch (std::exception const& ex)
        {
            OStringStream ess;
            ess << "failed to update Equipment:\n"
                << ex.what();
            throw std::runtime_error(ess.str().c_str());
        }
    }


    /// @brief Convert equipment to the JSON value.
    /**
    @param[in] eq The equipment.
    @param[in] deviceClass The corresponding device class.
    @return The JSON value.
    */
    static json::Value equipment2json(EquipmentPtr eq, Device::ClassPtr deviceClass)
    {
        json::Value jval;
        jval["code"] = eq->id;
        jval["name"] = eq->name;
        jval["type"] = eq->type;
        jval["deviceClass"] = json::Value(); //deviceClass2json(deviceClass);
        return jval;
    }

public:

    /// @brief Update device from the JSON value.
    /**
    @param[in] jval The JSON value.
    @param[in,out] device The device to update.
    */
    static void json2device(json::Value const& jval, DevicePtr device)
    {
        try
        {
            { // just check identifier is the same!
                const String id = jval["id"].asString();
                if (id.empty())
                    throw std::runtime_error("identifier is empty");
                if (device->id != id)
                    throw std::runtime_error("invalid identifier");
            }

            device->name = jval["name"].asString();
            //device->key = jval["key"].asString();
            device->status = jval["status"].asString();
            json2network(jval["network"], device->network);
            json2deviceClass(jval["deviceClass"], device->deviceClass);

            { // equipment
                json::Value const& json_eq = jval["equipment"];
                if (!json_eq.isNull() && !json_eq.isArray())
                    throw std::runtime_error("equipment is not an array");

                typedef json::Value::ElementIterator Iterator;
                Iterator i = json_eq.elementsBegin();
                const Iterator e = json_eq.elementsEnd();
                for (; i != e; ++i)
                    json2equipment(*i, device);
            }
        }
        catch (std::exception const& ex)
        {
            OStringStream ess;
            ess << "failed to update Device:\n"
                << ex.what();
            throw std::runtime_error(ess.str().c_str());
        }
    }


    /// @brief Convert device to the JSON value.
    /**
    @param[in] device The device.
    @return The JSON value.
    */
    static json::Value device2json(DevicePtr device)
    {
        const size_t N = device->equipment.size();
        json::Value json_eq;
        json_eq.resize(N);
        for (size_t i = 0; i < N; ++i)
        {
            const EquipmentPtr eq = device->equipment[i];
            json_eq[i] = equipment2json(eq, device->deviceClass);
        }

        json::Value jval;
        //jval["id"] = device->id;
        jval["name"] = device->name;
        jval["key"] = device->key;
        jval["status"] = device->status;
        jval["network"] = network2json(device->network);
        jval["deviceClass"] = deviceClass2json(device->deviceClass);
        jval["equipment"] = json_eq;
        return jval;
    }
};

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<ServerAPI> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] httpClient The HTTP client instance.
    @param[in] baseUrl The base URL.
    @return The new instance.
    */
    static SharedPtr create(http::Client::SharedPtr httpClient, String const& baseUrl)
    {
        return SharedPtr(new ThisType(httpClient, baseUrl));
    }

private:
    http::Client::SharedPtr m_http; ///< @brief The HTTP client.
    int m_http_major; ///< @brief The HTTP major version.
    int m_http_minor; ///< @brief The HTTP minor version.

    log::Logger m_log;              ///< @brief The logger.
    http::Url m_baseUrl;            ///< @brief The base URL.
    size_t m_timeout_ms;            ///< @brief The HTTP request timeout, milliseconds.


    /// @brief The main constructor.
    /**
    @param[in] httpClient The HTTP client instance.
    @param[in] baseUrl The base URL.
    */
    ServerAPI(http::Client::SharedPtr httpClient, String const& baseUrl)
        : m_http(httpClient), m_http_major(1), m_http_minor(0),
          m_log("CloudV6"), m_baseUrl(baseUrl), m_timeout_ms(60000)
    {}


/// @name Device
/// @{
public:

    /// @brief The "register device" callback type.
    typedef boost::function2<void, boost::system::error_code, Device::SharedPtr> RegisterDeviceCallback;


    /// @brief Register device on the server.
    /**
    @param[in] device The device to register.
    @param[in] callback The callback functor.
    */
    void asyncRegisterDevice(Device::SharedPtr device, RegisterDeviceCallback callback)
    {
        http::Url::Builder urlb(m_baseUrl);
        urlb.appendPath("device");
        urlb.appendPath(device->id);

        const json::Value jcontent = Serializer::device2json(device);
        http::RequestPtr req = http::Request::PUT(urlb.build());
        req->addHeader(http::header::Content_Type, "application/json");
        req->addHeader("Auth-DeviceID", device->id);
        req->addHeader("Auth-DeviceKey", device->key);
        req->setContent(json::json2str(jcontent));
        req->setVersion(m_http_major, m_http_minor);

        HIVELOG_DEBUG(m_log, "register device:\n" << json::json2hstr(jcontent));
        m_http->send(req, boost::bind(&ThisType::onRegisterDevice, shared_from_this(),
            _1, _2, _3, device, callback), m_timeout_ms);
    }

private:

    /// @brief The "register device" completion handler.
    /**
    @param[in] err The error code.
    @param[in] request The HTTP request.
    @param[in] response The HTTP response.
    @param[in] device The device registered.
    @param[in] callback The callback functor.
    */
    void onRegisterDevice(boost::system::error_code err, http::RequestPtr request,
        http::ResponsePtr response, Device::SharedPtr device, RegisterDeviceCallback callback)
    {
        if (!err && response && response->getStatusCode() == http::status::OK)
        {
            // TODO: handle all exceptions
            json::Value jval = json::str2json(response->getContent());
            HIVELOG_DEBUG(m_log, "got \"register device\" response:\n"
                << json::json2hstr(jval));
            Serializer::json2device(jval, device);
            callback(err, device);
        }
        else
        {
            HIVELOG_WARN_STR(m_log, "failed to get \"register device\" response");
            callback(err, device);
        }
    }
/// @}


/// @name Device command
/// @{
public:

    /// @brief The "poll commands" callback type.
    typedef boost::function3<void, boost::system::error_code,
        Device::SharedPtr, std::vector<Command> const&> PollCommandsCallback;


    /// @brief Poll commands from the server.
    /**
    @param[in] device The device to poll commands for.
    @param[in] callback The callback functor.
    */
    void asyncPollCommands(Device::SharedPtr device, PollCommandsCallback callback)
    {
        http::Url::Builder urlb(m_baseUrl);
        urlb.appendPath("device");
        urlb.appendPath(device->id);
        urlb.appendPath("command/poll");

        http::RequestPtr req = http::Request::GET(urlb.build());
        req->addHeader("Auth-DeviceID", device->id);
        req->addHeader("Auth-DeviceKey", device->key);
        req->setVersion(m_http_major, m_http_minor);

        HIVELOG_DEBUG(m_log, "poll commands for \"" << device->id << "\"");
        m_http->send(req, boost::bind(&ThisType::onPollCommands, shared_from_this(),
            _1, _2, _3, device, callback), m_timeout_ms);
    }

private:

    /// @brief The "poll commands" completion handler.
    /**
    @param[in] err The error code.
    @param[in] request The HTTP request.
    @param[in] response The HTTP response.
    @param[in] device The device to poll commands for.
    @param[in] callback The callback functor.
    */
    void onPollCommands(boost::system::error_code err, http::RequestPtr request,
        http::ResponsePtr response, Device::SharedPtr device, PollCommandsCallback callback)
    {
        std::vector<Command> commands;
        json::Value jval;

        if (!err && response && response->getStatusCode() == http::status::OK)
        {
            // TODO: handle all exceptions
            jval = json::str2json(response->getContent());
            if (jval.isArray())
            {
                commands.reserve(jval.size());
                typedef json::Value::ElementIterator Iterator;
                for (Iterator i = jval.elementsBegin(); i != jval.elementsEnd(); ++i)
                    commands.push_back(Serializer::json2cmd(*i));
            }
        }

        HIVELOG_DEBUG(m_log, "got \"poll commands\" response:\n"
            << json::json2hstr(jval));

        callback(err, device, commands);
    }

public:

    // TODO: response callback?

    /// @brief Send command result to the server.
    /**
    @param[in] device The device.
    @param[in] cmd The command.
    */
    void asyncSendCommandResult(Device::SharedPtr device, Command const& cmd)
    {
        http::Url::Builder urlb(m_baseUrl);
        urlb.appendPath("device");
        urlb.appendPath(device->id);
        urlb.appendPath("command");
        urlb.appendPath(boost::lexical_cast<String>(cmd.id));

        json::Value jbody;
        jbody["status"] = cmd.status;
        jbody["result"] = cmd.result;

        http::RequestPtr req = http::Request::PUT(urlb.build());
        req->addHeader(http::header::Content_Type, "application/json");
        req->addHeader("Auth-DeviceID", device->id);
        req->addHeader("Auth-DeviceKey", device->key);
        req->setContent(json::json2str(jbody));
        req->setVersion(m_http_major, m_http_minor);

        HIVELOG_DEBUG(m_log, "command result:\n" << json::json2hstr(jbody));
        m_http->send(req, boost::bind(&ThisType::onSendCommandResult,
            shared_from_this(), _1, _2, _3), m_timeout_ms);
    }

private:

    /// @brief The "send command result" completion handler.
    /**
    @param[in] err The error code.
    @param[in] request The HTTP request.
    @param[in] response The HTTP response.
    */
    void onSendCommandResult(boost::system::error_code err, http::RequestPtr request, http::ResponsePtr response)
    {
        HIVELOG_DEBUG_STR(m_log, "got \"command result\" response");

        // TODO: call the callback
    }
/// @}


/// @name Device notification
/// @{
public:

    // TODO: response callback?

    /// @brief Send notification to the server.
    /**
    @param[in] device The device.
    @param[in] ntf The notification.
    */
    void asyncSendNotification(Device::SharedPtr device, Notification const& ntf)
    {
        http::Url::Builder urlb(m_baseUrl);
        urlb.appendPath("device");
        urlb.appendPath(device->id);
        urlb.appendPath("notification");

        const json::Value jbody = Serializer::ntf2json(ntf);
        http::RequestPtr req = http::Request::POST(urlb.build());
        req->addHeader(http::header::Content_Type, "application/json");
        req->addHeader("Auth-DeviceID", device->id);
        req->addHeader("Auth-DeviceKey", device->key);
        req->setContent(json::json2str(jbody));
        req->setVersion(m_http_major, m_http_minor);

        HIVELOG_DEBUG(m_log, "notification:\n" << json::json2hstr(jbody));
        m_http->send(req, boost::bind(&ThisType::onSendNotification,
            shared_from_this(), _1, _2, _3), m_timeout_ms);
    }

private:

    /// @brief The "send notification" completion handler.
    /**
    @param[in] err The error code.
    @param[in] request The HTTP request.
    @param[in] response The HTTP response.
    */
    void onSendNotification(boost::system::error_code err, http::RequestPtr request, http::ResponsePtr response)
    {
        HIVELOG_DEBUG_STR(m_log, "got \"notification\" response");

        // TODO: call the callback
    }
/// @}

public:

    /// @brief Cancel all requests.
    void cancelAll()
    {
        m_http->cancelAll();
    }
};

} // cloud6 namespace

#endif // __SANDBOX_CLOUD6_HPP_
