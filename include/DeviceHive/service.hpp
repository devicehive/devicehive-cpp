/** @file
@brief The DeviceHive framework.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#ifndef __DEVICEHIVE_SERVICE_HPP_
#define __DEVICEHIVE_SERVICE_HPP_

#include <hive/defs.hpp>
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
#   include <set>
#endif // HIVE_PCH


/// @brief The DeviceHive framework.
namespace devicehive
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

    /// @brief The trivial destructor.
    virtual ~Network()
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
    static SharedPtr create(String const& name,
                            String const& key,
                            String const& desc = String())
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
Represents meta-information about one unit of equipment device has onboard.

The equipment identifier may be any non-empty string.

Main properties are name and type.
*/
class Equipment
{
public:
    UInt64 id; ///< @brief The DB identifier from server.
    String code; ///< @brief The unique identifier.
    String name; ///< @brief The custom name.
    String type; ///< @brief The equipment type.
    json::Value data; ///< @brief The custom data.

protected:

    /// @brief The default constructor.
    Equipment()
        : id(0)
    {}

public:

    /// @brief The trivial destructor.
    virtual ~Equipment()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Equipment> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] code The identifier.
    @param[in] name The custom name.
    @param[in] type The equipment type.
    @return The new equipment instance.
    */
    static SharedPtr create(String const& code,
                            String const& name,
                            String const& type)
    {
        SharedPtr pthis(new Equipment());
        pthis->code = code;
        pthis->name = name;
        pthis->type = type;
        return pthis;
    }
};

/// @brief The equipment shared pointer type.
typedef Equipment::SharedPtr EquipmentPtr;


/// @brief The device.
/**
Represents a unit that runs microcode and controls a set of equipment.

The device identifier should be a valid GUID string!

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
        json::Value data; ///< @brief The custom data.

    protected:

        /// @brief The default constructor.
        Class()
            : id(0)
            , isPermanent(false)
            , offlineTimeout(0)
        {}

    public:

        /// @brief The trivial destructor.
        virtual ~Class()
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
    json::Value data; ///< @brief The custom data.

    NetworkPtr network; ///< @brief The corresponding network.
    ClassPtr deviceClass; ///< @brief The corresponding device class.

    std::vector<EquipmentPtr> equipment; ///< @brief The equipment.

protected:

    /// @brief The default constructor.
    Device()
    {}

public:

    /// @brief The trivial destructor.
    virtual ~Device()
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
    static SharedPtr create(String const& id,
                            String const& name,
                            String const& key,
                            ClassPtr deviceClass = ClassPtr(),
                            NetworkPtr network = NetworkPtr())
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
    @param[in] code The equipment identifier.
    @return The equipment or NULL.
    */
    EquipmentPtr findEquipmentByCode(String const& code) const
    {
        const size_t N = equipment.size();
        for (size_t i = 0; i < N; ++i)
        {
            if (equipment[i]->code == code)
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
    String timestamp; ///< @brief The command timestamp in UTC format.
    String name; ///< @brief The command name.
    json::Value params; ///< @brief The command parameters.
    int lifetime; ///< @brief The number of seconds until this command expires.
    int flags; ///< @brief Optional flags.
    String status; ///< @brief Command status.
    json::Value result; ///< @brief Command result.

protected:

    /// @brief The default constructor.
    Command()
        : id(0)
        , lifetime(0)
        , flags(0)
    {}

public:

    /// @brief The trivial destructor.
    virtual ~Command()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Command> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] name The command name.
    @param[in] params The custom parameters.
    @return The new command.
    */
    static SharedPtr create(String const& name = String(),
                            json::Value const& params = json::Value())
    {
        SharedPtr pthis(new Command());
        pthis->name = name;
        pthis->params = params;
        return pthis;
    }
};

/// @brief The command shared pointer type.
typedef Command::SharedPtr CommandPtr;


