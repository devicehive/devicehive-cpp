/** @file
@brief The simple device example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_simple_dev
*/
#ifndef __EXAMPLES_SIMPLE_DEV_HPP_
#define __EXAMPLES_SIMPLE_DEV_HPP_

#include <DeviceHive/restful.hpp>
#include <DeviceHive/websocket.hpp>
#include "basic_app.hpp"


/// @brief The simple device example.
namespace simple_dev
{
    using namespace hive;

/// @brief Various contants and timeouts.
enum Timeouts
{
    SERVER_RECONNECT_TIMEOUT    = 10000, ///< @brief Try to open server connection each X milliseconds.
    SENSORS_UPDATE_TIMEOUT      = 1000,  ///< @brief Try to update temperature sensors each X milliseconds.
    RETRY_TIMEOUT               = 5000,  ///< @brief Common retry timeout, milliseconds.
    DEVICE_OFFLINE_TIMEOUT      = 0
};


/// @brief The LED control.
/**
Writes the LED state "0" or "1" to the external "device" file.
*/
class LedControl:
    public devicehive::Equipment
{
public:
    String fileName; ///< @brief The target file name.

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<LedControl> SharedPtr; 


    /// @brief The factory method.
    /**
    @param[in] code The unique identifier.
    @param[in] name The custom name.
    @param[in] type The equipment type.
    @param[in] fileName The target file name.
    @return The new LED control instance.
    */
    static SharedPtr create(String const& code, String const& name,
        String const& type, String const& fileName)
    {
        SharedPtr eq(new LedControl());
        eq->code = code;
        eq->name = name;
        eq->type = type;
        eq->fileName = fileName;
        return eq;
    }

public:

    /// @brief Set the LED state.
    /**
    Writes the state to the "device" file.

    @param[in] state The string state to write.
    */
//! [LedControl_setState]
    void setState(String const& state) const
    {
        if (!fileName.empty())
        {
            std::ofstream o_file(fileName.c_str());
            o_file << state;
        }
    }
//! [LedControl_setState]
};


/// @brief The temperature sensor.
/**
Reports current temperature which is read from the external "device" file.
*/
class TempSensor:
    public devicehive::Equipment
{
public:
    String fileName; ///< @brief The source file name.
    String lastValue; ///< @brief The last value sent to the server.
    double margin; ///< @brief The temperature margin.

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<TempSensor> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] code The unique identifier.
    @param[in] name The custom name.
    @param[in] type The equipment type.
    @param[in] fileName The source file name.
    @return The new temperature sensor instance.
    */
    static SharedPtr create(String const& code, String const& name,
        String const& type, String const& fileName)
    {
        SharedPtr eq(new TempSensor());
        eq->code = code;
        eq->name = name;
        eq->type = type;
        eq->fileName = fileName;
        eq->margin = 0.2;
        return eq;
    }

public:

    /// @brief Get the temperature.
    /**
    Read the value from the file.
    There is no error if file doesn't exist.

    @return The string value.
        Empty on CRC error.
    */
//! [TempSensor_getValue]
    String getValue() const
    {
        String val;

        if (!fileName.empty())
        {
            std::ifstream i_file(fileName.c_str());

            String line;
            while (std::getline(i_file, line))
            {
                // check for CRC
                size_t p = line.find("crc=");
                if (p != String::npos)
                {
                    p = line.find("NO", p);
                    if (p != String::npos)
                        return String(); // bad CRC
                }

                p = line.find("t=");
                if (p != String::npos)
                    val = line.substr(p+2);
            }
        }

        // convert to Celcius
        if (!val.empty())
        {
            OStringStream oss;
            oss << std::fixed << std::setprecision(3)
                << (boost::lexical_cast<double>(val)/1000.0);
            val = oss.str();
        }

        return val;
    }
//! [TempSensor_getValue]


    /// @brief Check for really new temperature value.
    /**
    @param[in] val The current value.
    @return `true` if value should be sent to the server.
    */
    bool haveToSend(String const& val) const
    {
        if (!val.empty())
        {
            if (lastValue.empty())
                return true;

            const double dt = boost::lexical_cast<double>(val)
                - boost::lexical_cast<double>(lastValue);
            return fabs(dt) > margin;
        }

        return false;
    }
};


