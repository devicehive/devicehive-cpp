/** @file
@brief The simple device example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_ex01
*/
#ifndef __EXAMPLES_SIMPLE_DEV_HPP_
#define __EXAMPLES_SIMPLE_DEV_HPP_

#include <DeviceHive/cloud6.hpp>
#include "basic_app.hpp"


/// @brief The simple device example.
namespace simple_dev
{
    using namespace cloud6;
    using namespace hive;


/// @brief The LED control.
/**
Writes the LED state "0" or "1" to the external "device" file.
*/
class LedControl:
    public Equipment
{
public:
    String fileName; ///< @brief The target file name.

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<LedControl> SharedPtr; 


    /// @brief The factory method.
    /**
    @param[in] id The unique identifier.
    @param[in] name The custom name.
    @param[in] type The equipment type.
    @param[in] fileName The target file name.
    @return The new LED control instance.
    */
    static SharedPtr create(String const& id, String const& name,
        String const& type, String const& fileName)
    {
        SharedPtr eq(new LedControl());
        eq->id = id;
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
    public Equipment
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
    @param[in] id The unique identifier.
    @param[in] name The custom name.
    @param[in] type The equipment type.
    @param[in] fileName The source file name.
    @return The new temperature sensor instance.
    */
    static SharedPtr create(String const& id, String const& name,
        String const& type, String const& fileName)
    {
        SharedPtr eq(new TempSensor());
        eq->id = id;
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

@see @ref page_ex01
*/
class Application:
    public basic_app::Application,
    public basic_app::ServerModule
{
    typedef basic_app::Application Base; ///< @brief The base type.
protected:

    /// @brief The default constructor.
    Application()
        : ServerModule(m_ios, m_log)
        , m_timer(m_ios)
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

        String deviceId = "82d1cfb9-43f8-4a22-b708-45bb4f68ae54";
        String deviceKey = "5f5cd1fa-4455-42dd-b024-d8044d36c59e";
        String deviceName = "C++ device";

        String baseUrl = "http://ecloud.dataart.com/ecapi6";

        String networkName = "C++ network";
        String networkKey = "";
        String networkDesc = "C++ device test network";

        String deviceClassName = "C++ test device";
        String deviceClassVersion = "0.0.1";

        // custom device properties
        std::vector<EquipmentPtr> equipment;
        for (int i = 1; i < argc; ++i) // skip executable name
        {
            if (boost::algorithm::iequals(argv[i], "--help"))
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
                std::cout << "\t--led <id> <name> <file name>\n";
                std::cout << "\t--temp <id> <name> <file name>\n";

                throw std::runtime_error("STOP");
            }
            else if (boost::algorithm::iequals(argv[i], "--deviceId") && i+1 < argc)
                deviceId = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--deviceName") && i+1 < argc)
                deviceName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--deviceKey") && i+1 < argc)
                deviceKey = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkName") && i+1 < argc)
                networkName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkKey") && i+1 < argc)
                networkKey = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkDesc") && i+1 < argc)
                networkDesc = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--deviceClassName") && i+1 < argc)
                deviceClassName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--deviceClassVersion") && i+1 < argc)
                deviceClassVersion = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--server") && i+1 < argc)
                baseUrl = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--led") && i+3 < argc)
            {
                const String id = argv[++i];
                const String name = argv[++i];
                const String file = argv[++i];

                equipment.push_back(LedControl::create(id,
                    name, "Controllable LED", file));
            }
            else if (boost::algorithm::iequals(argv[i], "--temp") && i+3 < argc)
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
            Device::SharedPtr device = Device::create(deviceId, deviceName, deviceKey,
                Device::Class::create(deviceClassName, deviceClassVersion, false, 10),
                Network::create(networkName, networkKey, networkDesc));
            device->status = "Online";

            device->equipment.swap(equipment);
            pthis->m_device = device;
        }

        pthis->initServerModule(baseUrl, pthis);
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
    Registers the device on the server.
    */
    virtual void start()
    {
        Base::start();
        asyncRegisterDevice(m_device);
    }


    /// @brief Stop the application.
    /**
    Cancels all pending HTTP requests and stops the "update" timer.
    */
    virtual void stop()
    {
        cancelServerModule();
        m_timer.cancel();
        Base::stop();
    }

private: // ServerModule

    /// @brief The "register device" callback.
    /**
    Starts listening for commands from the server and starts "update" timer.

    @param[in] err The error code.
    @param[in] device The device.
    */
    virtual void onRegisterDevice(boost::system::error_code err, DevicePtr device)
    {
        ServerModule::onRegisterDevice(err, device);

        if (!err)
        {
            resetAllLedControls("0");
            asyncPollCommands(device);
            asyncUpdateSensors(0); // ASAP
        }
    }


    /// @brief The "poll commands" callback.
    /**
    @param[in] err The error code.
    @param[in] device The device.
    @param[in] commands The list of commands.
    */
    virtual void onPollCommands(boost::system::error_code err, DevicePtr device, std::vector<Command> const& commands)
    {
        ServerModule::onPollCommands(err, device, commands);

        if (!err)
        {
            typedef std::vector<Command>::const_iterator Iterator;
            for (Iterator i = commands.begin(); i != commands.end(); ++i)
            {
                Command cmd = *i;
                String eqId = cmd.params["equipment"].asString();

                EquipmentPtr eq = device->findEquipmentById(eqId);
                if (boost::iequals(cmd.name, "UpdateLedState"))
                {
                    if (LedControl::SharedPtr led = boost::shared_dynamic_cast<LedControl>(eq))
                    {
                        String state = cmd.params["state"].asString();
                        led->setState(state);

                        cmd.status = "Completed";
                        m_serverAPI->asyncSendCommandResult(device, cmd);

                        Notification ntf;
                        ntf.name = "equipment";
                        ntf.params["equipment"] = led->id;
                        ntf.params["state"] = state;
                        m_serverAPI->asyncSendNotification(device, ntf);
                    }
                }

                // TODO: handle all errors!!!
            }

            // start poll again
            asyncPollCommands(device);
        }
    }

private:

    /// @brief Start update timer.
    /**
    @param[in] wait_sec The number of seconds to wait before update.
    */
    void asyncUpdateSensors(long wait_sec)
    {
        m_timer.expires_from_now(boost::posix_time::seconds(wait_sec));
        m_timer.async_wait(boost::bind(&Application::onUpdateSensors,
            this, boost::asio::placeholders::error));
    }


    /// @brief Update all active sensors.
    /**
    @param[in] err The error code.
    */
    void onUpdateSensors(boost::system::error_code err)
    {
        if (!err)
        {
            //HIVELOG_TRACE(m_log, "update sensors");

            const size_t N = m_device->equipment.size();
            for (size_t i = 0; i < N; ++i)
            {
                EquipmentPtr eq = m_device->equipment[i];
                if (TempSensor::SharedPtr sensor = boost::shared_dynamic_cast<TempSensor>(eq))
                {
                    String const val = sensor->getValue();
                    if (sensor->haveToSend(val))
                    {
                        Notification ntf;
                        ntf.name = "equipment";
                        ntf.params["equipment"] = sensor->id;
                        ntf.params["temperature"] = val;
                        m_serverAPI->asyncSendNotification(m_device, ntf);

                        sensor->lastValue = val;
                    }
                }
            }

            // start again
            asyncUpdateSensors(1); // TODO: update interval
        }
        else if (err == boost::asio::error::operation_aborted)
            HIVELOG_DEBUG_STR(m_log, "\"update\" timer cancelled");
        else
        {
            HIVELOG_ERROR(m_log, "\"update\" timer error: ["
                << err << "] " << err.message());
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
            EquipmentPtr eq = m_device->equipment[i];
            if (LedControl::SharedPtr led = boost::shared_dynamic_cast<LedControl>(eq))
            {
                led->setState(state);

                Notification ntf;
                ntf.name = "equipment";
                ntf.params["equipment"] = led->id;
                ntf.params["state"] = state;
                m_serverAPI->asyncSendNotification(m_device, ntf);
            }
        }
    }

private:
    boost::asio::deadline_timer m_timer; ///< @brief The update timer.
    Device::SharedPtr m_device; ///< @brief The device.
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
        log::Target::SharedPtr log_file = log::Target::File::create("simple_dev.log");
        log::Logger::root().setTarget(log::Target::Tie::create(
            log_file, log::Logger::root().getTarget()));
        log::Logger::root().setLevel(log::LEVEL_TRACE);
        log::Logger("/hive/http").setTarget(log_file).setLevel(log::LEVEL_DEBUG); // disable annoying messages
    }

    Application::create(argc, argv)->run();
}

} // simple_dev namespace


///////////////////////////////////////////////////////////////////////////////
/** @page page_ex01 C++ device example

[BeagleBone]: http://beagleboard.org/bone/ "BeagleBone official site"
[RaspberryPi]: http://www.raspberrypi.org/ "RaspberryPi official site"
[Boost]: http://www.boost.org/ "Boost official site"
[crosstool-NG]: http://crosstool-ng.org/ "crosstool-NG official site"


This simple example aims to demonstrate usage of DeviceHive framework from C++ application.

In terms of DeviceHive framework this example is **device** implementation:

![DeviceHive framework](@ref preview2.png)

Our device is one of small computers (like [BeagleBone] or [RaspberryPi])
which runs Linux and manages the following peripheral:
    - simple LED, allowing user to switch it *ON* or *OFF*
    - DS18B20 sensor, providing user with temperature measurements

The Internet connection is provided by 3G modem connected to device's USB port.

Android application is used as a **client** to control LED state and to monitor temperature.

See @ref page_ex01_appendix_hardware for more information about used hardware, schemes etc.

The full source code available in Examples/simple_dev.hpp file.


Requirements {#page_ex01_requirements}
======================================

To build this example under the Linux with ARM architecture we have to prepare appropriate toolchain
for cross compilation. Please see @ref page_ex01_appendix_toolchain for more details.

The C++ implementation of DeviceHive framework is based on [Boost] library.
To build this example you have to prepare the following libraries:
(see @ref page_ex01_appendix_boost for more details about how to do that):
    - boost.system

Also at least the following header-only [Boost] libraries are used:
    - boost.asio (which is the key part of the framework)
    - boost.shared_ptr
    - boost.bind


Application {#page_ex01_application}
====================================

Application is a console program and it's represented by an instance of simple_dev::Application.
This instance can be created using simple_dev::Application::create() factory method.
Application does all its useful work in simple_dev::Application::run() method.

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
    - simple_dev::Application::m_device the DeviceHive device object which contains several equipment objects:
        - any number of simple_dev::LedControl instances
        - any number of simple_dev::TempSensor instances
    - simple_dev::Application::m_api the "clue" which links our application and the DeviceHive server


LED control {#page_ex01_equipment_led}
--------------------------------------

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

@snippet Examples/simple_dev.hpp LedControl_setState


Temperature sensor {#page_ex01_equipment_temp}
----------------------------------------------

The temperature sensor also uses *device* file. But this file is written by hardware
and simple_dev::TempSensor instance reads it periodically. The *device* file format is not
so simple as for LED control but we can extract actual temperature value searching
for the `t=` keyword.

Also we have to verify integrity of the *device* file and skip measurement if CRC equals to "NO".

@snippet Examples/simple_dev.hpp TempSensor_getValue

Typical DS18B20 file content is:

~~~
83 01 4b 46 7f ff 0d 10 66 : crc=66 YES
83 01 4b 46 7f ff 0d 10 66 t=24187
~~~

It means that current temperature is 24.187C.

Our application periodically checks all the connected temperature sensors and sends
notifications to the DeviceHive server if the temperature measurement has been changed
since last time.



Control flow {#page_ex01_application_flow}
------------------------------------------

At the start we should register our device on the server using simple_dev::Application::asyncRegisterDevice() method.
If registration is successful we start listening for the commands from the server using simple_dev::Application::asyncPollCommands() method.
At the same time we start *update* timer and periodically check the temperature sensors using simple_dev::Application::asyncUpdateSensors().
If temperature measurement has been changed, then we send notification to the DeviceHive server.


Command line arguments {#page_ex01_application_cmdline}
-------------------------------------------------------

Application supports the following command line options:

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
./xtest --led p8_3 "LED" "/sys/class/gpio/gpio38/value" \
 --temp p8_6 "DS18B20" "/sys/bus/w1/devices/28-00000393268a/w1_slave"
~~~

Where `gpio38` is the name of output GPIO pin which is used to control LED.
And `28-00000393268a` is the name of *1-wire* device.


Make and run {#page_ex01_make_and_run}
======================================

To build example application just run the following command:

~~~{.sh}
make xtest-01
~~~

To build example application using custom toolchain use `CROSS_COMPILE` variable:

~~~{.sh}
make xtest-01 CROSS_COMPILE=arm-unknown-linux-gnueabi-
~~~

Now you can copy executable to your device and use it.

~~~{.sh}
./xtest-01
~~~

See @ref page_ex01_application_cmdline for more options.


Appendix {#page_ex01_appendix}
==============================

Preparing toolchain {#page_ex01_appendix_toolchain}
---------------------------------------------------

Since [RaspberryPi] and [BeagleBone] has ARM processor we have to use special toolchain to build our application.
There are a few tools which can help to prepare such a toolchain: crossdev, crosstool, [crosstool-NG], etc.
Let's use the last one.


###Installing [crosstool-NG]

The following [article](http://www.bootc.net/archives/2012/05/26/how-to-build-a-cross-compiler-for-your-raspberry-pi)
describes how to prepare toolchain for [RaspberryPi]. The short version is:
    - download and unpack
    - `./configure && make && sudo make install`
    - `cd ./tools/crosstool-ng/raspberrypi`
    - `ct-ng menuconfig`
    - `ct-ng build`

The [crosstool-NG] package has the following dependencies: `bison` `flex` `texinfo` `subversion` `libtool` `automake` `libncurses5-dev`
You may install all of these dependencies using the following command:

~~~{.sh}
sudo apt-get install bison flex texinfo subversion libtool automake libncurses5-dev
~~~


###Simple "Hello World!" application

To check cross compilation is working just build the "Hello" application:
~~~{.sh}
make hello CROSS_COMPILE=arm-unknown-linux-gnueabi-
~~~

The following command
~~~{.sh}
file hello
~~~
should print something like:
~~~{.sh}
hello: ELF 32-bit LSB executable, ARM, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 3.5.0, not stripped
~~~

Now you can copy executable to your [RaspberryPi] device and check it's working fine:
~~~{.sh}
./hello World
~~~


Building boost {#page_ex01_appendix_boost}
------------------------------------------

The [Boost] library is widely used in DeviceHive C++ framework. Although most of the [Boost] libraries are header-only,
there are a few [Boost] libraries which we have to build using ARM toolchain.

- download and unpack boost library
- `./bootstrap.sh --with-libraries=date_time,thread,system --prefix=/usr/local/raspberrypi`
- `./b2 toolset=gcc-arm`

Before build the boost you have ensure that the `~/user-config.jam` configuration file contains the following line:
~~~
using gcc : arm : arm-unknown-linux-gnueabi-g++ ;
~~~

Now you can link your application with boost static or shared libraries.


Hardware details {#page_ex01_appendix_hardware}
---------------------------------------------

[BeagleBone] `gpio38` refers to the `GPIO1_6` (`1*32+6=38`) which is located in `P8_3` pin.

On [RaspberryPi] to enable 1-wire support load the `w1-gpio` and `w1-therm` kernel modules.

@example Examples/simple_dev.hpp
*/

#endif // __EXAMPLES_SIMPLE_DEV_HPP_