/// @brief The notification.
/**
Represents a message dispatched by devices for clients.
*/
class Notification
{
public:
    UInt64 id; ///< @brief The notification identifier.
    String timestamp; ///< @brief The notification timestamp in UTC format.
    String name; ///< @brief The notification name.
    json::Value params; ///< @brief The notification parameters.

protected:

    /// @brief The default constructor.
    Notification()
        : id(0)
    {}

public:

    /// @brief The trivial destructor.
    virtual ~Notification()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Notification> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] name The notification name.
    @param[in] params The custom parameters.
    @return The new notification.
    */
    static SharedPtr create(String const& name = String(),
                            json::Value const& params = json::Value())
    {
        SharedPtr pthis(new Notification());
        pthis->name = name;
        pthis->params = params;
        return pthis;
    }
};

/// @brief The notification shared pointer type.
typedef Notification::SharedPtr NotificationPtr;


/// @brief The JSON serializer.
/**
This class helps to serialize/deserialize key DeviceHive objects to/from JSON values.
*/
class Serializer
{
public:

    /// @brief Update a notification from the JSON value.
    /**
    @param[in] jval The JSON value to convert.
    @param[in,out] notification The notification to update.
    @throws std::runtime_error
    */
    static void fromJson(json::Value const& jval, NotificationPtr notification)
    {
        try
        {
            notification->id = jval["id"].asUInt();
            notification->timestamp = jval["timestamp"].asString();
            notification->name = jval["notification"].asString();
            notification->params = jval["parameters"];
        }
        catch (std::exception const& ex)
        {
            OStringStream ess;
            ess << "failed to deserialize Notification:\n"
                << ex.what();
            throw std::runtime_error(ess.str().c_str());
        }
    }


    /// @brief Convert a notification to the JSON value.
    /**
    @param[in] notification The notification to convert.
    @return The JSON value.
    */
    static json::Value toJson(NotificationPtr notification)
    {
        json::Value jval;
        //jval["id"] = notification->id;
        if (!notification->timestamp.empty())
            jval["timestamp"] = notification->timestamp;
        jval["notification"] = notification->name;
        jval["parameters"] = notification->params;
        return jval;
    }

public:

    /// @brief Update a command from the JSON value.
    /**
    @param[in] jval The JSON value to convert.
    @param[in,out] command The command to update.
    */
    static void fromJson(json::Value const& jval, CommandPtr command)
    {
        try
        {
            command->id = jval["id"].asUInt();
            command->timestamp = jval["timestamp"].asString();
            command->name = jval["command"].asString();
            command->params = jval["parameters"];
            command->lifetime = jval["lifetime"].asInt32();
            command->flags = jval["flags"].asInt32();
            command->status = jval["status"].asString();
            command->result = jval["result"];
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
    @param[in] command The command to convert.
    @return The JSON value.
    */
    static json::Value toJson(CommandPtr command)
    {
        json::Value jval;
        //jval["id"] = cmd.id;
        jval["timestamp"] = command->timestamp;
        jval["command"] = command->name;
        jval["parameters"] = command->params;
        jval["lifetime"] = command->lifetime;
        jval["flags"] = command->flags;
        jval["status"] = command->status;
        jval["result"] = command->result;
        return jval;
    }

public:

    /// @brief Update a device class from the JSON value.
    /**
    @param[in] jval The JSON value.
    @param[in,out] deviceClass The device class to update.
    */
    static void fromJson(json::Value const& jval, Device::ClassPtr deviceClass)
    {
        try
        {
            deviceClass->id = jval["id"].asUInt();
            deviceClass->name = jval["name"].asString();
            deviceClass->version = jval["version"].asString();
            deviceClass->isPermanent = jval["isPermanent"].asBool();
            deviceClass->offlineTimeout = jval["offlineTimeout"].asInt32();
            deviceClass->data = jval["data"];
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
    static json::Value toJson(Device::ClassPtr deviceClass)
    {
        json::Value jval;
        //jval["id"] = deviceClass->id;
        jval["name"] = deviceClass->name;
        jval["version"] = deviceClass->version;
        jval["isPermanent"] = deviceClass->isPermanent;
        if (0 < deviceClass->offlineTimeout)
            jval["offlineTimeout"] = deviceClass->offlineTimeout;
        if (!deviceClass->data.isNull())
            jval["data"] = deviceClass->data;

        return jval;
    }

public:

    /// @brief Update a network from the JSON value.
    /**
    @param[in] jval The JSON value.
    @param[in,out] network The network to update.
    */
    static void fromJson(json::Value const& jval, NetworkPtr network)
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
    static json::Value toJson(NetworkPtr network)
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

    /// @brief Update an equipment from the JSON value.
    /**
    @param[in] jval The JSON value.
    @param[in,out] device The device to update equipment for.
    */
    static void equipmentFromJson(json::Value const& jval, DevicePtr device)
    {
        try
        {
            const String code = jval["code"].asString();
            if (code.empty())
                throw std::runtime_error("identifier is empty");

            EquipmentPtr equipment = device->findEquipmentByCode(code);
            if (!equipment)
            {
                equipment = Equipment::create(code, String(), String());
                device->equipment.push_back(equipment);
            }

            equipment->name = jval["name"].asString();
            equipment->type = jval["type"].asString();
            equipment->data = jval["data"];
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
    @param[in] equipment The equipment.
    @return The JSON value.
    */
    static json::Value toJson(EquipmentPtr equipment)
    {
        json::Value jval;
        jval["code"] = equipment->code;
        jval["name"] = equipment->name;
        jval["type"] = equipment->type;
        if (!equipment->data.isNull())
            jval["data"] = equipment->data;
        return jval;
    }

public:

    /// @brief Update a device from the JSON value.
    /**
    @param[in] jval The JSON value.
    @param[in,out] device The device to update.
    */
    static void fromJson(json::Value const& jval, DevicePtr device)
    {
        try
        {
            { // just check identifier is the same!
                const String id = jval["id"].asString();
                if (id.empty())
                    throw std::runtime_error("identifier is empty");
                if (!boost::iequals(device->id, id))
                    throw std::runtime_error("invalid identifier");
            }

            device->name = jval["name"].asString();
            //device->key = jval["key"].asString();
            device->status = jval["status"].asString();
            device->data = jval["data"];

            { // update network
                json::Value const& jnet = jval["network"];
                if (!jnet.isNull())
                {
                    if (!device->network)
                        device->network = Network::create(String(), String());
                    fromJson(jnet, device->network);
                }
            }

            { // update device class
                json::Value const& jclass = jval["deviceClass"];
                if (!jclass.isNull())
                {
                    if (!device->deviceClass)
                        device->deviceClass = Device::Class::create(String(), String());
                    fromJson(jclass, device->deviceClass);
                }
            }

            { // equipment
                json::Value const& json_eq = jval["equipment"];
                if (!json_eq.isNull() && !json_eq.isArray())
                    throw std::runtime_error("equipment is not an array");

                const size_t N = json_eq.size();
                for (size_t i = 0; i < N; ++i)
                    equipmentFromJson(json_eq[i], device);
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
    static json::Value toJson(DevicePtr device)
    {
        const size_t N = device->equipment.size();
        json::Value json_eq; json_eq.resize(N);
        for (size_t i = 0; i < N; ++i)
            json_eq[i] = toJson(device->equipment[i]);

        json::Value jval;
        //jval["id"] = device->id;
        if (!device->name.empty())
            jval["name"] = device->name;
        jval["key"] = device->key;
        jval["status"] = device->status;
        if (device->network)
            jval["network"] = toJson(device->network);
        if (device->deviceClass)
            jval["deviceClass"] = toJson(device->deviceClass);
        jval["equipment"] = json_eq;
        if (!device->data.isNull())
            jval["data"] = device->data;
        return jval;
    }
};


/// @brief The server information.
struct ServerInfo
{
    String api_version;     ///< @brief The server API version.
    String timestamp;       ///< @brief The server timestamp.
    String alternativeUrl;  ///< @brief The websocket or REST server URL.
};


/// @brief The Device Service events.
/**
*/
class IDeviceServiceEvents
{
public:

    ///@brief The error code type.
    typedef boost::system::error_code ErrorCode;

public:

    /// @brief The "connected" callback.
    /**
    @param[in] err The error code.
    */
    virtual void onConnected(ErrorCode err)
    {}


    /// @brief The "server info" callback.
    /**
    @param[in] err The error code.
    @param[in] info The server information.
    */
    virtual void onServerInfo(ErrorCode err, ServerInfo info)
    {}

public:

    /// @brief The "register device" callback.
    /**
    @param[in] err The error code.
    @param[in] device The device.
    */
    virtual void onRegisterDevice(ErrorCode err, DevicePtr device)
    {}


    /// @brief The "get device data" callback.
    /**
    @param[in] err The error code.
    @param[in] device The device.
    */
    virtual void onGetDeviceData(ErrorCode err, DevicePtr device)
    {}


    /// @brief The "update device data" callback.
    /**
    @param[in] err The error code.
    @param[in] device The device.
    */
    virtual void onUpdateDeviceData(ErrorCode err, DevicePtr device)
    {}

public:

    /// @brief The "insert command" callback.
    /**
    @param[in] err The error code.
    @param[in] device The device.
    @param[in] command The command.
    */
    virtual void onInsertCommand(ErrorCode err, DevicePtr device, CommandPtr command)
    {}


    /// @brief The "update command" callback.
    /**
    @param[in] err The error code.
    @param[in] device The device.
    @param[in] command The command.
    */
    virtual void onUpdateCommand(ErrorCode err, DevicePtr device, CommandPtr command)
    {}

public:

    /// @brief The "insert notification" callback.
    /**
    @param[in] err The error code.
    @param[in] device The device.
    @param[in] notification The notification.
    */
    virtual void onInsertNotification(ErrorCode err, DevicePtr device, NotificationPtr notification)
    {}
};


/// @brief The Device service interface.
/**
*/
class IDeviceService
{
public:

    /// @brief The trivial destructor.
    virtual ~IDeviceService()
    {}

public:

    /// @brief Cancel all tasks.
    virtual void cancelAll() = 0;

public:

    /// @brief Connect to the server.
    virtual void asyncConnect() = 0;


    /// @brief Get the server info asynchronously.
    virtual void asyncGetServerInfo() = 0;

public:

    /// @brief Register the device asynchronously.
    /**
    @param[in] device The device to register.
    */
    virtual void asyncRegisterDevice(DevicePtr device) = 0;


    /// @brief Get the device data asynchronously.
    /**
    @param[in] device The device to get data for.
    */
    virtual void asyncGetDeviceData(DevicePtr device) = 0;


    /// @brief Update the device data asynchronously.
    /**
    @param[in] device The device to update.
    */
    virtual void asyncUpdateDeviceData(DevicePtr device) = 0;

public:

    /// @brief Subscribe for commands.
    /**
    @param[in] device The device to poll commands for.
    @param[in] timestamp The timestamp of the last received command. Empty for server's "now".
    */
    virtual void asyncSubscribeForCommands(DevicePtr device, String const& timestamp) = 0;


    /// @brief Unsubscribe from commands.
    /**
    @param[in] device The device to poll commands for.
    */
    virtual void asyncUnsubscribeFromCommands(DevicePtr device) = 0;

public:

    /// @brief Update command result.
    /**
    @param[in] device The device.
    @param[in] command The command to update.
    */
    virtual void asyncUpdateCommand(DevicePtr device, CommandPtr command) = 0;

public:

    /// @brief Insert notification.
    /**
    @param[in] device The device.
    @param[in] notification The notification to insert.
    */
    virtual void asyncInsertNotification(DevicePtr device, NotificationPtr notification) = 0;
};


/// @brief The Device service shared pointer.
typedef boost::shared_ptr<IDeviceService> IDeviceServicePtr;

} // devicehive namespace

#endif // __DEVICEHIVE_SERVICE_HPP_