/// @brief The simple device application.
/**
Contains any number of LED controls and temperature sensors.

Updates temperature sensors every second.

@see @ref page_simple_dev
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

        String deviceId = "82d1cfb9-43f8-4a22-b708-45bb4f68ae54";
        String deviceKey = "5f5cd1fa-4455-42dd-b024-d8044d36c59e";
        String deviceName = "C++ device";

        String baseUrl = "http://ecloud.dataart.com/ecapi8";
        size_t web_timeout = 0; // zero - don't change

        String networkName = "C++ network";
        String networkKey = "";
        String networkDesc = "C++ device test network";

        String deviceClassName = "C++ test device";
        String deviceClassVersion = "0.0.1";

        // custom device properties
        std::vector<devicehive::EquipmentPtr> equipment;
        for (int i = 1; i < argc; ++i) // skip executable name
        {
            if (boost::iequals(argv[i], "--help"))
            {
                std::cout << argv[0] << " [options]";
                std::cout << "\t--deviceId <device identifier>\n";
                std::cout << "\t--deviceKey <device authentication key>\n";
                std::cout << "\t--deviceName <device name>\n";
                std::cout << "\t--networkName <network name>\n";
                std::cout << "\t--networkKey <network authentication key>\n";
                std::cout << "\t--networkDesc <network description>\n";
                std::cout << "\t--deviceClassName <device class name>\n";
                std::cout << "\t--deviceClassVersion <device class version>\n";
                std::cout << "\t--server <server URL>\n";
                std::cout << "\t--web-timeout <timeout, seconds>\n";
                std::cout << "\t--no-ws disable automatic websocket service switching\n";
                std::cout << "\t--led <id> <name> <file name>\n";
                std::cout << "\t--temp <id> <name> <file name>\n";

                exit(1);
            }
            else if (boost::iequals(argv[i], "--deviceId") && i+1 < argc)
                deviceId = argv[++i];
            else if (boost::iequals(argv[i], "--deviceName") && i+1 < argc)
                deviceName = argv[++i];
            else if (boost::iequals(argv[i], "--deviceKey") && i+1 < argc)
                deviceKey = argv[++i];
            else if (boost::iequals(argv[i], "--networkName") && i+1 < argc)
                networkName = argv[++i];
            else if (boost::iequals(argv[i], "--networkKey") && i+1 < argc)
                networkKey = argv[++i];
            else if (boost::iequals(argv[i], "--networkDesc") && i+1 < argc)
                networkDesc = argv[++i];
            else if (boost::iequals(argv[i], "--deviceClassName") && i+1 < argc)
                deviceClassName = argv[++i];
            else if (boost::iequals(argv[i], "--deviceClassVersion") && i+1 < argc)
                deviceClassVersion = argv[++i];
            else if (boost::iequals(argv[i], "--server") && i+1 < argc)
                baseUrl = argv[++i];
            else if (boost::iequals(argv[i], "--web-timeout") && i+1 < argc)
                web_timeout = boost::lexical_cast<UInt32>(argv[++i]);
            else if (boost::iequals(argv[i], "--no-ws"))
                pthis->m_disableWebsockets = true;
            else if (boost::iequals(argv[i], "--led") && i+3 < argc)
            {
                const String id = argv[++i];
                const String name = argv[++i];
                const String file = argv[++i];

                equipment.push_back(LedControl::create(id,
                    name, "Controllable LED", file));
            }
            else if (boost::iequals(argv[i], "--temp") && i+3 < argc)
            {
                const String id = argv[++i];
                const String name = argv[++i];
                const String file = argv[++i];

                equipment.push_back(TempSensor::create(id,
                    name, "Temperature Sensor", file));
            }
        }

        if (1) // create device
        {
            devicehive::DevicePtr device = devicehive::Device::create(deviceId, deviceName, deviceKey,
                devicehive::Device::Class::create(deviceClassName, deviceClassVersion, false, DEVICE_OFFLINE_TIMEOUT),
                devicehive::Network::create(networkName, networkKey, networkDesc));
            device->status = "Online";

            device->equipment.swap(equipment);
            pthis->m_device = device;
        }

        if (1) // create service
        {
            http::Url url(baseUrl);

            if (boost::iequals(url.getProtocol(), "ws")
                || boost::iequals(url.getProtocol(), "wss"))
            {
                if (pthis->m_disableWebsockets)
                    throw std::runtime_error("websockets are disabled by --no-ws switch");

                devicehive::WebsocketService::SharedPtr service = devicehive::WebsocketService::create(
                    http::Client::create(pthis->m_ios), baseUrl, pthis);
                if (0 < web_timeout)
                    service->setTimeout(web_timeout*1000); // seconds -> milliseconds

                HIVELOG_INFO_STR(pthis->m_log, "WebSocket service is used");
                pthis->m_service = service;
            }
            else
            {
                devicehive::RestfulService::SharedPtr service = devicehive::RestfulService::create(
                    http::Client::create(pthis->m_ios), baseUrl, pthis);
                if (0 < web_timeout)
                    service->setTimeout(web_timeout*1000); // seconds -> milliseconds

                HIVELOG_INFO_STR(pthis->m_log, "RESTful service is used");
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
    Registers the device on the server.
    */
    virtual void start()
    {
        Base::start();
        m_service->asyncConnect();
    }


    /// @brief Stop the application.
    /**
    Cancels all pending HTTP requests and stops the "update" timer.
    */
    virtual void stop()
    {
        m_service->cancelAll();
        Base::stop();
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
            handleError(err, "connection");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onServerInfo()
    virtual void onServerInfo(boost::system::error_code err, devicehive::ServerInfo info)
    {
        if (!err)
        {
            m_lastCommandTimestamp = info.timestamp;

            // try to switch to websocket protocol
            if (!m_disableWebsockets && !info.alternativeUrl.empty())
                if (devicehive::RestfulService::SharedPtr rest = boost::dynamic_pointer_cast<devicehive::RestfulService>(m_service))
            {
                HIVELOG_INFO(m_log, "switching to Websocket service: " << info.alternativeUrl);
                rest->cancelAll();

                devicehive::WebsocketService::SharedPtr service = devicehive::WebsocketService::create(
                    http::Client::create(m_ios), info.alternativeUrl, shared_from_this());
                service->setTimeout(rest->getTimeout());
                m_service = service;

                // connect again as soon as possible
                m_delayed->callLater(boost::bind(&devicehive::IDeviceService::asyncConnect, m_service));
                return;
            }

            m_service->asyncRegisterDevice(m_device);
        }
        else
            handleError(err, "getting server info");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onRegisterDevice()
    virtual void onRegisterDevice(boost::system::error_code err, devicehive::DevicePtr device)
    {
        if (!err)
        {
            resetAllLedControls("0");
            m_service->asyncSubscribeForCommands(m_device, m_lastCommandTimestamp);
            m_delayed->callLater(boost::bind(&This::updateSensors, shared_from_this())); // ASAP
        }
        else
            handleError(err, "registering device");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onInsertCommand()
    virtual void onInsertCommand(ErrorCode err, devicehive::DevicePtr device, devicehive::CommandPtr command)
    {
        if (!err)
        {
            m_lastCommandTimestamp = command->timestamp;
            bool processed = true;

            try
            {
                const String code = command->params["equipment"].asString();

                devicehive::EquipmentPtr eq = device->findEquipmentByCode(code);
                if (boost::iequals(command->name, "UpdateLedState"))
                {
                    if (LedControl::SharedPtr led = boost::dynamic_pointer_cast<LedControl>(eq))
                    {
                        String state = command->params["state"].asString();
                        led->setState(state);

                        command->status = "Success";
                        command->result = "Completed";

                        json::Value ntf_params;
                        ntf_params["equipment"] = led->code;
                        ntf_params["state"] = state;
                        m_service->asyncInsertNotification(device,
                            devicehive::Notification::create("equipment", ntf_params));
                    }
                    else
                        throw std::runtime_error("unknown or invalid equipment");
                }
                else
                    throw std::runtime_error("unknown command");
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
            handleError(err, "polling command");
    }

private:

    /// @brief Update all active sensors.
    void updateSensors()
    {
        //HIVELOG_TRACE(m_log, "update sensors");

        const size_t N = m_device->equipment.size();
        for (size_t i = 0; i < N; ++i)
        {
            devicehive::EquipmentPtr eq = m_device->equipment[i];
            if (TempSensor::SharedPtr sensor = boost::dynamic_pointer_cast<TempSensor>(eq))
            {
                String const val = sensor->getValue();
                if (sensor->haveToSend(val))
                {
                    json::Value ntf_params;
                    ntf_params["equipment"] = sensor->code;
                    ntf_params["temperature"] = val;
                    m_service->asyncInsertNotification(m_device,
                        devicehive::Notification::create("equipment", ntf_params));

                    sensor->lastValue = val;
                }
            }
        }

        if (!terminated())
        {
            m_delayed->callLater(SENSORS_UPDATE_TIMEOUT,
                boost::bind(&This::updateSensors, shared_from_this()));
        }
    }


    /// @brief Reset all LED controls.
    /**
    @param[in] state The LED state reset to.
    */
    void resetAllLedControls(String const& state)
    {
        const size_t N = m_device->equipment.size();
        for (size_t i = 0; i < N; ++i)
        {
            devicehive::EquipmentPtr eq = m_device->equipment[i];
            if (LedControl::SharedPtr led = boost::dynamic_pointer_cast<LedControl>(eq))
            {
                led->setState(state);

                json::Value ntf_params;
                ntf_params["equipment"] = led->code;
                ntf_params["state"] = state;
                m_service->asyncInsertNotification(m_device,
                    devicehive::Notification::create("equipment", ntf_params));
            }
        }
    }

private:

    /// @brief Handle the communication error.
    /**
    @param[in] err The error code.
    @param[in] hint The custom hint.
    */
    void handleError(boost::system::error_code err, const char *hint)
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
    devicehive::IDeviceServicePtr m_service; ///< @brief The cloud service.
    devicehive::DevicePtr m_device; ///< @brief The device.
    String m_lastCommandTimestamp; ///< @brief The timestamp of the last received command.
    bool m_disableWebsockets;       ///< @brief No automatic websocket switch.
};


/// @brief The simple device application entry point.
/**
Creates the Application instance and calls its Application::run() method.

@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
*/
inline void main(int argc, const char* argv[])
{
    { // configure logging
        using namespace hive::log;

        Target::SharedPtr log_file = Target::File::create("simple_dev.log");
        Target::SharedPtr log_console = Logger::root().getTarget();
        Logger::root().setTarget(Target::Tie::create(log_file, log_console));
        Logger::root().setLevel(LEVEL_TRACE);
        Logger("/hive/websocket").setTarget(log_file).setLevel(LEVEL_DEBUG); // disable annoying messages
        Logger("/hive/http").setTarget(log_file).setLevel(LEVEL_DEBUG); // disable annoying messages
        log_console->setFormat(Format::create("%N %L %M\n"));
        log_console->setMinimumLevel(LEVEL_DEBUG);
    }

    Application::create(argc, argv)->run();
}

} // simple_dev namespace


///////////////////////////////////////////////////////////////////////////////
/** @page page_simple_dev Simple device example

[BeagleBone]: http://beagleboard.org/bone/ "BeagleBone official site"
[RaspberryPi]: http://www.raspberrypi.org/ "RaspberryPi official site"


This simple example aims to demonstrate usage of DeviceHive framework from C++
application.

In terms of DeviceHive framework this example is **Device** implementation:

![DeviceHive framework](@ref preview2.png)

Our device is one of small computers (like [BeagleBone] or [RaspberryPi])
which runs Linux and manages the following peripheral:
    - simple LED, allowing user to switch it *ON* or *OFF*
    - DS18B20 sensor, providing user with temperature measurements

The Internet connection is provided by 3G modem connected to device's USB port.

Android application is used as a **Client** to control LED state and to monitor
temperature.

Please see @ref page_start to get information about how to prepare toolchain
and build the *boost* library.


Application
===========

Application is a console program and it's represented by an instance of
simple_dev::Application. This instance can be created using
simple_dev::Application::create() factory method. Application does all its
useful work in simple_dev::Application::run() method.

~~~{.cpp}
using namespace simple_dev;
int main(int argc, const char* argv[])
{
    Application::create(argc, argv)->run();
    return 0;
}
~~~

Application contains the following key objects:
    - simple_dev::Application::m_ios the IO service instance to perform various
      asynchronous operations (such as HTTP requests and update timers)
    - simple_dev::Application::m_device the DeviceHive device object which
      contains several equipment objects:
        - any number of simple_dev::LedControl instances
        - any number of simple_dev::TempSensor instances
    - simple_dev::Application::m_api the "clue" which links our application and
      the DeviceHive server


LED control
-----------

We assume that actual LED hardware is mapped by kernel to some *device* file
which is in virtual filesystem such as `/proc` or `/sys`. Such *device* file
name based on the GPIO pin number. For example, the `gpio38` pin
on [BeagleBone] is mapped to the `/sys/class/gpio/gpio38/value` *device* file.
Also [BeagleBone] has four default LED controls which are placed on the board
and have the following *device* files:
    - `/sys/class/leds/%beaglebone::usr0/brightness`
    - `/sys/class/leds/%beaglebone::usr1/brightness`
    - `/sys/class/leds/%beaglebone::usr2/brightness`
    - `/sys/class/leds/%beaglebone::usr3/brightness`

We bind simple_dev::LedControl instance with a *device* file. Once we've got
command from the server to change the LED state, this state
will be immediately written to the corresponding *device* file
using simple_dev::LedControl::setState() method, which is very simple:

@snippet examples/simple_dev.hpp LedControl_setState


Temperature sensor
------------------

The temperature sensor also uses *device* file. But this file is written by
hardware and simple_dev::TempSensor instance reads it periodically.
The *device* file format is not so simple as for LED control but we can extract
actual temperature value searching for the `t=` keyword.

Also we have to verify integrity of the *device* file and skip measurement if
CRC equals to "NO".

@snippet examples/simple_dev.hpp TempSensor_getValue

Typical DS18B20 file content is:

~~~
83 01 4b 46 7f ff 0d 10 66 : crc=66 YES
83 01 4b 46 7f ff 0d 10 66 t=24187
~~~

It means that current temperature is 24.187C.

Our application periodically checks all the connected temperature sensors and
sends notifications to the DeviceHive server if the temperature measurement has
been changed (more than 0.2 degree) since last time.


Control flow
------------

At the start we should register our device on the server using
simple_dev::Application::asyncRegisterDevice() method. If registration is
successful we start listening for the commands from the server using
simple_dev::Application::asyncPollCommands() method. At the same time we start
*update* timer and periodically check the temperature sensors using
simple_dev::Application::asyncUpdateSensors() method. If temperature
measurement has been changed, then we send notification to the DeviceHive
server.


Command line arguments
----------------------

Application supports the following command line arguments:

|                                   Option | Description
|-----------------------------------------|-----------------------------------------------
|          `--deviceId <id>`               | `<id>` is the device identifier
|        `--deviceName <name>`             | `<name>` is the device name
|         `--deviceKey <key>`              | `<key>` is the device authentication key
|       `--networkName <name>`             | `<name>` is the network name
|        `--networkKey <key>`              | `<key>` is the network authentication key
|       `--networkDesc <desc>`             | `<desc>` is the network description
|   `--deviceClassName <name>`             | `<name>` is the device class name
|`--deviceClassVersion <ver>`              | `<ver>` is the device class version
|            `--server <URL>`              | `<URL>` is the server URL
|               `--led <id> <name> <file>` | `<id>` is the unique identifier \n `<name>` is the LED control name \n `<file>` is the *device* file name
|              `--temp <id> <name> <file>` | `<id>` is the unique identifier \n `<name>` is the temperature sensor name \n `<file>` is the *device* file name

The `--led` and `--temp` commands can be used to create any number of equipment
instances (LED controls and temperature sensors respectively).

For example, to start application on [BeagleBone] board run the following command:

~~~{.sh}
./simple_dev --led p8_3 "LED" "/sys/class/gpio/gpio38/value" \
 --temp p8_6 "DS18B20" "/sys/bus/w1/devices/28-00000393268a/w1_slave"
~~~

Where `gpio38` is the name of output GPIO pin which is used to control LED.
And `28-00000393268a` is the name of *1-wire* device.


Make and run
============

To build example application just run the following command:

~~~{.sh}
make simple_dev
~~~

To build example application using custom toolchain use `CROSS_COMPILE` variable:

~~~{.sh}
make simple_dev CROSS_COMPILE=arm-unknown-linux-gnueabi-
~~~

Now you can copy executable to your device and use it.

~~~{.sh}
./simple_dev
~~~


Hardware details
----------------

[BeagleBone] `gpio38` refers to the `GPIO1_6` (`1*32+6=38`)
which is located in `P8_3` pin.

On [RaspberryPi] to enable 1-wire support load the `w1-gpio`
and `w1-therm` kernel modules first.


@example examples/simple_dev.hpp
*/

#endif // __EXAMPLES_SIMPLE_DEV_HPP_
